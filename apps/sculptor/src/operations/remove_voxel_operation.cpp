module vw.sculptor;

import std;

import vw.core;
import vw.ecs;
import vw.world;
import vw.platform;
import vw.gfx;

namespace vw::sculptor {

remove_voxel_operation::remove_voxel_operation(
    engine_type& eng, app_state& st, const remove_voxel_params& params
)
    : engine_(&eng), state_(&st), params_(params) {}

void remove_voxel_operation::execute() {
    auto ent = state_->scene.name_to_entity[params_.name];

    auto& world        = engine_->get_world();
    auto& model_sys = world.system<ecs::model_system>();

    auto& model_comp = world.get<ecs::model_component>(ent);
    previous_block_  = model_comp.get_voxel(params_.position).id;

    model_sys.modify(ent).set_voxel(params_.position, empty_voxel);
    state_->file.has_unsaved_changes = true;
}

void remove_voxel_operation::undo() {
    auto ent = state_->scene.name_to_entity[params_.name];

    auto& world        = engine_->get_world();
    auto& model_sys = world.system<ecs::model_system>();

    model_sys.modify(ent).set_voxel(params_.position, voxel{previous_block_});
    state_->file.has_unsaved_changes = true;
}

}  // namespace vw::sculptor
