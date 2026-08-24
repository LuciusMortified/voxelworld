module vw.testbed;

import std;
import vw.core;

namespace vw::testbed {
namespace {

// Таблица сцен: имя, которое принимает --bench, и то, как построить сцену на
// уже стоящем стенде.
struct scene_entry {
    std::string_view name;
    auto (*build)(testbed_app&, const testbed_options&) -> std::unique_ptr<scene>;
};

constexpr std::array<scene_entry, 10> scene_table{{
    {"parked", [](testbed_app& stand, const testbed_options&) -> std::unique_ptr<scene> {
         return std::make_unique<parked_scene>(stand);
     }},
    {"spin", [](testbed_app& stand, const testbed_options&) -> std::unique_ptr<scene> {
         return std::make_unique<spin_scene>(stand);
     }},
    {"advance", [](testbed_app& stand, const testbed_options&) -> std::unique_ptr<scene> {
         return std::make_unique<advance_scene>(stand);
     }},
    {"flythrough", [](testbed_app& stand, const testbed_options&) -> std::unique_ptr<scene> {
         return std::make_unique<flythrough_scene>(stand);
     }},
    {"dig", [](testbed_app& stand, const testbed_options& opts) -> std::unique_ptr<scene> {
         return std::make_unique<dig_scene>(stand, opts.dig);
     }},
    {"light", [](testbed_app& stand, const testbed_options& opts) -> std::unique_ptr<scene> {
         return std::make_unique<light_scene>(stand, opts.light);
     }},
    {"torches", [](testbed_app& stand, const testbed_options& opts) -> std::unique_ptr<scene> {
         return std::make_unique<torches_scene>(stand, opts.torches);
     }},
    {"village", [](testbed_app& stand, const testbed_options& opts) -> std::unique_ptr<scene> {
         return std::make_unique<village_scene>(stand, opts.torches);
     }},
    {"blobs", [](testbed_app& stand, const testbed_options& opts) -> std::unique_ptr<scene> {
         return std::make_unique<blobs_scene>(stand, opts.blobs);
     }},
    {"crowd", [](testbed_app& stand, const testbed_options& opts) -> std::unique_ptr<scene> {
         return std::make_unique<crowd_scene>(stand, opts.crowd);
     }},
}};

}  // namespace

auto find_scene(
    std::string_view name, const testbed_options& opts
) -> std::optional<scene_factory> {
    const auto found = std::ranges::find(scene_table, name, &scene_entry::name);
    if (found == scene_table.end()) {
        return std::nullopt;
    }

    return scene_factory{[build = found->build, opts](testbed_app& stand) {
        return build(stand, opts);
    }};
}

auto scene_names() -> std::vector<std::string_view> {
    std::vector<std::string_view> names;
    names.reserve(scene_table.size());
    for (const auto& entry : scene_table) {
        names.push_back(entry.name);
    }
    return names;
}

}  // namespace vw::testbed
