#pragma once

#ifndef VW_GFX_VOX_SERIALIZER_INL_H
#define VW_GFX_VOX_SERIALIZER_INL_H
#include <fstream>

namespace vw::gfx {


template <typename WC>
vox_serializer<WC>::vox_serializer(
    world_type& world, entity root, std::optional<entity_names_type> entity_names
) : world_(&world), root_(root) {
    if (entity_names.has_value()) {
        entity_names_ = std::move(entity_names.value());
    } else {
        generate_entity_names_();
    }
}

template <typename WC>
auto vox_serializer<WC>::serialize(
    const std::filesystem::path& filepath
) -> bool {
    std::ofstream file(filepath.string(), std::ios::trunc);
    if (!file.is_open()) {
        return false;
    }

    write_header_(file);

    std::deque<entity> to_process;
    to_process.push_back(root_);

    while (!to_process.empty()) {
        entity current = to_process.front();
        to_process.pop_front();

        write_entity_(file, current);

        if (world_->template has_component<hierarchy_component>(current)) {
            auto& hierarchy = world_->template get_component<hierarchy_component>(current);
            for (const auto& child : hierarchy.get_children()) {
                to_process.push_back(child);
            }
        }
    }

    return true;
}

template <typename WC>
void vox_serializer<WC>::generate_entity_names_() {
    std::deque<entity> to_process;
    to_process.push_back(root_);
    entity_names_[root_] = "root";

    while (!to_process.empty()) {
        entity current = to_process.front();
        to_process.pop_front();

        if (!entity_names_.contains(current)) {
            std::string name = std::format("child_{}", entity_names_.size());
            entity_names_[current] = name;
        }

        if (world_->template has_component<hierarchy_component>(current)) {
            auto& hierarchy = world_->template get_component<hierarchy_component>(current);
            for (const auto& child : hierarchy.get_children()) {
                to_process.push_back(child);
            }
        }
    }
}

template <typename WC>
void vox_serializer<WC>::write_header_(
    std::ofstream& file
) {
    file << std::format("# Vox File Version {}\n", vox_file_version);
    file << std::format("root {}\n", entity_names_[root_]);
}

template <typename WC>
void vox_serializer<WC>::write_entity_(
    std::ofstream& file, entity ent
) {
    bool can_be_serialized = //
        world_->template has_component<hierarchy_component>(ent) &&
        world_->template has_component<transform_component>(ent) &&
        world_->template has_component<spatial_component>(ent);
    if (!can_be_serialized) {
        return;
    }

    auto& hierarchy_comp = world_->template get_component<hierarchy_component>(ent);
    auto& transform_comp = world_->template get_component<transform_component>(ent);

    file << std::format("entity {}\n", entity_names_[ent]);

    if (hierarchy_comp.has_parent()) {
        entity parent = hierarchy_comp.get_parent();
        file << std::format("\tparent {}\n", entity_names_[parent]);
    }

    auto pos = transform_comp.get_position();
    auto rot = transform_comp.get_rotation();
    auto scl = transform_comp.get_scale();
    auto ori = transform_comp.get_origin();

    file << std::format(
        "\tt {} {} {}\t{} {} {}\t{} {} {}\t{} {} {}\n",
        pos.x, pos.y, pos.z,
        rot.x, rot.y, rot.z,
        scl.x, scl.y, scl.z,
        ori.x, ori.y, ori.z
    );

    write_model_(file, ent);
}

template <typename WC>
void vox_serializer<WC>::write_model_(
    std::ofstream& file, entity ent
) {
    bool can_be_serialized = //
        world_->template has_component<model_component>(ent);
    if (!can_be_serialized) {
        return;
    }

    auto& model_comp = world_->template get_component<model_component>(ent);

    auto size = model_comp.size();
    file << std::format("\tm {} {} {}\n", size.x, size.y, size.z);

    for (int z = 0; z < size.z; ++z) {
        for (int y = 0; y < size.y; ++y) {
            for (int x = 0; x < size.x; ++x) {
                voxel v = model_comp.get_voxel(x, y, z);
                if (v.is_empty()) {
                    continue;
                }
                file << std::format(
                    "\t\tv {} {} {} 0x{:08X}\n",
                    x, y, z, v.value.value
                );
            }
        }
    }
}

}  // namespace vw::gfx

#endif  // VW_GFX_VOX_SERIALIZER_INL_H
