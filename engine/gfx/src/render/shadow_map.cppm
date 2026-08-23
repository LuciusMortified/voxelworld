export module vw.gfx:render.shadow_map;

import std;

import vw.core;
import vw.ecs;
import vw.world;
import vw.platform;
import :camera;
import :resource;
import :render.vulkan_context;
import vulkan;

namespace vw::gfx {
using namespace ::vw::ecs;
using namespace ::vw::plat;
}

export namespace vw::gfx {

class vulkan_context;
class camera;

// То, что у каскадов стоит крутить руками. Каждая настройка разменивает резкость
// на долю кадра, которую стоят тени, а баланс зависит от того, насколько далеко
// в конкретной игре реально уходит взгляд.
struct shadow_settings {
    // Выключено. За первым каскадом край шёл лесенкой, и лесенка ехала вслед за
    // камерой; ни одно сочетание настроек ниже этого не убрало. Всё здесь
    // оставлено как было, чтобы решение можно было отменить, но не работает
    // ничего: ни прохода глубины, ни отсева каскадов, ни фильтрации. Парный
    // выключатель — SHADOW_ENABLED в voxel.frag, и они обязаны совпадать. Работу
    // за тени делает небесный свет.
    bool enabled = false;

    // Где кончается первый каскад. Это не дальность резкой зоны: тексель первого
    // каскада примерно тысячная от этого значения, поэтому вынос границы дальше
    // размывает зону, а не удлиняет её. Покупается этим отношение между соседями —
    // то, из чего сделана лесенка. 32 из тысячи дают отношение 2,37: 1,6
    // экранного пикселя лесенки в первом каскаде и 2,3 у ближнего края каждого
    // следующего.
    float32 first_split = 32.0f;

    // Насколько далеко тени отбрасываются вообще. Дальше мир освещён плоско.
    float32 distance = 1000.0f;

    // Насколько солнце вправе повернуться до перерисовки каскада, в текселях
    // сдвига на его ободе. Меньше — плавнее под движущимся солнцем и дороже по
    // перерисовкам. Поставь достаточно мало, и настройка перестанет решать хоть
    // что-то: каждый каскад грязен каждый кадр, и всей политикой становится
    // бюджет ниже — карусель по каскаду за кадр. Хотеть этого разумно, но так
    // частота обновления теней привязывается к частоте кадров, а не к скорости
    // солнца.
    float32 turn_texels = 0.05f;

    // Каскадов, перерисовываемых за кадр. Весь смысл кэша каскадов в том, что это
    // число сильно меньше их количества.
    uint32 updates_per_frame = 1;

    // Полутень, в текселях того каскада, из которого идёт выборка.
    //
    // Половина текселя — почти ничего, и в этом суть: ширина стоила своих денег,
    // пока лесенка была в пять пикселей, а дальние каскады прыгали решёткой
    // целиком. При отношении 2,37 и одинаковой частоте обновления край стоит
    // ровно сам по себе, а размытие стоит контакта грани с её же тенью — и ничего
    // не даёт взамен.
    float32 filter_texels = 0.5f;

    // Сдвиг глубины вдоль нормали поверхности, в текселях, масштабированный
    // шириной фильтра. Настолько узкое ядро читает почти ту глубину, которую ему
    // дали, поэтому сдвиг ему почти не нужен, а лишний сдвиг — это тень, стянутая
    // с того, кто её отбрасывает.
    float32 normal_bias = 0.0f;
    float32 slope_bias  = 0.5f;
};

class shadow_map {
public:
    // Пять, а не четыре. На границе каскадов видно отношение между соседями, и при
    // геометрической прогрессии разбиений оно всюду одинаково: на одном и том же
    // диапазоне пять каскадов идут корнем четвёртой степени там, где четыре идут
    // третьей, а лесенка равна этому отношению на 1,66 экранного пикселя. На
    // 24..1000 это было 3,47 против 2,54, семь пикселей против пяти. Цена — пятый
    // слой массива теней, 17 МБ.
    static constexpr uint32 cascade_count = 5;

    explicit shadow_map(vulkan_context& context, uint32 size = 2048);

    [[nodiscard]] auto get_size() const -> uint32 { return size_; }
    ~shadow_map();

    shadow_map(const shadow_map&)                    = delete;
    auto operator=(const shadow_map&) -> shadow_map& = delete;
    shadow_map(shadow_map&&)                         = delete;
    auto operator=(shadow_map&&) -> shadow_map&      = delete;

    auto update(const camera& camera, const vec3f& light_direction) -> void;

    [[nodiscard]] auto get_settings() -> shadow_settings& { return settings_; }
    [[nodiscard]] auto get_settings() const -> const shadow_settings& { return settings_; }

    // Каскад хранит глубину в мировых координатах, поэтому карта, нарисованная на
    // прошлых кадрах, остаётся верной, пока покрывает запрашиваемый отрезок.
    // Каждый каскад поэтому строится с запасом и перерисовывается, только когда
    // камера вышла за этот запас, повернулся свет или изменилась геометрия внутри.
    auto invalidate(const vw::spatial::aabb& bounds) -> void;
    auto invalidate_all() -> void;
    [[nodiscard]] auto is_cascade_pending(uint32 cascade_index) const -> bool;
    [[nodiscard]] auto get_pending_count() const -> uint32;
    auto clear_pending() -> void;

    [[nodiscard]] auto get_light_space_matrix(uint32 cascade_index) const -> mat4f;
    [[nodiscard]] auto get_light_space_matrices() const -> const std::array<mat4f, cascade_count>&;
    [[nodiscard]] auto get_cascade_splits() const -> const std::array<float32, cascade_count>&;

    // Мировых единиц на тексель тени, по каскадам. В них фрагментный шейдер меряет
    // свой сдвиг и ширину фильтра.
    [[nodiscard]] auto get_cascade_texel_sizes() const
        -> const std::array<float32, cascade_count>&;
    [[nodiscard]] auto get_cascade_frustums() const
        -> const std::array<vw::spatial::frustum, cascade_count>&;
    [[nodiscard]] auto get_image() const -> vk::Image;
    [[nodiscard]] auto get_image_view(uint32 cascade_index) const -> vk::ImageView;
    [[nodiscard]] auto get_array_image_view() const -> vk::ImageView;
    [[nodiscard]] auto get_sampler() const -> vk::Sampler;
    [[nodiscard]] auto get_debug_sampler() const -> vk::Sampler;
    [[nodiscard]] auto get_framebuffer(uint32 cascade_index) const -> vk::Framebuffer;
    [[nodiscard]] auto get_render_pass() const -> vk::RenderPass;

private:
    auto create_shadow_map_image() -> void;
    auto create_sampler() -> void;
    auto create_framebuffers() -> void;
    auto create_render_pass() -> void;
    auto cleanup() -> void;

    [[nodiscard]] auto select_cascades_(
        const std::array<vec3f, cascade_count>& centers,
        const std::array<float32, cascade_count>& radii
    ) -> uint32;

    auto build_cascade_matrix_(
        uint32 cascade_index,
        const std::array<vec3f, 8>& corners,
        const vec3f& center,
        float32 radius,
        const vec3f& light_dir,
        float32 shadow_dist
    ) -> void;

    vulkan_context* context_;

    vk::Image shadow_image_                                              = nullptr;
    vk::DeviceMemory shadow_image_memory_                                = nullptr;
    vk::ImageView shadow_array_image_view_                               = nullptr;
    std::array<vk::ImageView, cascade_count> shadow_cascade_image_views_ = {};
    vk::Sampler shadow_sampler_                                          = nullptr;
    vk::Sampler debug_sampler_                                           = nullptr;
    std::array<vk::Framebuffer, cascade_count> shadow_framebuffers_      = {};
    vk::RenderPass shadow_render_pass_                                   = nullptr;

    std::array<mat4f, cascade_count> light_space_matrices_            = {};
    std::array<vw::spatial::frustum, cascade_count> cascade_frustums_ = {};
    std::array<float32, cascade_count> cascade_splits_                = {};

    std::array<float32, cascade_count> cascade_texel_sizes_ = {};
    std::array<vec3f, cascade_count> drawn_centers_         = {};
    std::array<float32, cascade_count> drawn_radii_         = {};

    // Направление, с которым каскад был нарисован на самом деле, а не направление
    // прошлого кадра. Держится по каскадам, потому что перерисовываются они по
    // паре за раз и отстают от солнца на разную величину.
    std::array<vec3f, cascade_count> drawn_light_dirs_ = {};

    // Сколько кадров каскад числится грязным, но не выбран. Разрешает ничью между
    // каскадами, которым перерисовка нужна одинаково сильно.
    std::array<uint32, cascade_count> frames_waited_ = {};
    uint32 dirty_mask_   = 0;
    uint32 pending_mask_ = 0;

    uint32 size_;

    shadow_settings settings_{};

    // Разбиения, под которые каскады строились в последний раз. Изменение любого
    // двигает все границы, и каскад, закэшированный под старые, покрывает не тот
    // отрезок.
    float32 built_first_split_ = 0.0f;
    float32 built_distance_    = 0.0f;

    // Запас покрытия, обменянный на частоту обновления: карта строится настолько
    // больше нужного отрезка, чтобы оставаться верной, пока камера дрейфует в
    // пределах запаса. Стоит разрешения, покупает кадры вовсе без работы над
    // тенями.
    float32 cascade_padding_ratio_ = 0.18f;

    // Перерисовку хотят чуть раньше, чем запас кончится. Этот узкий зазор —
    // единственное, в чём каскад может подождать, а ожидание и не даёт четырём из
    // них подойти к сроку в одном кадре.
    float32 cascade_trigger_ratio_ = 0.15f;
};

}  // namespace vw::gfx
