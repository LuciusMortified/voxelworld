import std;

import vw.core;
import vw.gfx;
import vw.testbed;

using namespace vw;

namespace {

// Что нужно знать до того, как что-либо построено: сколько кадров мерить, куда
// писать отчёт и какой глубины делать очереди. Всё остальное читают те, кому оно
// принадлежит: стенд — про стенд, сцена — про сцену.
auto bench_from(const testbed::arg_reader& args) -> gfx::bench_config {
    gfx::bench_config bench;

    if (!args.text("--bench")) {
        return bench;
    }

    bench.measure_frames      = args.count("--bench-frames", 2000);
    bench.warmup_frames       = args.count("--bench-warmup", 200);
    bench.fixed_delta_seconds = 1.0f / 60.0f;
    bench.mesh_workers        = args.count("--mesh-workers", 0);
    bench.terrain_workers     = args.count("--terrain-workers", 0);

    if (const auto path = args.text("--bench-out")) {
        bench.report_path = std::string{*path};
    }
    if (const auto path = args.text("--bench-json")) {
        bench.json_path = std::string{*path};
    }

    return bench;
}

// Какую сцену открыть. --bench=<имя> называет её прямо; --lights и --blobs — те
// же сцены, только чтобы смотреть глазами: без камеры на рельсах и без выхода по
// счётчику кадров.
auto scene_name(const testbed::arg_reader& args) -> std::string_view {
    if (const auto named = args.text("--bench")) {
        return *named;
    }
    if (args.flag("--lights")) {
        return "torches";
    }
    if (args.flag("--blobs")) {
        return "blobs";
    }
    return "flythrough";
}

}  // namespace

auto main(int argc, char** argv) -> int {
    const testbed::arg_reader args{argc, argv};

    const auto name = scene_name(args);

    // Имя, не совпавшее ни с одним известным, — ошибка со списком. Раньше оно
    // молча давало flythrough, и опечатка была не ошибкой, а другой сценой:
    // прогон проходил целиком и мерил не то, что просили.
    auto factory = testbed::find_scene(name, args);
    if (!factory) {
        std::string known;
        for (const auto scene : testbed::scene_names()) {
            if (!known.empty()) {
                known += ", ";
            }
            known += scene;
        }
        log::error("unknown scene '{}'; known scenes are: {}", name, known);
        return 1;
    }

    try {
        log::add_file_sink("testbed.log");
        std::make_unique<gfx::engine>(1280, 720, "Voxel World - Testbed", bench_from(args))
            ->run<testbed::testbed_app>(args, *factory);
    } catch (const std::exception& e) {
        log::error("Error: {}", e.what());
        return 1;
    }

    return 0;
}
