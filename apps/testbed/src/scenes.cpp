module vw.testbed;

import std;
import vw.core;

namespace vw::testbed {
namespace {

// Таблица сцен: имя, которое принимает --bench, и то, как построить сцену на
// уже стоящем стенде.
struct scene_entry {
    std::string_view name;
    auto (*build)(testbed_app&, const arg_reader&) -> std::unique_ptr<scene>;
};

// Сцена строится из стенда и командной строки: свои ключи она читает сама, и
// таблица о них ничего не знает.
template <typename Scene>
constexpr auto entry_for(std::string_view name) -> scene_entry {
    return {name, [](testbed_app& stand, const arg_reader& args) -> std::unique_ptr<scene> {
                return std::make_unique<Scene>(stand, args);
            }};
}

const std::array<scene_entry, 10> scene_table{{
    entry_for<parked_scene>("parked"),
    entry_for<spin_scene>("spin"),
    entry_for<advance_scene>("advance"),
    entry_for<flythrough_scene>("flythrough"),
    entry_for<dig_scene>("dig"),
    entry_for<light_scene>("light"),
    entry_for<torches_scene>("torches"),
    entry_for<village_scene>("village"),
    entry_for<blobs_scene>("blobs"),
    entry_for<crowd_scene>("crowd"),
}};

}  // namespace

auto find_scene(
    std::string_view name, const arg_reader& args
) -> std::optional<scene_factory> {
    const auto found = std::ranges::find(scene_table, name, &scene_entry::name);
    if (found == scene_table.end()) {
        return std::nullopt;
    }

    return scene_factory{[build = found->build, args](testbed_app& stand) {
        return build(stand, args);
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
