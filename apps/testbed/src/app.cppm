export module vw.testbed:app;

import std;

import vw.core;
import vw.ecs;
import vw.world;
import vw.platform;
import vw.gfx;

import :args;
import :probes.clusters;
import :scene;

export namespace vw::testbed {

// Несомый источник круглый там, где лужа поставленного блока — ромб, поэтому
// одна и та же досягаемость у них накрывает разный пол. Сечение ромба по свету
// — квадрат площади 2r^2, шара — круг pi*r^2, и сходятся они на r * sqrt(2/pi).
// Излучение четырнадцать поэтому несёт одиннадцать вокселей, а не четырнадцать,
// и лужа на земле выходит того размера, какой дал бы блок.
inline constexpr float32 round_reach = 0.8f;

// Что делает левая кнопка при захваченном курсоре. По умолчанию ничего: это
// прежде всего стенд для замеров, и случайный клик, переложивший рельеф, тихо
// испортил бы прогон.
enum class edit_tool : int32 {
    none = 0,
    place,
    remove,
};

struct block_choice {
    const char* name;
    block_id id;
};

// Короткое меню, а не все сорок восемь цветов палитры. Два светящих идут
// первыми, потому что ради них всё и затевалось; остального хватает, чтобы
// построить что-нибудь, на что этот свет упадёт.
constexpr std::array<block_choice, 8> block_menu{{
    {"lamp (emits 14)", blocks::lamp},
    {"lava (emits 15)", blocks::lava},
    {"stone", blocks::gray_5},
    {"dark stone", blocks::gray_2},
    {"grass", blocks::green_5},
    {"dirt", blocks::brown_2},
    {"sand", blocks::orange_5},
    {"white", blocks::white},
}};

// Воксель под прицелом и пустой перед ним, в воксельных координатах, а не в
// мировых. В какой из двух пишет инструмент — вся разница между «поставить» и
// «убрать».
struct voxel_pick {
    vec3i solid;
    vec3i empty;
};

// Стенд: мир, камера, инструменты правки, день с ночью и всё прочее, что
// одинаково для любой сцены. Сама сцена живёт отдельно и получает стенд в
// конструкторе.
class testbed_app final : public gfx::app {
public:
    testbed_app(gfx::engine& eng, const arg_reader& args, const scene_factory& make_scene);
    ~testbed_app() override;

    testbed_app(const testbed_app&)                    = delete;
    auto operator=(const testbed_app&) -> testbed_app& = delete;
    testbed_app(testbed_app&&)                         = delete;
    auto operator=(testbed_app&&) -> testbed_app&      = delete;

    [[nodiscard]] auto is_bench_ready() const -> bool override;
    auto render(float32 delta_time) -> void override;

    // Всё, что стенд знает о прогоне: блок сцены и показания приборов. Движок
    // спрашивает это перед тем, как записать отчёт, поэтому оно попадает и в
    // текст, и в JSON.
    auto collect_report(gfx::report& out) const -> void override;

    // ——— то, чем стенд обслуживает сцену ———

    [[nodiscard]] auto engine() const -> gfx::engine& {
        return get_engine();
    }

    [[nodiscard]] auto world() const -> ecs::world&;
    [[nodiscard]] auto renderer() const -> gfx::renderer&;
    [[nodiscard]] auto camera() const -> gfx::camera&;

    [[nodiscard]] auto grid() const -> ecs::world_grid& {
        return *world_grid_;
    }

    [[nodiscard]] auto terrain() const -> ecs::perlin_terrain_generator& {
        return *generator_;
    }

    // Сколько мировых единиц в вокселе. Спрашивают этого столько же, сколько
    // сам генератор: правка мира говорит в мировых, а рельеф отвечает в
    // вокселях, и путать их — промах ровно в этот множитель, притом молчаливый.
    [[nodiscard]] auto voxel_scale() const -> int32 {
        return generator_params_.voxel_scale;
    }

    // Высота, на которой стоит камера бенча: земля над началом координат плюс
    // просвет. Известна только после того, как колонка под камерой загрузилась.
    [[nodiscard]] auto altitude() const -> float32 {
        return bench_altitude_;
    }

    [[nodiscard]] auto camera_placed() const -> bool {
        return camera_placed_;
    }

    // Все три очереди — генерация, свет, меш — пусты, то есть мир вокруг
    // догрузился.
    [[nodiscard]] auto streaming_settled() const -> bool;

    // Сцена ведёт камеру сама, если стенд в режиме замера. Свободные режимы
    // (--lights, --blobs) оставляют камеру человеку.
    [[nodiscard]] auto drives_camera() const -> bool {
        return drives_camera_;
    }

    // Куб светящих блоков, вкопанный в землю под камерой: сцены ставят им своё
    // содержимое, а UI — по кнопке.
    auto drop_emitter(block_id id, int32 radius) -> void;

private:
    auto setup_world_grid() -> void;
    auto try_place_camera() -> void;

    auto tick_day_night_(float32 delta_time) -> void;
    auto apply_time_of_day_() -> void;
    auto step_time_of_day_(float32 delta) -> void;

    auto set_torch_(bool on) -> void;
    auto tick_torch_(const vec3f& at) -> void;

    [[nodiscard]] auto pick_voxel_() const -> std::optional<voxel_pick>;
    auto update_hovered_() -> void;
    auto draw_hover_() -> void;
    auto apply_tool_() -> void;

    auto render_ui() -> void;
    auto handle_key_press(plat::keyboard::keys key) -> void;



    std::unique_ptr<gfx::free_camera_controller> camera_controller_;
    ecs::world_grid* world_grid_ = nullptr;
    ecs::entity viewer_          = ecs::invalid_entity;
    ecs::perlin_terrain_generator* generator_ = nullptr;
    ecs::perlin_terrain_generator::params generator_params_;
    bool camera_placed_ = false;

    // Полдень для начала, чтобы первым увиденным был тот свет, под который
    // настраивался весь остальной движок.
    float32 time_of_day_        = 0.5f;
    float32 day_length_seconds_ = 120.0f;
    float32 night_intensity_    = 0.06f;
    bool day_night_running_     = true;
    bool sun_in_bench_          = false;

    std::string drop_status_;
    ecs::entity torch_ = ecs::invalid_entity;

    edit_tool tool_     = edit_tool::none;
    int32 place_choice_ = 0;
    int32 reach_voxels_ = 12;
    int32 edit_clicks_  = 0;
    std::optional<voxel_pick> hovered_;

    float32 bench_altitude_   = 0.0f;
    mutable bool bench_ready_ = false;
    bool world_ready_         = false;
    bool drives_camera_       = false;

    cluster_probe clusters_;

    // Строится последней: конструктор сцены вправе спрашивать стенд про мир и
    // камеру, а к этому моменту всё остальное уже стоит.
    std::unique_ptr<scene> scene_;
};

}  // namespace vw::testbed
