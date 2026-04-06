#pragma once

#ifndef VW_GFX_VOX_PREFAB_DATA_H
#define VW_GFX_VOX_PREFAB_DATA_H

#include <optional>
#include <string>
#include <vector>

#include "vw/core/vec3.h"
#include "vw/core/voxel.h"

namespace vw::gfx {

struct vox_socket_data {
    std::string name;
    vec3f position;
    vec3f rotation;
    vec3f scale;
};

struct vox_model_data {
    vec3i size;
    std::vector<std::pair<vec3i, voxel>> voxels;
};

struct vox_entity_data {
    std::string name;
    std::string parent_name;

    vec3f position;
    vec3f rotation;
    vec3f scale{1.f, 1.f, 1.f};
    vec3f origin;
    bool has_transform = false;

    std::optional<vox_model_data> model;
    std::optional<std::string> animation_target_name;
    std::vector<vox_socket_data> sockets;
    bool has_sockets = false;
};

struct vox_prefab_data {
    std::string root_name;
    std::vector<vox_entity_data> entities;
};

}  // namespace vw::gfx

#endif  // VW_GFX_VOX_PREFAB_DATA_H
