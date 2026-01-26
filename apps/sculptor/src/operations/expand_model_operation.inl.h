#pragma once

#ifndef VW_SCULPTOR_EXPAND_MODEL_OPERATION_INL_H
#define VW_SCULPTOR_EXPAND_MODEL_OPERATION_INL_H

namespace vw::sculptor {

inline expand_model_operation::expand_model_operation(
    engine_type& eng, app_state& st, const expand_model_params& params
) : engine_(&eng), state_(&st), params_(params) {}

inline void expand_model_operation::execute() {
    auto ent = state_->name_to_entity[params_.name];

    auto& world        = engine_->get_world();
    auto& model_registry = world.get_model_registry();
    auto& model_system = world.get_model_system();
    auto& transform_system = world.get_transform_system();

    auto& model_comp = world.get_component<gfx::model_component>(ent);
    auto model = model_comp.get_model();

    auto size = model->size();
    auto new_size = vec3i{
        size.x + std::abs(params_.dir.x),
        size.y + std::abs(params_.dir.y),
        size.z + std::abs(params_.dir.z)
    };
    auto new_model = model_registry.create_unnamed(new_size);

    auto zeroed_dir = vec3i{
        params_.dir.x < 0 ? 1 : 0,
        params_.dir.y < 0 ? 1 : 0,
        params_.dir.z < 0 ? 1 : 0
    };

    for (int x = 0; x < size.x; ++x) {
        for (int y = 0; y < size.y; ++y) {
            for (int z = 0; z < size.z; ++z) {
                const auto v = model->get_voxel(x, y, z);
                const auto new_p = vec3i{
                    x + zeroed_dir.x,
                    y + zeroed_dir.y,
                    z + zeroed_dir.z
                };
                new_model->set_voxel(new_p, v);
            }
        }
    }

    model_system.modify(ent).set_model(new_model);

    auto& transform_comp = world.get_component<gfx::transform_component>(ent);
    auto new_origin = transform_comp.get_origin() - vec3f{
        static_cast<float>(zeroed_dir.x),
        static_cast<float>(zeroed_dir.y),
        static_cast<float>(zeroed_dir.z)
    };
    transform_system.modify(ent).set_origin(new_origin);
}

inline void expand_model_operation::undo() {
    auto ent = state_->name_to_entity[params_.name];

    auto& world        = engine_->get_world();
    auto& model_registry = world.get_model_registry();
    auto& model_system = world.get_model_system();
    auto& transform_system = world.get_transform_system();

    auto& model_comp = world.get_component<gfx::model_component>(ent);
    auto model = model_comp.get_model();

    auto size = model->size();
    auto new_size = vec3i{
        size.x - std::abs(params_.dir.x),
        size.y - std::abs(params_.dir.y),
        size.z - std::abs(params_.dir.z)
    };
    auto new_model = model_registry.create_unnamed(new_size);

    const vec3i beg = vec3i{
        params_.dir.x < 0 ? 1 : 0,
        params_.dir.y < 0 ? 1 : 0,
        params_.dir.z < 0 ? 1 : 0
    };
    const vec3i end = vec3i{
        params_.dir.x <= 0 ? size.x : size.x - 1,
        params_.dir.y <= 0 ? size.y : size.y - 1,
        params_.dir.z <= 0 ? size.z : size.z - 1
    };
    const vec3i zeroed_dir = vec3i{
        params_.dir.x < 0 ? 1 : 0,
        params_.dir.y < 0 ? 1 : 0,
        params_.dir.z < 0 ? 1 : 0
    };

    for (int x = beg.x; x < end.x; ++x) {
        for (int y = beg.y; y < end.y; ++y) {
            for (int z = beg.z; z < end.z; ++z) {
                const auto v = model->get_voxel(x, y, z);
                const auto new_p = vec3i{
                    x - zeroed_dir.x,
                    y - zeroed_dir.y,
                    z - zeroed_dir.z
                };
                new_model->set_voxel(new_p, v);
            }
        }
    }

    model_system.modify(ent).set_model(new_model);

    auto& transform_comp = world.get_component<gfx::transform_component>(ent);
    auto new_origin = transform_comp.get_origin() + vec3f{
        static_cast<float>(zeroed_dir.x),
        static_cast<float>(zeroed_dir.y),
        static_cast<float>(zeroed_dir.z)
    };
    transform_system.modify(ent).set_origin(new_origin);
}

}  // namespace vw::sculptor

#endif  // VW_SCULPTOR_EXPAND_MODEL_OPERATION_INL_H
