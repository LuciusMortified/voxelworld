#pragma once

#ifndef VW_GFX_VOXA_DESERIALIZER_INL_H
#define VW_GFX_VOXA_DESERIALIZER_INL_H

#include <fstream>
#include <sstream>

#include "vw/log/logger.h"

namespace vw::gfx {

namespace detail {
inline constexpr log::log_category voxa_deserializer_lc{"voxa_deserializer"};
}  // namespace detail

inline auto voxa_deserializer::deserialize(
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

inline void voxa_deserializer::process_clip_(std::istringstream& iss) {
    std::string name;
    iss >> name;
    if (iss.fail()) {
        error_ = error_type::parse_error;
        return;
    }
    clip_ = std::make_shared<animation_clip>(name);
}

inline void voxa_deserializer::process_track_(std::istringstream& iss) {
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

inline void voxa_deserializer::process_channel_(std::istringstream& iss) {
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

inline void voxa_deserializer::process_keyframe_(std::istringstream& iss) {
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

inline void voxa_deserializer::finalize_channel_() {
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

inline void voxa_deserializer::finalize_track_() {
    if (!current_track_ || !clip_) {
        return;
    }

    clip_->add_track(std::move(*current_track_));
    current_track_ = nullptr;
}

inline auto voxa_deserializer::string_to_interp_(
    const std::string& s
) -> math::interpolation_type {
    if (s == "step") return math::interpolation_type::step;
    if (s == "ease_in") return math::interpolation_type::ease_in;
    if (s == "ease_out") return math::interpolation_type::ease_out;
    if (s == "ease_in_out") return math::interpolation_type::ease_in_out;
    if (s == "cubic_bezier") return math::interpolation_type::cubic_bezier;
    return math::interpolation_type::linear;
}

}  // namespace vw::gfx

#endif  // VW_GFX_VOXA_DESERIALIZER_INL_H
