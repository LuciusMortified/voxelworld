#pragma once

#ifndef VW_ECS_VOX_SERIALIZER_INL_H
#define VW_ECS_VOX_SERIALIZER_INL_H

#include <deque>
#include <format>

namespace vw::ecs {

inline vox_serializer::vox_serializer(
    world_type& world, vox_writer& writer, entity root, options opts
) : world_(&world), writer_(&writer), root_(root), excluded_(std::move(opts.excluded)) {
    if (opts.entity_names.has_value()) {
        entity_names_ = std::move(opts.entity_names.value());
    } else {
        generate_entity_names_();
    }
}

inline auto vox_serializer::serialize(
    const std::filesystem::path& filepath
) -> std::expected<void, error_type> {
    auto prefab = extract();
    return writer_->write(filepath, prefab);
}

inline auto vox_serializer::extract() const -> vw::asset::vox_prefab_data {
    vw::asset::vox_prefab_data prefab;
    prefab.root_name = entity_names_.at(root_);

    std::deque<entity> to_process;
    to_process.push_back(root_);

    while (!to_process.empty()) {
        entity current = to_process.front();
        to_process.pop_front();

        if (excluded_.contains(current)) {
            continue;
        }

        bool can_be_serialized =
            world_->has<hierarchy_component>(current) &&
            world_->has<transform_component>(current) &&
            world_->has<spatial_component>(current);

        if (can_be_serialized) {
            prefab.entities.push_back(extract_entity_(current));
        }

        if (world_->has<hierarchy_component>(current)) {
            auto& hierarchy = world_->get<hierarchy_component>(current);
            for (const auto& child : hierarchy.get_children()) {
                to_process.push_back(child);
            }
        }
    }

    return prefab;
}

inline void vox_serializer::generate_entity_names_() {
    std::deque<entity> to_process;
    to_process.push_back(root_);
    entity_names_[root_] = "root";

    while (!to_process.empty()) {
        entity current = to_process.front();
        to_process.pop_front();

        if (excluded_.contains(current)) {
            continue;
        }

        if (!entity_names_.contains(current)) {
            std::string name = std::format("child_{}", entity_names_.size());
            entity_names_[current] = name;
        }

        if (world_->has<hierarchy_component>(current)) {
            auto& hierarchy = world_->get<hierarchy_component>(current);
            for (const auto& child : hierarchy.get_children()) {
                to_process.push_back(child);
            }
        }
    }
}

inline auto vox_serializer::extract_entity_(entity ent) const -> vw::asset::vox_entity_data {
    auto& hierarchy_comp = world_->get<hierarchy_component>(ent);
    auto& transform_comp = world_->get<transform_component>(ent);

    vw::asset::vox_entity_data data;
    data.name = entity_names_.at(ent);

    if (hierarchy_comp.has_parent()) {
        entity parent = hierarchy_comp.get_parent();
        data.parent_name = entity_names_.at(parent);
    }

    data.position = transform_comp.get_position();
    data.rotation = transform_comp.get_rotation_euler();
    data.scale = transform_comp.get_scale();
    data.origin = transform_comp.get_origin();
    data.has_transform = true;

    if (world_->has<animation_target_component>(ent)) {
        auto& target = world_->get<animation_target_component>(ent);
        data.animation_target_name = target.get_name();
    }

    if (world_->has<socket_component>(ent)) {
        auto& socket_comp = world_->get<socket_component>(ent);
        data.has_sockets = true;
        for (const auto& sp : socket_comp.get_sockets()) {
            auto rot_euler = math::quat_to_euler(sp.rotation);
            data.sockets.push_back({sp.name, sp.position, rot_euler, sp.scale});
        }
    }

    if (world_->has<model_component>(ent)) {
        auto& model_comp = world_->get<model_component>(ent);
        auto size = model_comp.size();

        vw::asset::vox_model_data model_data;
        model_data.size = size;

        for (int32 z = 0; z < size.z; ++z) {
            for (int32 y = 0; y < size.y; ++y) {
                for (int32 x = 0; x < size.x; ++x) {
                    voxel v = model_comp.get_voxel(x, y, z);
                    if (v.is_empty()) {
                        continue;
                    }
                    model_data.voxels.emplace_back(vec3i{x, y, z}, v);
                }
            }
        }

        data.model = std::move(model_data);
    }

    return data;
}

}  // namespace vw::ecs

#endif  // VW_ECS_VOX_SERIALIZER_INL_H
