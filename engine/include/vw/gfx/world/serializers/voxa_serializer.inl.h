#pragma once

#ifndef VW_GFX_VOXA_SERIALIZER_INL_H
#define VW_GFX_VOXA_SERIALIZER_INL_H

#include <format>
#include <fstream>

#include "vw/log/logger.h"

namespace vw::gfx {

namespace detail {
inline constexpr log::log_category voxa_serializer_lc{"voxa_serializer"};
}  // namespace detail

inline voxa_serializer::voxa_serializer(const animation_clip& clip) : clip_(&clip) {}

inline auto voxa_serializer::serialize(
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

inline void voxa_serializer::write_header_(std::ofstream& file) {
    file << std::format("# Voxa File Version {}\n", voxa_file_version);
    file << std::format("clip {}\n", clip_->get_name());
}

inline void voxa_serializer::write_track_(
    std::ofstream& file, const animation_track& track
) {
    file << std::format("\ntrack {} {:.6g}\n", track.get_target_name(), track.get_fps());

    for (const auto& channel : track.get_channels()) {
        write_channel_(file, channel);
    }
}

inline void voxa_serializer::write_channel_(
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

inline void voxa_serializer::write_keyframes_vec3f_(
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

inline void voxa_serializer::write_keyframes_quat_(
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

inline auto voxa_serializer::interp_to_string_(
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

}  // namespace vw::gfx

#endif  // VW_GFX_VOXA_SERIALIZER_INL_H
