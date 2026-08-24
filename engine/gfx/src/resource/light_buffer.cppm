export module vw.gfx:resource.light_buffer;

import std;

import vw.core;
import vw.ecs;
import vw.world;
import :gpu_buffers;
import vulkan;

namespace vw::gfx {
using namespace ::vw::ecs;
}

export namespace vw::gfx {


class vulkan_context;

// Для точечных источников (в SSBO). Два числа, а не пять: затухание то же, что у
// запечённого канала, а ему нужны пик и дальность. См. light_component.
struct point_light_data {
    alignas(16) vec4f position;
    alignas(16) vec4f color;
    alignas(4) float32 intensity;
    alignas(4) float32 range;
};

// Насколько глубоко под телом его пятно ещё стоит рисовать, в высотах падения.
//
// Раньше такого числа у диска не было вовсе — он сужается как единица на единицу
// плюс высота и никогда не доходит до нуля. Это годилось, пока восемь пятен ехали
// в кадровом uniform и каждый пиксель земли обходил все восемь; для отсева это не
// годится: ничто неограниченное нельзя положить в список мест, до которых оно
// достаёт.
//
// Три — потому что к этой высоте диск вчетверо уже и в шестнадцать раз бледнее:
// для тел из дерева это меньше трети вокселя в поперечнике и около девяти уровней
// восьмибитного канала. Последняя четверть дальности гасится в шейдере, чтобы
// конец был затуханием, а не границей.
constexpr float32 blob_reach_falls = 3.0F;

// Один участок земли, затемнённый под одним телом.
struct blob_data {
    // xyz — где стоят ноги тела, w — радиус диска под ним.
    alignas(16) vec4f position_radius;

    // x — высота, на которой сказывается подъём, y — насколько темна середина,
    // z — рост тела, то есть где кончается земля под ним и начинается оно само,
    // w — насколько глубоко вниз оно ещё достаёт.
    alignas(16) vec4f params;

    // Колонка земли, которую это затемняет, двумя концами капсулы: xyz верха,
    // w радиус, затем xyz низа. Считается на CPU и передаётся, а не выводится
    // дважды: отсев и эталон обязаны совпасть по ней точно, а дешевле всего
    // совпасть, получив одни и те же числа.
    //
    // Капсула, а не шар, потому что колонка высокая и тонкая: на двух сотнях тел
    // шар вокруг неё занимал 27308 кластеров за кадр там, где сама колонка
    // занимает неполные шесть тысяч.
    alignas(16) vec4f cull_a;
    alignas(16) vec4f cull_b;
};

class light_buffer {
public:
    using world_type = world;

    // По буферу на кадр в полёте, то же кольцо, что у кадровых uniform. Один
    // host-visible буфер CPU переписывал, пока предыдущий кадр мог его ещё
    // читать: невидимо, пока единственный читатель — фрагментный шейдер, и совсем
    // не безобидно, когда в него указывает список индексов.
    static constexpr uint32 max_frames_in_flight = 2;

    explicit light_buffer(
        vulkan_context& context,
        deletion_queue& deletion,
        vk::DescriptorPool descriptor_pool,
        vk::DescriptorSetLayout descriptor_set_layout
    );
    ~light_buffer();

    // Перестраивается каждый кадр, а не при изменении компонента. Раньше было
    // второе, и это было неверно: перемещение сущности не трогает её
    // light_component, поэтому факел в руках игрока загружал свою позицию один раз
    // и потом вечно освещал место, откуда вышел. Шестьдесят четыре источника — это
    // два килобайта; ранний выход не экономил ничего, что стоило бы такой ошибки.
    //
    // Отсев по фрустуму здесь же, потому что фрагментный шейдер обходит все
    // источники буфера на каждый пиксель и пропустить хоть один не умеет.
    auto update(
        world_type& world, const spatial::frustum& frustum, const vec3f& eye, uint32 frame_index
    ) -> void;

    [[nodiscard]] auto get_descriptor_set(uint32 frame_index) const -> vk::DescriptorSet;
    [[nodiscard]] auto is_empty() const -> bool;
    [[nodiscard]] auto get_lights_count() const -> uint32;

    // Что попало в буфер в этом кадре, в том порядке, в каком его индексирует
    // шейдер. Отсеву это нужно, чтобы сказать, о чём его спрашивали, а хранение
    // стоит той единственной аллокации, которую вектор делал и так.
    [[nodiscard]] auto get_lights() const -> std::span<const point_light_data> {
        return lights_;
    }

    // Предел, оставляющий ближайшие и отбрасывающий остальные. По умолчанию
    // no_cap: с фроксельными списками фрагмент больше не обходит буфер, поэтому
    // число, защищавшее попиксельный цикл, не защищает ничего и только теряет
    // источники. --bench-visible возвращает предел, чтобы оценить наивный путь.
    static constexpr uint32 no_cap = std::numeric_limits<uint32>::max();

    [[nodiscard]] auto get_max_visible() -> uint32& {
        return max_visible_;
    }

private:
    auto expand_buffer_if_needed_(uint32 frame_index, uint32 required_count) -> void;
    auto update_descriptor_set_(uint32 frame_index) -> void;

    static constexpr uint32 default_capacity_ = 64;

    uint32 max_visible_ = no_cap;

    vulkan_context* context_;
    deletion_queue* deletion_;
    std::vector<point_light_data> lights_;
    std::array<uint32, max_frames_in_flight> capacities_{};
    std::array<std::unique_ptr<storage_buffer>, max_frames_in_flight> lights_buffers_;
    std::array<vk::DescriptorSet, max_frames_in_flight> descriptor_sets_{};

    vk::DescriptorPool descriptor_pool_            = nullptr;
    vk::DescriptorSetLayout descriptor_set_layout_ = nullptr;

    uint32 lights_count_ = 0;
};

struct light_cull_ubo {
    alignas(16) float32 view[16]{};
    alignas(16) vec4f cluster_params{};
    alignas(16) vec4f cluster_extent{};
    alignas(16) vec4f screen_dims{};
    alignas(16) vec4<uint32> cull_dims{};
};

// Пятна под телами, в том же дескрипторном наборе, что и источники, и на том же
// кольце. Биндинг 3 набора 3; наборы принадлежат light_buffer — поэтому они
// приходят снаружи, а не выделяются здесь.
class blob_buffer {
public:
    using world_type = world;

    static constexpr uint32 max_frames_in_flight = 2;

    blob_buffer(
        vulkan_context& context,
        deletion_queue& deletion,
        std::span<const vk::DescriptorSet> sets
    );

    // Ни предела, ни отбора ближайших. Раньше было восемь мест и nth_element,
    // решавший, каких именно восемь, отчего девятое тело в толпе стояло вообще ни
    // на чём; со списком на кластер пиксель земли обходит два-три накрывающих его
    // пятна, а не все тела кадра.
    auto update(world_type& world, uint32 frame_index) -> void;

    [[nodiscard]] auto get_blobs() const -> std::span<const blob_data> {
        return blobs_;
    }

    [[nodiscard]] auto get_count() const -> uint32 {
        return static_cast<uint32>(blobs_.size());
    }

private:
    auto expand_buffer_if_needed_(uint32 frame_index, uint32 required_count) -> void;
    auto write_binding_(uint32 frame_index) -> void;

    static constexpr uint32 default_capacity_ = 32;

    vulkan_context* context_;
    deletion_queue* deletion_;
    std::vector<blob_data> blobs_;
    std::array<uint32, max_frames_in_flight> capacities_{};
    std::array<std::unique_ptr<storage_buffer>, max_frames_in_flight> buffers_;
    std::array<vk::DescriptorSet, max_frames_in_flight> sets_{};
};
}  // namespace vw::gfx
