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

enum class tools : uint8 {
    invalid,
    add_voxel,
    remove_voxel,
    paint_voxel,
};

struct ui_state {
    float left_top_voffset    = 0.f;
    float left_bottom_voffset = 0.f;
    float right_top_voffset   = 0.f;

    bool need_startup_modal   = true;
    bool need_new_file_modal  = false;
    bool need_open_file_modal = false;
    bool need_save_as_modal   = false;

    float bottom_panel_height   = 200.f;
    bool show_timeline          = false;
    bool need_create_clip_modal = false;
    bool need_save_clip         = false;
    bool need_load_clip_modal   = false;
    bool need_close_clip        = false;
};

struct app_state {
    static constexpr std::string_view asset_dir_name = "models";

    using entity_guard_type = gfx::entity_guard<>;

    ui_state ui;

    bool has_unsaved_changes = false;
    std::unordered_map<std::string, bool> unsaved_clips;

    [[nodiscard]] auto has_unsaved_clip(const std::string& name) const -> bool;
    [[nodiscard]] auto has_any_unsaved_clip() const -> bool;

    std::string filename;

    tools selected_tool  = tools::add_voxel;
    color selected_color = colors::white;

    std::string selected_name;
    std::string root_name;
    std::unordered_map<std::string, gfx::entity> name_to_entity;
    std::unordered_map<gfx::entity, std::string> entity_to_name;

    std::vector<std::unique_ptr<entity_guard_type>> entities;

    auto find_guard(gfx::entity ent) -> entity_guard_type*;

    std::string selected_clip_name;
    std::string selected_track_name;
    gfx::animation_property selected_property = gfx::animation_property::position;

    float32 selected_keyframe_time = -1.f;
    float32 timeline_cursor        = 0.f;
    std::unordered_set<std::string> expanded_tracks;
    bool is_previewing = false;

    bool need_toggle_playback = false;
    bool need_stop_playback   = false;
    bool need_add_keyframe    = false;
    bool need_delete_keyframe = false;
    bool need_step_forward    = false;
    bool need_step_backward   = false;

    std::unordered_map<std::string, transform> saved_transforms;
    bool has_saved_transforms = false;

    struct socket_preview {
        std::string filename;
        std::string preview_root_name;
        std::vector<std::unique_ptr<entity_guard_type>> guards;
    };

    std::unordered_map<std::string, socket_preview> socket_previews;
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
