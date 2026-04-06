#pragma once

#ifndef VW_SCULPTOR_FILE_SERVICE_INL_H
#define VW_SCULPTOR_FILE_SERVICE_INL_H

#include <filesystem>

#include "vw/gfx/world/serializers/vox_serializer.h"
#include "vw/gfx/world/serializers/vox_writer_plain.h"

namespace vw::sculptor {

inline file_service::file_service(
    engine_type& eng, app_state& state
)
    : engine_(&eng), state_(&state) {}

inline auto file_service::save() -> bool {
    if (state_->scene.root_name.empty() ||
        !state_->scene.name_to_entity.contains(state_->scene.root_name)) {
        return false;
    }

    gfx::vox_writer_plain writer;
    gfx::vox_serializer serializer{
        engine_->get_world(),
        writer,
        state_->scene.name_to_entity.at(state_->scene.root_name),
        {.entity_names = state_->scene.entity_to_name,
         .excluded     = state_->sockets.get_preview_entities()}
    };

    namespace fs = std::filesystem;
    const fs::path assets_dir_path{app_state::asset_dir_name};
    const fs::path filepath{assets_dir_path / state_->file.filename};

    if (!serializer.serialize(filepath)) {
        return false;
    }

    state_->file.has_unsaved_changes = false;
    return true;
}

inline auto file_service::save_as(
    const std::filesystem::path& filepath
) -> bool {
    if (state_->scene.root_name.empty() ||
        !state_->scene.name_to_entity.contains(state_->scene.root_name)) {
        return false;
    }

    gfx::vox_writer_plain writer;
    gfx::vox_serializer serializer{
        engine_->get_world(),
        writer,
        state_->scene.name_to_entity.at(state_->scene.root_name),
        {.entity_names = state_->scene.entity_to_name,
         .excluded     = state_->sockets.get_preview_entities()}
    };

    if (!serializer.serialize(filepath)) {
        return false;
    }

    state_->file.filename            = filepath.filename().string();
    state_->file.has_unsaved_changes = false;
    return true;
}


}  // namespace vw::sculptor

#endif  // VW_SCULPTOR_FILE_SERVICE_INL_H
