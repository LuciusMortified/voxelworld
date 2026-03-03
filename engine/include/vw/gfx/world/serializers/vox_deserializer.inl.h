#pragma once

#ifndef VW_GFX_VOX_DESERIALIZER_INL_H
#define VW_GFX_VOX_DESERIALIZER_INL_H

#include <charconv>
#include <fstream>

#include "vw/log/logger.h"

namespace vw::gfx {

namespace detail {
inline constexpr log::log_category vox_deserializer_lc{"vox_deserializer"};
}  // namespace detail

template <typename WC>
vox_deserializer<WC>::vox_deserializer(
    world_type& world
)
    : world_(&world) {}

template <typename WC>
auto vox_deserializer<WC>::deserialize(
    const std::filesystem::path& filepath, const options& opts
) -> std::expected<result, error_type> {
    result_ = {};
    options_ = opts;
    error_ = std::nullopt;
    current_entity_ = invalid_entity;
    current_entity_guard_ = nullptr;
    current_model_ = nullptr;

    std::ifstream file(filepath);
    if (!file.is_open()) {
        log::warn(detail::vox_deserializer_lc, "failed to open file: {}", filepath.string());
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
            log::warn(detail::vox_deserializer_lc, "unknown command: {}", cmd);
        }
    }

    if (error_.has_value()) {
        log::warn(detail::vox_deserializer_lc, "parse error in file: {}", filepath.string());
        return std::unexpected(*error_);
    }

    if (current_entity_guard_) {
        result_.entities.push_back(std::move(current_entity_guard_));
    }

    return std::move(result_);
}

template <typename WC>
void vox_deserializer<WC>::process_root_(
    std::istringstream& iss
) {
    std::string name;
    iss >> name;
    if (iss.fail()) {
        error_ = error_type::parse_error;
        return;
    }

    result_.root_name = name;
}

template <typename WC>
void vox_deserializer<WC>::process_entity_(
    std::istringstream& iss
) {
    if (current_entity_guard_) {
        result_.entities.push_back(std::move(current_entity_guard_));
    }

    std::string name;
    iss >> name;
    if (iss.fail()) {
        error_ = error_type::parse_error;
        return;
    }

    auto ent_guard = std::make_unique<entity_guard<WC>>(*world_);
    ent_guard->template with<hierarchy_component>();
    ent_guard->template with<transform_component>();
    ent_guard->template with<spatial_component>();

    auto ent = ent_guard->get_entity();

    result_.name_to_entity[name] = ent;
    result_.entity_to_name[ent]  = name;

    current_entity_       = ent;
    current_entity_guard_ = std::move(ent_guard);
}

template <typename WC>
void vox_deserializer<WC>::process_parent_(
    std::istringstream& iss
) {
    if (!current_entity_.is_valid()) {
        return;
    }

    std::string parent_name;
    iss >> parent_name;
    if (iss.fail()) {
        error_ = error_type::parse_error;
        return;
    }

    if (!result_.name_to_entity.contains(parent_name)) {
        return;
    }

    auto parent_entity     = result_.name_to_entity[parent_name];
    auto& hierarchy_system = world_->get_hierarchy_system();
    hierarchy_system.modify(current_entity_).set_parent(parent_entity);
}

template <typename WC>
void vox_deserializer<WC>::process_transform_(
    std::istringstream& iss
) {
    if (!current_entity_.is_valid()) {
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

    auto& transform_system = world_->get_transform_system();
    transform_system.modify(current_entity_)
        .set_position(position)
        .set_rotation_euler(rotation)
        .set_scale(scale)
        .set_origin(origin);
}

template <typename WC>
void vox_deserializer<WC>::process_target_(
    std::istringstream& iss
) {
    if (!current_entity_.is_valid() || options_.skip_targets) {
        return;
    }

    std::string target_name;
    iss >> target_name;
    if (iss.fail()) {
        error_ = error_type::parse_error;
        return;
    }

    current_entity_guard_->template with<animation_target_component>();

    auto& animation_system = world_->get_animation_system();
    animation_system.modify_target(current_entity_).set_target_name(target_name);
}

template <typename WC>
void vox_deserializer<WC>::process_sockets_() {
    if (!current_entity_.is_valid() || options_.skip_sockets) {
        return;
    }

    current_entity_guard_->template with<socket_component>();
}

template <typename WC>
void vox_deserializer<WC>::process_socket_(
    std::istringstream& iss
) {
    if (!current_entity_.is_valid() || options_.skip_sockets) {
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

    auto& socket_system = world_->get_socket_system();
    socket_system.modify(current_entity_).add_socket(name, position, math::euler_to_quat(rotation), scale);
}

template <typename WC>
void vox_deserializer<WC>::process_model_(
    std::istringstream& iss
) {
    if (!current_entity_.is_valid()) {
        return;
    }

    vec3i size;
    iss >> size.x >> size.y >> size.z;
    if (iss.fail()) {
        error_ = error_type::parse_error;
        return;
    }

    auto ent_name = result_.entity_to_name[current_entity_];

    auto& model_registry = world_->get_model_registry();

    current_model_ = model_registry.create(ent_name, size);

    current_entity_guard_->template with<model_component>();

    auto& model_system = world_->get_model_system();
    model_system.modify(current_entity_).set_model(current_model_);
}

template <typename WC>
void vox_deserializer<WC>::process_voxel_(
    std::istringstream& iss
) {
    if (!current_entity_.is_valid() || !current_model_) {
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
        log::warn(detail::vox_deserializer_lc, "invalid color value: {}", color_str);
        error_ = error_type::parse_error;
        return;
    }

    current_model_->set_voxel(position, color{color_value});
}

}  // namespace vw::gfx

#endif  // VW_GFX_VOX_DESERIALIZER_INL_H
