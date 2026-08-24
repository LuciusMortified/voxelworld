export module vw.testbed:options;

import std;

import vw.core;

export namespace vw::testbed {

// Несомый источник круглый там, где лужа поставленного блока — ромб, поэтому
// одна и та же досягаемость у них накрывает разный пол. Сечение ромба по свету
// — квадрат площади 2r^2, шара — круг pi*r^2, и сходятся они на r * sqrt(2/pi).
// Излучение четырнадцать поэтому несёт одиннадцать вокселей, а не четырнадцать,
// и лужа на земле выходит того размера, какой дал бы блок.
inline constexpr float32 round_reach = 0.8f;

// Что командная строка может сказать про сам стенд, безотносительно сцены.
struct stand_options {
    bool chunk_cull   = false;
    bool sun_in_bench = false;

    // Замерный прогон: камеру ведёт сцена, а не человек, и приложение выходит
    // само, отсчитав кадры. Свободные режимы (--lights, --blobs) оставляют
    // камеру мыши.
    bool bench_running = false;

    // 0 оставляет движку его умолчание, для всех четырёх.
    uint32 visible_lights = 0;
    uint32 cluster_tile   = 0;
    uint32 cluster_slices = 0;
    uint32 cluster_cap    = 0;

    bool clustered_lights = true;

    // Вычитка сетки источников: stats — дешёвая половина, verify — она же плюс
    // эталон на CPU раз в verify_every кадров.
    bool cluster_stats  = false;
    uint32 verify_every = 0;
};

// Дальше — по параметрам на сцену. Каждая читает только свой блок: у сцены нет
// повода знать, сколько ламп ставит соседняя.

struct dig_options {
    int32 per_frame = 1;
};

struct light_options {
    int32 lamps_per_frame = 1;

    // Контрольный прогон: те же правки, та же геометрия, блок который не светит.
    // Разница двух прогонов и есть цена света.
    bool inert = false;
};

struct torches_options {
    int32 static_lights  = 400;
    int32 dynamic_lights = 64;
    int32 village_groups = 24;

    // Эмиттеров за кадр, пока сцена расставляется. Тот же --bench-lamps, что и
    // у light: обе ставят светящие блоки, просто с разными целями.
    int32 per_frame = 1;

    // Во сколько раз быстрее ходят движущиеся источники. Замерный прогон, где
    // это не единица, меряет другую сцену.
    float32 light_speed = 1.0F;

    // Та же сцена без бенча и без камеры на рельсах — чтобы посмотреть глазами.
    bool free_camera = false;
};

struct blobs_options {
    int32 bodies     = 8;
    bool free_camera = false;
};

struct crowd_options {
    uint32 size = 0;
};

// Всё вместе — то, что разбирает командная строка и получает стенд.
struct testbed_options {
    std::string scene;

    stand_options stand;
    dig_options dig;
    light_options light;
    torches_options torches;
    blobs_options blobs;
    crowd_options crowd;
};

}  // namespace vw::testbed
