#pragma once

#ifndef VW_SCULPTOR_STATE_H
#define VW_SCULPTOR_STATE_H

#include <unordered_set>
#include <variant>

#include "vw/core/transform.h"
#include "vw/gfx/animation/animation_types.h"
#include "vw/gfx/animation/keyframe.h"
#include "vw/gfx/world/entity_guard.h"

namespace vw::sculptor {

using keyframe_value = std::variant<gfx::keyframe_vec3f, gfx::keyframe_quat>;
using entity_guard_type = gfx::entity_guard<>;

enum class tools : uint8 {
    invalid,
    add_voxel,
    remove_voxel,
    paint_voxel,
    color_picker,
};

struct ui_state {
    float left_top_voffset    = 0.f;
    float left_bottom_voffset = 0.f;
    float right_top_voffset   = 0.f;

    bool need_startup_modal   = true;
    bool need_new_file_modal  = false;
    bool need_open_file_modal = false;
    bool need_save_as_modal   = false;

    float bottom_panel_height   = 400.f;
    bool show_timeline          = false;
    bool show_clip_manager      = false;
    bool show_sockets           = true;
    bool need_create_clip_modal = false;
    bool need_save_clip         = false;
    bool need_load_clip_modal   = false;
    bool need_close_clip        = false;
};

struct file_state {
    std::string filename;
    bool has_unsaved_changes = false;
};

struct scene_state {
    std::string selected_name;
    std::string root_name;
    std::unordered_map<std::string, gfx::entity> name_to_entity;
    std::unordered_map<gfx::entity, std::string> entity_to_name;
    std::vector<std::unique_ptr<entity_guard_type>> entities;

    [[nodiscard]] auto find_guard(gfx::entity ent) const -> entity_guard_type*;
};

struct tool_state {
    tools selected_tool  = tools::add_voxel;
    color selected_color = colors::white;
};

struct clip_settings {
    float32 playback_speed                = 1.0f;
    gfx::animation_loop_mode loop_mode    = gfx::animation_loop_mode::once;
    gfx::transition blend_transition      = {};
    gfx::transition fade_in               = {};
    gfx::transition fade_out              = {};
};

struct animation_state {
    std::string selected_clip_name;
    std::string selected_track_name;
    gfx::animation_property selected_property = gfx::animation_property::position;

    uint32 selected_keyframe_id = gfx::invalid_keyframe_id;
    float32 timeline_cursor     = 0.f;
    std::unordered_set<std::string> expanded_tracks;
    bool animation_mode = false;

    bool need_toggle_playback = false;
    bool need_stop_playback   = false;
    bool need_add_keyframe    = false;
    bool need_delete_keyframe = false;
    bool need_step_forward    = false;
    bool need_step_backward   = false;

    std::unordered_map<std::string, bool> unsaved_clips;
    std::unordered_map<std::string, size_t> clip_to_layer;
    std::unordered_map<std::string, clip_settings> clip_settings_map;
    std::unordered_map<std::string, transform> saved_transforms;
    bool has_saved_transforms = false;

    [[nodiscard]] auto has_unsaved_clip(const std::string& name) const -> bool;
    [[nodiscard]] auto has_any_unsaved_clip() const -> bool;
    [[nodiscard]] auto get_layer_for_clip(const std::string& name) const -> size_t;
    [[nodiscard]] auto get_clip_settings(const std::string& name) const -> clip_settings;
    [[nodiscard]] auto get_clip_settings_mut(const std::string& name) -> clip_settings&;
};

struct socket_state {
    struct socket_preview {
        std::string filename;
        std::string preview_root_name;
        std::vector<std::unique_ptr<entity_guard_type>> guards;
    };

    std::unordered_map<std::string, socket_preview> socket_previews;

    [[nodiscard]] static auto socket_preview_key(
        const std::string& entity_name, const std::string& socket_name
    ) -> std::string {
        return std::format("{}:{}", entity_name, socket_name);
    }

    [[nodiscard]] auto get_preview_entities() const -> std::unordered_set<gfx::entity> {
        std::unordered_set<gfx::entity> result;
        for (const auto& preview : socket_previews | std::views::values) {
            for (const auto& guard : preview.guards) {
                result.insert(guard->get_entity());
            }
        }
        return result;
    }
};

struct app_state {
    static constexpr std::string_view asset_dir_name = "models";

    ui_state ui;
    file_state file;
    scene_state scene;
    tool_state tool;
    animation_state anim;
    socket_state sockets;
};

}  // namespace vw::sculptor

template <>
struct std::hash<vw::sculptor::tools> {
    auto operator()(
        vw::sculptor::tools t
    ) const noexcept -> size_t {
        return std::hash<vw::uint32>()(static_cast<vw::uint32>(t));
    }
};

#include "app_state.inl.h"

#endif  // VW_SCULPTOR_STATE_H
