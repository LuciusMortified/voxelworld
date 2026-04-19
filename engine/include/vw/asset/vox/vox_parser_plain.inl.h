#pragma once

#ifndef VW_ASSET_VOX_PARSER_PLAIN_INL_H
#define VW_ASSET_VOX_PARSER_PLAIN_INL_H

#include <charconv>
#include <fstream>

#include "vw/log/logger.h"

namespace vw::asset {

namespace detail {
inline constexpr log::log_category vox_parser_plain_lc{"vox_parser_plain"};
}  // namespace detail

inline vox_parser_plain::vox_parser_plain(const block_registry& block_registry)
    : block_registry_(&block_registry) {}

inline auto vox_parser_plain::parse(const std::filesystem::path& filepath)
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

inline void vox_parser_plain::process_root_(std::istringstream& iss) {
    std::string name;
    iss >> name;
    if (iss.fail()) {
        error_ = error_type::parse_error;
        return;
    }

    prefab_.root_name = name;
}

inline void vox_parser_plain::process_entity_(std::istringstream& iss) {
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

inline void vox_parser_plain::process_parent_(std::istringstream& iss) {
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

inline void vox_parser_plain::process_transform_(std::istringstream& iss) {
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

inline void vox_parser_plain::process_target_(std::istringstream& iss) {
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

inline void vox_parser_plain::process_sockets_() {
    if (!current_entity_) {
        return;
    }

    current_entity_->has_sockets = true;
}

inline void vox_parser_plain::process_socket_(std::istringstream& iss) {
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

inline void vox_parser_plain::process_model_(std::istringstream& iss) {
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

inline void vox_parser_plain::process_voxel_(std::istringstream& iss) {
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

#endif  // VW_ASSET_VOX_PARSER_PLAIN_INL_H
