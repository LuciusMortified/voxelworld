#pragma once

#ifndef VW_GFX_VOX_DESERIALIZER_INL_H
#define VW_GFX_VOX_DESERIALIZER_INL_H
#include <iso646.h>

#include <fstream>

namespace vw::gfx {

template <typename WC>
vox_deserializer<WC>::vox_deserializer(
    world_type& world
)
    : world_(&world) {}

template <typename WC>
auto vox_deserializer<WC>::deserialize(
    const std::filesystem::path& filepath
) -> std::optional<result> {
    std::ifstream file(filepath);
    if (!file.is_open()) {
        return std::nullopt;
    }

    std::string line;
    while (std::getline(file, line)) {
        std::istringstream iss(line);
        std::string cmd;
        iss >> cmd;

        if (cmd == "root") {
            process_root_(iss);
        }
        if (cmd == "entity") {
            process_entity_(iss);
        }
        if (cmd == "parent") {
            process_parent_(iss);
        }
        if (cmd == "t") {
            process_transform_(iss);
        }
        if (cmd == "m") {
            process_model_(iss);
        }
        if (cmd == "v") {
            process_voxel_(iss);
        }
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

    auto& transform_system = world_->get_transform_system();
    transform_system.modify(current_entity_)
        .set_position(position)
        .set_rotation(rotation)
        .set_scale(scale)
        .set_origin(origin);
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

    std::string color_str;
    iss >> color_str;

    uint32 color_value = std::stoul(color_str, nullptr, 16);

    current_model_->set_voxel(position, color{color_value});
}

}  // namespace vw::gfx

#endif  // VW_GFX_VOX_DESERIALIZER_INL_H
