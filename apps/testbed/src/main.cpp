import std;

import vw.core;
import vw.gfx;
import vw.testbed;

using namespace vw;

namespace {

auto option_value(
    std::string_view arg, std::string_view name
) -> std::optional<std::string_view> {
    if (arg.starts_with(name) && arg.size() > name.size() && arg[name.size()] == '=') {
        return arg.substr(name.size() + 1);
    }
    return std::nullopt;
}

auto parse_uint(std::string_view text, uint32 fallback) -> uint32 {
    uint32 value = 0;
    const auto* end = text.data() + text.size();
    const auto result = std::from_chars(text.data(), end, value);
    return (result.ec == std::errc{} && result.ptr == end) ? value : fallback;
}

auto parse_float(std::string_view text, float32 fallback) -> float32 {
    float32 value = 0.0F;
    const auto* end = text.data() + text.size();
    const auto result = std::from_chars(text.data(), end, value);
    return (result.ec == std::errc{} && result.ptr == end) ? value : fallback;
}

// Разбор командной строки целиком: имя сцены, размеры бенча и параметры каждой
// сцены. Сцены сами про ключи ничего не знают — им достаётся только их блок.
auto parse(int argc, char** argv, gfx::bench_config& bench, testbed::testbed_options& opts)
    -> void {
    std::optional<std::string_view> value;

    for (int32 i = 1; i < argc; ++i) {
        const std::string_view arg{argv[i]};

        if ((value = option_value(arg, "--bench"))) {
            opts.scene                = std::string{*value};
            opts.stand.bench_running  = true;
            bench.measure_frames      = 2000;
            bench.warmup_frames       = 200;
            bench.fixed_delta_seconds = 1.0f / 60.0f;

            if (*value == "crowd" && opts.crowd.size == 0) {
                opts.crowd.size = 50;
            }
        }
    }

    for (int32 i = 1; i < argc; ++i) {
        const std::string_view arg{argv[i]};

        if ((value = option_value(arg, "--bench-frames"))) {
            bench.measure_frames = parse_uint(*value, 2000);
        } else if ((value = option_value(arg, "--bench-warmup"))) {
            bench.warmup_frames = parse_uint(*value, 200);
        } else if ((value = option_value(arg, "--bench-out"))) {
            bench.report_path = std::string{*value};
        } else if ((value = option_value(arg, "--bench-json"))) {
            bench.json_path = std::string{*value};
        } else if ((value = option_value(arg, "--bench-crowd"))) {
            opts.crowd.size = parse_uint(*value, 50);
        } else if ((value = option_value(arg, "--bench-dig"))) {
            opts.dig.per_frame = static_cast<int32>(parse_uint(*value, 1));
        } else if ((value = option_value(arg, "--bench-lamps"))) {
            const auto lamps        = static_cast<int32>(parse_uint(*value, 1));
            opts.light.lamps_per_frame = lamps;
            opts.torches.per_frame     = lamps;
        } else if ((value = option_value(arg, "--bench-static"))) {
            opts.torches.static_lights = static_cast<int32>(parse_uint(*value, 400));
        } else if ((value = option_value(arg, "--bench-dynamic"))) {
            opts.torches.dynamic_lights = static_cast<int32>(parse_uint(*value, 64));
        } else if ((value = option_value(arg, "--bench-visible"))) {
            opts.stand.visible_lights = parse_uint(*value, 0);
        } else if ((value = option_value(arg, "--bench-groups"))) {
            opts.torches.village_groups =
                std::max(static_cast<int32>(parse_uint(*value, 24)), 1);
        } else if ((value = option_value(arg, "--bench-bodies"))) {
            opts.blobs.bodies = static_cast<int32>(parse_uint(*value, 8));
        } else if ((value = option_value(arg, "--cluster-tile"))) {
            opts.stand.cluster_tile = parse_uint(*value, 0);
        } else if ((value = option_value(arg, "--cluster-slices"))) {
            // Один срез — это ровно плоские тайлы, единственный честный способ
            // назвать цену резки по глубине.
            opts.stand.cluster_slices = parse_uint(*value, 0);
        } else if ((value = option_value(arg, "--cluster-cap"))) {
            opts.stand.cluster_cap = parse_uint(*value, 0);
        } else if (arg == "--cluster-stats") {
            // Дешёвая половина вычитки: насколько полна сетка и сколько из неё
            // перелилось, без самих списков.
            opts.stand.cluster_stats = true;
        } else if ((value = option_value(arg, "--verify-lights"))) {
            opts.stand.verify_every = parse_uint(*value, 60);
        } else if (arg == "--verify-lights") {
            opts.stand.verify_every = 60;
        } else if (arg == "--no-clusters") {
            // Другой путь: каждый источник кадра обходится на пиксель. Список
            // фрокселей стоит числа только рядом с этим.
            opts.stand.clustered_lights = false;
        } else if (arg == "--blobs") {
            // Сцена тел без ведущего: кольцо встаёт, тела качаются, камера ваша.
            opts.scene             = "blobs";
            opts.blobs.free_camera = true;
        } else if ((value = option_value(arg, "--light-speed"))) {
            // Во сколько раз быстрее ходят движущиеся источники, единица — то,
            // чем пользуется бенч. Замерный прогон, который это ставит, меряет
            // другую сцену.
            opts.torches.light_speed = std::max(parse_float(*value, 1.0F), 0.0F);
        } else if (arg == "--lights") {
            // Сцена факелов без ведущего: те же эмиттеры и те же движущиеся
            // источники, свободная камера, без выхода по счётчику кадров.
            opts.scene               = "torches";
            opts.torches.free_camera = true;
        } else if (arg == "--bench-inert") {
            // Контрольный прогон: те же правки, та же геометрия, блок который
            // не светит. Разница двух прогонов и есть цена света.
            opts.light.inert = true;
        } else if ((value = option_value(arg, "--mesh-workers"))) {
            bench.mesh_workers = parse_uint(*value, 0);
        } else if ((value = option_value(arg, "--terrain-workers"))) {
            bench.terrain_workers = parse_uint(*value, 0);
        } else if (arg == "--chunk-cull") {
            opts.stand.chunk_cull = true;
        } else if (arg == "--sun") {
            opts.stand.sun_in_bench = true;
        }
    }
}

}  // namespace

auto main(int argc, char** argv) -> int {
    gfx::bench_config bench;
    testbed::testbed_options opts;

    parse(argc, argv, bench, opts);

    // Сцена по умолчанию — облёт: он и был умолчанием, когда имя не совпадало
    // ни с чем. Разница в том, что теперь несовпадение — это ошибка, а не тихо
    // другая сцена: опечатка в имени стоила целого прогона, который мерил не то.
    if (opts.scene.empty()) {
        opts.scene = "flythrough";
    }

    auto factory = testbed::find_scene(opts.scene, opts);
    if (!factory) {
        std::string known;
        for (const auto name : testbed::scene_names()) {
            if (!known.empty()) {
                known += ", ";
            }
            known += name;
        }
        log::error("unknown scene '{}'; known scenes are: {}", opts.scene, known);
        return 1;
    }

    try {
        log::add_file_sink("testbed.log");
        std::make_unique<gfx::engine>(1280, 720, "Voxel World - Testbed", std::move(bench))
            ->run<testbed::testbed_app>(std::move(opts), *factory);
    } catch (const std::exception& e) {
        log::error("Error: {}", e.what());
        return 1;
    }

    return 0;
}
