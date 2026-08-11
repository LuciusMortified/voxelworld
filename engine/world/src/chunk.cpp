module vw.world;

import std;

namespace vw::ecs {

chunk::chunk(
    world& w, vec3i coord, std::shared_ptr<asset::model> mdl, int32 voxel_scale
)
    : world_(&w)
    , ent_(w.create()
        .with<transform_component>()
        .with<model_component>()
        .with<spatial_component>()
        .get_entity())
    , model_(std::move(mdl)) {
    auto world_pos = vec3f{
        static_cast<float32>(coord.x * size * voxel_scale),
        static_cast<float32>(coord.y * size * voxel_scale),
        static_cast<float32>(coord.z * size * voxel_scale)
    };

    auto vs = static_cast<float32>(voxel_scale);
    w.system<transform_system>().modify(ent_)
        .set_position(world_pos)
        .set_scale({vs, vs, vs});
    {
        auto modifier = w.system<model_system>().modify(ent_);
        modifier.set_model(model_);
        modifier.set_top_brightness(true);
    }
    w.system<spatial_system>().modify(ent_).set_layer(spatial_layer::terrain);
}

chunk::~chunk() {
    if (world_ != nullptr && ent_.is_valid()) {
        world_->destroy(ent_);
    }
}

chunk::chunk(
    chunk&& other
) noexcept
    : world_(other.world_)
    , ent_(other.ent_)
    , model_(std::move(other.model_)) {
    other.world_ = nullptr;
    other.ent_   = invalid_entity;
}

auto chunk::operator=(
    chunk&& other
) noexcept -> chunk& {
    if (this != &other) {
        if (world_ != nullptr && ent_.is_valid()) {
            world_->destroy(ent_);
        }
        world_       = other.world_;
        ent_         = other.ent_;
        model_       = std::move(other.model_);
        other.world_ = nullptr;
        other.ent_   = invalid_entity;
    }
    return *this;
}

auto chunk::get_voxel(
    int32 x, int32 y, int32 z
) const -> voxel {
    return model_->get_voxel(x, y, z);
}

auto chunk::get_voxel(
    vec3i local
) const -> voxel {
    return model_->get_voxel(local);
}

void chunk::set_voxel(
    int32 x, int32 y, int32 z, const voxel& v
) const {
    model_->set_voxel(x, y, z, v);
}

void chunk::set_voxel(
    vec3i local, const voxel& v
) const {
    model_->set_voxel(local, v);
}

auto chunk::is_empty(
    int32 x, int32 y, int32 z
) const -> bool {
    return model_->is_empty(x, y, z);
}

auto chunk::get_model() const -> std::shared_ptr<asset::model> {
    return model_;
}

auto chunk::get_entity() const -> entity {
    return ent_;
}

}  // namespace vw::ecs
