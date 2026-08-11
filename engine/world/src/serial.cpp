module vw.world;


import std;
import :logging;


namespace vw::asset {

namespace detail {
constexpr log::log_category vox_parser_plain_lc{"vox_parser_plain"};
}  // namespace detail

vox_parser_plain::vox_parser_plain(const block_registry& block_registry)
    : block_registry_(&block_registry) {}

auto vox_parser_plain::parse(const std::filesystem::path& filepath)
    -> std::expected<vox_prefab_data, error_type> {
    prefab_ = {};
    current_entity_ = nullptr;
    error_ = std::nullopt;

    std::ifstream file(filepath);
    if (!file.is_open()) {
        log::warn(detail::vox_parser_plain_lc, "failed to open file: {}", filepath.string());
        return std::unexpected(error_type::file_open_failed);
    }

    std::string line;
    while (std::getline(file, line)) {
        if (error_.has_value()) break;

        std::istringstream iss(line);
        std::string cmd;
        iss >> cmd;

        if (cmd.empty() || cmd[0] == '#') {
            continue;
        }

        if (cmd == "root") {
            process_root_(iss);
        } else if (cmd == "entity") {
            process_entity_(iss);
        } else if (cmd == "parent") {
            process_parent_(iss);
        } else if (cmd == "t") {
            process_transform_(iss);
        } else if (cmd == "target") {
            process_target_(iss);
        } else if (cmd == "sockets") {
            process_sockets_();
        } else if (cmd == "socket") {
            process_socket_(iss);
        } else if (cmd == "m") {
            process_model_(iss);
        } else if (cmd == "v") {
            process_voxel_(iss);
        } else {
            log::warn(detail::vox_parser_plain_lc, "unknown command: {}", cmd);
        }
    }

    if (error_.has_value()) {
        log::warn(detail::vox_parser_plain_lc, "parse error in file: {}", filepath.string());
        return std::unexpected(*error_);
    }

    return std::move(prefab_);
}

void vox_parser_plain::process_root_(std::istringstream& iss) {
    std::string name;
    iss >> name;
    if (iss.fail()) {
        error_ = error_type::parse_error;
        return;
    }

    prefab_.root_name = name;
}

void vox_parser_plain::process_entity_(std::istringstream& iss) {
    std::string name;
    iss >> name;
    if (iss.fail()) {
        error_ = error_type::parse_error;
        return;
    }

    prefab_.entities.emplace_back();
    current_entity_ = &prefab_.entities.back();
    current_entity_->name = name;
}

void vox_parser_plain::process_parent_(std::istringstream& iss) {
    if (!current_entity_) {
        return;
    }

    std::string parent_name;
    iss >> parent_name;
    if (iss.fail()) {
        error_ = error_type::parse_error;
        return;
    }

    current_entity_->parent_name = parent_name;
}

void vox_parser_plain::process_transform_(std::istringstream& iss) {
    if (!current_entity_) {
        return;
    }

    vec3f position;
    iss >> position.x >> position.y >> position.z;

    vec3f rotation;
    iss >> rotation.x >> rotation.y >> rotation.z;

    vec3f scale;
    iss >> scale.x >> scale.y >> scale.z;

    vec3f origin;
    iss >> origin.x >> origin.y >> origin.z;

    if (iss.fail()) {
        error_ = error_type::parse_error;
        return;
    }

    current_entity_->position = position;
    current_entity_->rotation = rotation;
    current_entity_->scale = scale;
    current_entity_->origin = origin;
    current_entity_->has_transform = true;
}

void vox_parser_plain::process_target_(std::istringstream& iss) {
    if (!current_entity_) {
        return;
    }

    std::string target_name;
    iss >> target_name;
    if (iss.fail()) {
        error_ = error_type::parse_error;
        return;
    }

    current_entity_->animation_target_name = target_name;
}

void vox_parser_plain::process_sockets_() {
    if (!current_entity_) {
        return;
    }

    current_entity_->has_sockets = true;
}

void vox_parser_plain::process_socket_(std::istringstream& iss) {
    if (!current_entity_) {
        return;
    }

    std::string name;
    iss >> name;

    vec3f position;
    iss >> position.x >> position.y >> position.z;

    vec3f rotation;
    iss >> rotation.x >> rotation.y >> rotation.z;

    vec3f scale;
    iss >> scale.x >> scale.y >> scale.z;

    if (iss.fail()) {
        error_ = error_type::parse_error;
        return;
    }

    current_entity_->sockets.push_back({name, position, rotation, scale});
}

void vox_parser_plain::process_model_(std::istringstream& iss) {
    if (!current_entity_) {
        return;
    }

    vec3i size;
    iss >> size.x >> size.y >> size.z;
    if (iss.fail()) {
        error_ = error_type::parse_error;
        return;
    }

    current_entity_->model = vox_model_data{size, {}};
}

void vox_parser_plain::process_voxel_(std::istringstream& iss) {
    if (!current_entity_ || !current_entity_->model.has_value()) {
        return;
    }

    vec3i position;
    iss >> position.x >> position.y >> position.z;
    if (iss.fail()) {
        error_ = error_type::parse_error;
        return;
    }

    std::string color_str;
    iss >> color_str;
    if (iss.fail()) {
        error_ = error_type::parse_error;
        return;
    }

    std::string_view sv = color_str;
    if (sv.starts_with("0x") || sv.starts_with("0X")) {
        sv.remove_prefix(2);
    }

    uint32 color_value = 0;
    auto [ptr, ec] = std::from_chars(sv.data(), sv.data() + sv.size(), color_value, 16);
    if (ec != std::errc{}) {
        log::warn(detail::vox_parser_plain_lc, "invalid color value: {}", color_str);
        error_ = error_type::parse_error;
        return;
    }

    block_id bid = blocks::air;
    if (color_value > 0xFF) {
        bid = block_registry_->find_by_color(color{color_value});
    } else {
        bid = block_id{static_cast<uint8>(color_value)};
    }

    current_entity_->model->voxels.emplace_back(position, voxel{bid});
}

}  // namespace vw::asset


namespace vw::asset {

namespace detail {
constexpr log::log_category voxa_serializer_lc{"voxa_serializer"};
}  // namespace detail

voxa_serializer::voxa_serializer(const animation_clip& clip) : clip_(&clip) {}

auto voxa_serializer::serialize(
    const std::filesystem::path& filepath
) -> std::expected<void, error_type> {
    std::ofstream file(filepath.string(), std::ios::trunc);
    if (!file.is_open()) {
        log::warn(detail::voxa_serializer_lc, "failed to open file for writing: {}", filepath.string());
        return std::unexpected(error_type::file_open_failed);
    }

    write_header_(file);

    for (const auto& track : clip_->get_tracks()) {
        write_track_(file, track);
    }

    if (!file.good()) {
        log::warn(detail::voxa_serializer_lc, "write error for file: {}", filepath.string());
        return std::unexpected(error_type::write_failed);
    }

    return {};
}

void voxa_serializer::write_header_(std::ofstream& file) {
    file << std::format("# Voxa File Version {}\n", voxa_file_version);
    file << std::format("clip {}\n", clip_->get_name());
}

void voxa_serializer::write_track_(
    std::ofstream& file, const animation_track& track
) {
    file << std::format("\ntrack {} {:.6g}\n", track.get_target_name(), track.get_fps());

    for (const auto& channel : track.get_channels()) {
        write_channel_(file, channel);
    }
}

void voxa_serializer::write_channel_(
    std::ofstream& file, const animation_channel_variant& channel
) {
    std::visit(
        [&](const auto& ch) {
            std::string_view prop_name;
            switch (ch.get_property()) {
                case animation_property::position: prop_name = "position"; break;
                case animation_property::rotation: prop_name = "rotation"; break;
                case animation_property::scale: prop_name = "scale"; break;
                case animation_property::origin: prop_name = "origin"; break;
            }
            file << std::format("  channel {}\n", prop_name);

            using channel_type = std::decay_t<decltype(ch)>;
            if constexpr (std::is_same_v<channel_type, animation_channel<vec3f>>) {
                write_keyframes_vec3f_(file, ch);
            } else if constexpr (std::is_same_v<channel_type, animation_channel<quat>>) {
                write_keyframes_quat_(file, ch);
            }
        },
        channel
    );
}

void voxa_serializer::write_keyframes_vec3f_(
    std::ofstream& file, const animation_channel<vec3f>& ch
) {
    for (const auto& kf : ch.get_keyframes()) {
        file << std::format(
            "    k {:.6g} {:.6g} {:.6g} {:.6g} {} {:.6g} {:.6g}\n",
            kf.time, kf.value.x, kf.value.y, kf.value.z,
            interp_to_string_(kf.interp), kf.tangent_in, kf.tangent_out
        );
    }
}

void voxa_serializer::write_keyframes_quat_(
    std::ofstream& file, const animation_channel<quat>& ch
) {
    for (const auto& kf : ch.get_keyframes()) {
        file << std::format(
            "    k {:.6g} {:.6g} {:.6g} {:.6g} {:.6g} {} {:.6g} {:.6g}\n",
            kf.time, kf.value.x, kf.value.y, kf.value.z, kf.value.w,
            interp_to_string_(kf.interp), kf.tangent_in, kf.tangent_out
        );
    }
}

auto voxa_serializer::interp_to_string_(
    math::interpolation_type interp
) -> std::string_view {
    switch (interp) {
        case math::interpolation_type::linear: return "linear";
        case math::interpolation_type::step: return "step";
        case math::interpolation_type::ease_in: return "ease_in";
        case math::interpolation_type::ease_out: return "ease_out";
        case math::interpolation_type::ease_in_out: return "ease_in_out";
        case math::interpolation_type::cubic_bezier: return "cubic_bezier";
        default: return "linear";
    }
}

}  // namespace vw::asset


namespace vw::asset {

namespace detail {
constexpr log::log_category voxa_deserializer_lc{"voxa_deserializer"};
}  // namespace detail

auto voxa_deserializer::deserialize(
    const std::filesystem::path& filepath
) -> std::expected<std::shared_ptr<animation_clip>, error_type> {
    std::ifstream file(filepath);
    if (!file.is_open()) {
        log::warn(detail::voxa_deserializer_lc, "failed to open file: {}", filepath.string());
        return std::unexpected(error_type::file_open_failed);
    }

    clip_ = nullptr;
    current_track_ = nullptr;
    has_current_channel_ = false;
    error_ = std::nullopt;
    vec3f_keyframes_.clear();
    quat_keyframes_.clear();

    std::string line;
    while (std::getline(file, line)) {
        if (error_.has_value()) break;

        std::istringstream iss(line);
        std::string cmd;
        iss >> cmd;

        if (cmd.empty() || cmd[0] == '#') {
            continue;
        }

        if (cmd == "clip") {
            process_clip_(iss);
        } else if (cmd == "track") {
            process_track_(iss);
        } else if (cmd == "channel") {
            process_channel_(iss);
        } else if (cmd == "k") {
            process_keyframe_(iss);
        } else {
            log::warn(detail::voxa_deserializer_lc, "unknown command: {}", cmd);
        }
    }

    if (error_.has_value()) {
        log::warn(detail::voxa_deserializer_lc, "parse error in file: {}", filepath.string());
        return std::unexpected(*error_);
    }

    finalize_channel_();
    finalize_track_();

    return clip_;
}

void voxa_deserializer::process_clip_(std::istringstream& iss) {
    std::string name;
    iss >> name;
    if (iss.fail()) {
        error_ = error_type::parse_error;
        return;
    }
    clip_ = std::make_shared<animation_clip>(name);
}

void voxa_deserializer::process_track_(std::istringstream& iss) {
    finalize_channel_();
    finalize_track_();

    std::string target_name;
    float32 fps = 60.0f;
    iss >> target_name >> fps;
    if (iss.fail()) {
        error_ = error_type::parse_error;
        return;
    }

    current_track_ = std::make_unique<animation_track>(target_name, fps);
}

void voxa_deserializer::process_channel_(std::istringstream& iss) {
    finalize_channel_();

    std::string prop_name;
    iss >> prop_name;
    if (iss.fail()) {
        error_ = error_type::parse_error;
        return;
    }

    if (prop_name == "position") {
        current_property_ = animation_property::position;
        current_channel_is_quat_ = false;
    } else if (prop_name == "rotation") {
        current_property_ = animation_property::rotation;
        current_channel_is_quat_ = true;
    } else if (prop_name == "scale") {
        current_property_ = animation_property::scale;
        current_channel_is_quat_ = false;
    } else if (prop_name == "origin") {
        current_property_ = animation_property::origin;
        current_channel_is_quat_ = false;
    }

    has_current_channel_ = true;
}

void voxa_deserializer::process_keyframe_(std::istringstream& iss) {
    float32 time;
    iss >> time;

    if (current_channel_is_quat_) {
        keyframe<quat> kf;
        kf.time = time;
        std::string interp_str;
        iss >> kf.value.x >> kf.value.y >> kf.value.z >> kf.value.w;
        iss >> interp_str >> kf.tangent_in >> kf.tangent_out;
        if (iss.fail()) {
            error_ = error_type::parse_error;
            return;
        }
        kf.interp = string_to_interp_(interp_str);
        quat_keyframes_.push_back(kf);
    } else {
        keyframe<vec3f> kf;
        kf.time = time;
        std::string interp_str;
        iss >> kf.value.x >> kf.value.y >> kf.value.z;
        iss >> interp_str >> kf.tangent_in >> kf.tangent_out;
        if (iss.fail()) {
            error_ = error_type::parse_error;
            return;
        }
        kf.interp = string_to_interp_(interp_str);
        vec3f_keyframes_.push_back(kf);
    }
}

void voxa_deserializer::finalize_channel_() {
    if (!has_current_channel_ || !current_track_) {
        return;
    }

    if (current_channel_is_quat_) {
        animation_channel<quat> ch(current_property_);
        ch.set_keyframes(std::move(quat_keyframes_));
        current_track_->add<animation_property::rotation>(std::move(ch));
        quat_keyframes_.clear();
    } else {
        animation_channel<vec3f> ch(current_property_);
        ch.set_keyframes(std::move(vec3f_keyframes_));
        switch (current_property_) {
            case animation_property::position:
                current_track_->add<animation_property::position>(std::move(ch));
                break;
            case animation_property::scale:
                current_track_->add<animation_property::scale>(std::move(ch));
                break;
            case animation_property::origin:
                current_track_->add<animation_property::origin>(std::move(ch));
                break;
            default: break;
        }
        vec3f_keyframes_.clear();
    }

    has_current_channel_ = false;
}

void voxa_deserializer::finalize_track_() {
    if (!current_track_ || !clip_) {
        return;
    }

    clip_->add_track(std::move(*current_track_));
    current_track_ = nullptr;
}

auto voxa_deserializer::string_to_interp_(
    const std::string& s
) -> math::interpolation_type {
    if (s == "step") return math::interpolation_type::step;
    if (s == "ease_in") return math::interpolation_type::ease_in;
    if (s == "ease_out") return math::interpolation_type::ease_out;
    if (s == "ease_in_out") return math::interpolation_type::ease_in_out;
    if (s == "cubic_bezier") return math::interpolation_type::cubic_bezier;
    return math::interpolation_type::linear;
}

}  // namespace vw::asset


namespace vw::asset {

namespace detail {
constexpr log::log_category asset_storage_lc{"asset_storage"};
}  // namespace detail

asset_storage::asset_storage(vox_parser& parser, model_registry& registry)
    : parser_(&parser), model_registry_(&registry) {}

void asset_storage::load_prefab(
    std::string_view name, const std::filesystem::path& filepath
) {
    auto result = parser_->parse(filepath);
    if (!result.has_value()) {
        log::warn(detail::asset_storage_lc, "failed to load prefab '{}': {}", name, filepath.string());
        return;
    }

    std::string name_str(name);

    for (auto& ent : result->entities) {
        if (!ent.model.has_value()) {
            continue;
        }

        std::string model_key = name_str + "/" + ent.name;
        auto m = model_registry_->create(model_key, ent.model->size);

        for (const auto& [pos, v] : ent.model->voxels) {
            m->set_voxel(pos, v);
        }

        models_[model_key] = std::move(m);
        ent.model = std::nullopt;
    }

    prefabs_[name_str] = std::move(*result);
}

void asset_storage::load_clip(
    std::string_view name, const std::filesystem::path& filepath
) {
    voxa_deserializer deserializer;
    auto result = deserializer.deserialize(filepath);
    if (!result.has_value()) {
        log::warn(detail::asset_storage_lc, "failed to load clip '{}': {}", name, filepath.string());
        return;
    }

    clips_[std::string(name)] = std::move(*result);
}

auto asset_storage::get_entity(
    std::string_view prefab, std::string_view entity_name
) const -> const vox_entity_data& {
    auto pit = prefabs_.find(std::string(prefab));
    for (const auto& ent : pit->second.entities) {
        if (ent.name == entity_name) {
            return ent;
        }
    }

    log::warn(detail::asset_storage_lc, "entity '{}' not found in prefab '{}'", entity_name, prefab);
    return pit->second.entities.front();
}

auto asset_storage::get_model(
    std::string_view prefab, std::string_view entity_name
) const -> std::shared_ptr<model> {
    std::string key = std::string(prefab) + "/" + std::string(entity_name);
    auto it = models_.find(key);
    if (it != models_.end()) {
        return it->second;
    }
    return nullptr;
}

auto asset_storage::get_clip(std::string_view name) const
    -> std::shared_ptr<animation_clip> {
    auto it = clips_.find(std::string(name));
    if (it != clips_.end()) {
        return it->second;
    }
    return nullptr;
}

auto asset_storage::has_clip(std::string_view name) const -> bool {
    return clips_.find(std::string(name)) != clips_.end();
}

}  // namespace vw::asset


namespace vw::ecs {

namespace detail {
constexpr log::log_category vox_writer_plain_lc{"vox_writer_plain"};
}  // namespace detail

auto vox_writer_plain::write(
    const std::filesystem::path& filepath, const vw::asset::vox_prefab_data& prefab
) -> std::expected<void, error_type> {
    std::ofstream file(filepath.string(), std::ios::trunc);
    if (!file.is_open()) {
        log::warn(detail::vox_writer_plain_lc, "failed to open file for writing: {}", filepath.string());
        return std::unexpected(error_type::file_open_failed);
    }

    write_header_(file, prefab);

    for (const auto& ent : prefab.entities) {
        write_entity_(file, ent);
    }

    if (!file.good()) {
        log::warn(detail::vox_writer_plain_lc, "write error for file: {}", filepath.string());
        return std::unexpected(error_type::write_failed);
    }

    return {};
}

void vox_writer_plain::write_header_(
    std::ofstream& file, const vw::asset::vox_prefab_data& prefab
) {
    file << std::format("# Vox File Version {}\n", vox_file_version);
    file << std::format("root {}\n", prefab.root_name);
}

void vox_writer_plain::write_entity_(
    std::ofstream& file, const vw::asset::vox_entity_data& ent
) {
    file << std::format("entity {}\n", ent.name);

    if (!ent.parent_name.empty()) {
        file << std::format("\tparent {}\n", ent.parent_name);
    }

    if (ent.has_transform) {
        file << std::format(
            "\tt {} {} {}\t{} {} {}\t{} {} {}\t{} {} {}\n",
            ent.position.x, ent.position.y, ent.position.z,
            ent.rotation.x, ent.rotation.y, ent.rotation.z,
            ent.scale.x, ent.scale.y, ent.scale.z,
            ent.origin.x, ent.origin.y, ent.origin.z
        );
    }

    if (ent.animation_target_name.has_value()) {
        file << std::format("\ttarget {}\n", *ent.animation_target_name);
    }

    if (ent.has_sockets) {
        file << "\tsockets\n";
        for (const auto& sp : ent.sockets) {
            file << std::format(
                "\t\tsocket {} {} {} {} {} {} {} {} {} {}\n",
                sp.name,
                sp.position.x, sp.position.y, sp.position.z,
                sp.rotation.x, sp.rotation.y, sp.rotation.z,
                sp.scale.x, sp.scale.y, sp.scale.z
            );
        }
    }

    if (ent.model.has_value()) {
        write_model_(file, *ent.model);
    }
}

void vox_writer_plain::write_model_(
    std::ofstream& file, const vw::asset::vox_model_data& mdl
) {
    file << std::format("\tm {} {} {}\n", mdl.size.x, mdl.size.y, mdl.size.z);

    for (const auto& [pos, v] : mdl.voxels) {
        file << std::format("\t\tv {} {} {} 0x{:02X}\n", pos.x, pos.y, pos.z, v.id.value);
    }
}

}  // namespace vw::ecs

