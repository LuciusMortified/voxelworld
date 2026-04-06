#pragma once

#ifndef VW_GFX_VOX_DESERIALIZER_INL_H
#define VW_GFX_VOX_DESERIALIZER_INL_H

namespace vw::gfx {

template <typename WC>
vox_deserializer<WC>::vox_deserializer(world_type& world, vox_parser& parser)
    : world_(&world), parser_(&parser) {}

template <typename WC>
auto vox_deserializer<WC>::deserialize(
    const std::filesystem::path& filepath, const options& opts
) -> std::expected<result, error_type> {
    auto prefab = parser_->parse(filepath);
    if (!prefab.has_value()) {
        return std::unexpected(prefab.error());
    }

    result res;
    res.root_name = std::move(prefab->root_name);

    for (const auto& ent_data : prefab->entities) {
        apply_entity_(ent_data, res, opts);
    }

    return res;
}

template <typename WC>
void vox_deserializer<WC>::apply_entity_(
    const vox_entity_data& data, result& res, const options& opts
) {
    auto ent_guard = std::make_unique<entity_guard<WC>>(*world_);
    ent_guard->template with<hierarchy_component>();
    ent_guard->template with<transform_component>();
    ent_guard->template with<spatial_component>();

    auto ent = ent_guard->get_entity();

    res.name_to_entity[data.name] = ent;
    res.entity_to_name[ent]       = data.name;

    if (!data.parent_name.empty() && res.name_to_entity.contains(data.parent_name)) {
        auto parent_entity     = res.name_to_entity[data.parent_name];
        auto& hierarchy_system = world_->get_hierarchy_system();
        hierarchy_system.modify(ent).set_parent(parent_entity);
    }

    if (data.has_transform) {
        auto& transform_system = world_->get_transform_system();
        transform_system.modify(ent)
            .set_position(data.position)
            .set_rotation_euler(data.rotation)
            .set_scale(data.scale)
            .set_origin(data.origin);
    }

    if (data.animation_target_name.has_value() && !opts.skip_targets) {
        ent_guard->template with<animation_target_component>();
        auto& animation_system = world_->get_animation_system();
        animation_system.modify_target(ent).set_target_name(*data.animation_target_name);
    }

    if (data.has_sockets && !opts.skip_sockets) {
        ent_guard->template with<socket_component>();
        auto& socket_system = world_->get_socket_system();
        for (const auto& sp : data.sockets) {
            socket_system.modify(ent).add_socket(
                sp.name, sp.position, math::euler_to_quat(sp.rotation), sp.scale
            );
        }
    }

    if (data.model.has_value()) {
        auto& model_registry = world_->get_model_registry();
        auto model_ptr = model_registry.create(data.name, data.model->size);

        ent_guard->template with<model_component>();

        auto& model_system = world_->get_model_system();
        model_system.modify(ent).set_model(model_ptr);

        for (const auto& [pos, v] : data.model->voxels) {
            model_ptr->set_voxel(pos, v);
        }
    }

    res.entities.push_back(std::move(ent_guard));
}

}  // namespace vw::gfx

#endif  // VW_GFX_VOX_DESERIALIZER_INL_H
