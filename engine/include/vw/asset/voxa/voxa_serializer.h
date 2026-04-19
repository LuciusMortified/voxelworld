#pragma once

#ifndef VW_ASSET_VOXA_SERIALIZER_H
#define VW_ASSET_VOXA_SERIALIZER_H

#include <expected>
#include <filesystem>
#include <string_view>

#include "vw/asset/animation/animation_clip.h"

namespace vw::asset {

inline constexpr std::string_view voxa_file_version = "1.0";

class voxa_serializer final {
public:
    enum class error_type { file_open_failed, write_failed };

    explicit voxa_serializer(const animation_clip& clip);
    auto serialize(const std::filesystem::path& filepath) -> std::expected<void, error_type>;

private:
    void write_header_(std::ofstream& file);
    void write_track_(std::ofstream& file, const animation_track& track);
    void write_channel_(std::ofstream& file, const animation_channel_variant& channel);
    void write_keyframes_vec3f_(std::ofstream& file, const animation_channel<vec3f>& ch);
    void write_keyframes_quat_(std::ofstream& file, const animation_channel<quat>& ch);

    static auto interp_to_string_(math::interpolation_type interp) -> std::string_view;

    const animation_clip* clip_;
};

}  // namespace vw::asset

#include "voxa_serializer.inl.h"

#endif  // VW_ASSET_VOXA_SERIALIZER_H
