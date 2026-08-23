module vw.world;

import std;

namespace vw::ecs {

chunk::chunk(
    world& w, vec3i coord, std::shared_ptr<asset::model> mdl, int32 voxel_scale
)
    : world_(&w)
    , coord_(coord)
    , voxel_scale_(voxel_scale)
    , ent_(invalid_entity)
    , model_(std::move(mdl))
    , fill_(model_->scan_fill()) {
    for (int32 fd = 0; fd < 6; ++fd) {
        if (model_->has_boundary_slice(fd)) {
            known_neighbors_ |= static_cast<uint8>(1U << fd);
        }
    }

    // Сплошная порода со всеми прижатыми соседями либо один воздух: в обоих случаях
    // мешер прошёл бы весь чанк и не вернул ни одного квада. К моменту размещения
    // чанка границы окончательны, поэтому ответ под ним не изменится.
    const bool has_faces = fill_ == asset::model_fill::mixed ||
        (fill_ == asset::model_fill::solid && !model_->boundaries_are_solid());

    if (has_faces) {
        create_entity_();
        return;
    }

    // Этот чанк никто и никогда не смешит, поэтому только что выданные ему
    // плоскости уже сделали всю работу, какую могли. Для чанков, которые он
    // действительно строит, их освобождает мешер.
    model_->release_boundary();
}

void chunk::create_entity_() {
    auto& w = *world_;

    ent_ = w.create()
        .with<transform_component>()
        .with<model_component>()
        .with<spatial_component>()
        .get_entity();

    auto world_pos = vec3f{
        static_cast<float32>(coord_.x * size * voxel_scale_),
        static_cast<float32>(coord_.y * size * voxel_scale_),
        static_cast<float32>(coord_.z * size * voxel_scale_)
    };

    auto vs = static_cast<float32>(voxel_scale_);
    w.system<transform_system>().modify(ent_)
        .set_position(world_pos)
        .set_scale({vs, vs, vs});
    w.system<model_system>().modify(ent_).set_model(model_);
    w.system<spatial_system>().modify(ent_).set_layer(spatial_layer::terrain);
}

auto chunk::ensure_entity() -> bool {
    if (world_ == nullptr || ent_.is_valid()) {
        return false;
    }

    create_entity_();
    return true;
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
    , coord_(other.coord_)
    , voxel_scale_(other.voxel_scale_)
    , ent_(other.ent_)
    , model_(std::move(other.model_))
    , fill_(other.fill_)
    , known_neighbors_(other.known_neighbors_) {
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
        coord_       = other.coord_;
        voxel_scale_ = other.voxel_scale_;
        ent_             = other.ent_;
        model_           = std::move(other.model_);
        fill_            = other.fill_;
        known_neighbors_ = other.known_neighbors_;
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
) {
    model_->set_voxel(x, y, z, v);
    fill_ = model_->scan_fill();
}

void chunk::set_voxel(
    vec3i local, const voxel& v
) {
    set_voxel(local.x, local.y, local.z, v);
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

auto chunk::is_drawn() const -> bool {
    return ent_.is_valid();
}

auto chunk::is_solid() const -> bool {
    return fill_ == asset::model_fill::solid;
}

auto chunk::known_neighbors() const -> uint8 {
    return known_neighbors_;
}

void chunk::set_known_neighbors(uint8 mask) {
    known_neighbors_ = mask;
}

}  // namespace vw::ecs
