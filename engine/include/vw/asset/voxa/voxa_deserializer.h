#pragma once

#ifndef VW_ASSET_VOXA_DESERIALIZER_H
#define VW_ASSET_VOXA_DESERIALIZER_H

#include <expected>
#include <filesystem>
#include <memory>
#include <optional>

#include "vw/asset/animation/animation_clip.h"

namespace vw::asset {

class voxa_deserializer final {
public:
    enum class error_type { file_open_failed, parse_error };

    auto deserialize(
        const std::filesystem::path& filepath
    ) -> std::expected<std::shared_ptr<animation_clip>, error_type>;

private:
    void process_clip_(std::istringstream& iss);
    void process_track_(std::istringstream& iss);
    void process_channel_(std::istringstream& iss);
    void process_keyframe_(std::istringstream& iss);
    void finalize_channel_();
    void finalize_track_();

    static auto string_to_interp_(const std::string& s) -> math::interpolation_type;

    std::shared_ptr<animation_clip> clip_;
    std::optional<error_type> error_;

    std::unique_ptr<animation_track> current_track_;
    animation_property current_property_ = animation_property::position;
    bool has_current_channel_ = false;
    bool current_channel_is_quat_ = false;

    std::vector<keyframe<vec3f>> vec3f_keyframes_;
    std::vector<keyframe<quat>> quat_keyframes_;
};

}  // namespace vw::asset

#include "voxa_deserializer.inl.h"

#endif  // VW_ASSET_VOXA_DESERIALIZER_H
