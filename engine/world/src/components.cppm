export module vw.world:components;

import std;

import vw.core;
import vw.ecs;
import :anim;
import :index;
import :model;

export namespace vw::ecs {

class animation_fsm_system;
class animation_system;
class character_controller_system;
class hierarchy_system;
class light_system;
class model_system;
class physics_system;
class socket_system;
class spatial_system;
class transform_system;
class world_grid_system;

struct transform_component final {
    [[nodiscard]] auto get_local_matrix() const -> mat4f {
        if (local_dirty_) {
            local_matrix_ = transform_.calc_matrix();
            local_dirty_  = false;
        }
        return local_matrix_;
    }

    [[nodiscard]] auto get_world_matrix() const -> mat4f {
        return world_matrix_;
    }

    [[nodiscard]] auto get_transform() const -> const transform& {
        return transform_;
    }

    [[nodiscard]] auto get_position() const -> const vec3f& {
        return transform_.get_position();
    }

    [[nodiscard]] auto get_rotation() const -> const quat& {
        return transform_.get_rotation();
    }

    [[nodiscard]] auto get_rotation_euler() const -> vec3f {
        return transform_.get_rotation_euler();
    }

    [[nodiscard]] auto get_scale() const -> const vec3f& {
        return transform_.get_scale();
    }

    [[nodiscard]] auto get_origin() const -> const vec3f& {
        return transform_.get_origin();
    }

private:
    friend class hierarchy_system;
    friend class transform_system;

    transform transform_;

    mutable mat4f local_matrix_;
    mutable bool local_dirty_ = true;

    mutable mat4f world_matrix_;
    mutable bool world_dirty_ = true;
};

struct hierarchy_component final {
    [[nodiscard]] auto has_parent() const -> bool {
        return parent_.is_valid();
    }

    [[nodiscard]] auto get_parent() const -> entity {
        return parent_;
    }

    [[nodiscard]] auto has_child(entity child) const -> bool {
        return std::ranges::contains(children_, child);
    }

    [[nodiscard]] auto get_children() const -> const std::vector<entity>& {
        return children_;
    }

private:
    friend class hierarchy_system;
    friend class transform_system;

    entity parent_;
    std::vector<entity> children_;
};

struct model_component final {
    [[nodiscard]] auto has_model() const -> bool {
        return model_ != nullptr;
    }

    [[nodiscard]] auto get_model() const -> std::shared_ptr<asset::model> {
        return model_;
    }

    [[nodiscard]] auto get_voxel(int32 x, int32 y, int32 z) const -> voxel {
        return model_->get_voxel(x, y, z);
    }

    [[nodiscard]] auto get_voxel(vec3i pos) const -> voxel {
        return model_->get_voxel(pos);
    }

    [[nodiscard]] auto is_empty(int32 x, int32 y, int32 z) const -> bool {
        return model_->is_empty(x, y, z);
    }

    [[nodiscard]] auto is_empty(vec3i pos) const -> bool {
        return model_->is_empty(pos);
    }

    [[nodiscard]] auto width() const -> int32 {
        return model_->width();
    }

    [[nodiscard]] auto height() const -> int32 {
        return model_->height();
    }

    [[nodiscard]] auto depth() const -> int32 {
        return model_->depth();
    }

    [[nodiscard]] auto size() const -> vec3i {
        return model_->size();
    }

    [[nodiscard]] auto get_identity() const -> asset::model_identity {
        return model_->get_identity();
    }

private:
    friend class model_system;

    std::shared_ptr<asset::model> model_;
};

struct spatial_component final {
    [[nodiscard]] auto get_bounds() const -> const spatial::aabb& {
        return bounds_;
    }

    [[nodiscard]] auto get_layer() const -> spatial_layer_mask {
        return layer_;
    }

    [[nodiscard]] auto is_dirty() const -> bool {
        return dirty_;
    }

private:
    friend class spatial_system;

    spatial::aabb bounds_;
    spatial::aabb fat_bounds_;
    spatial_layer_mask layer_ = spatial_layer::prop;
    bool dirty_               = true;
};

struct light_component final {
    [[nodiscard]] auto get_color() const -> const vec3f& {
        return color_;
    }

    [[nodiscard]] auto get_intensity() const -> float32 {
        return intensity_;
    }

    [[nodiscard]] auto get_range() const -> float32 {
        return range_;
    }

    [[nodiscard]] auto get_attenuation_constant() const -> float32 {
        return attenuation_constant_;
    }

    [[nodiscard]] auto get_attenuation_linear() const -> float32 {
        return attenuation_linear_;
    }

    [[nodiscard]] auto get_attenuation_quadratic() const -> float32 {
        return attenuation_quadratic_;
    }

private:
    friend class light_system;

    vec3f color_{1.0F, 1.0F, 1.0F};
    float32 intensity_             = 1.0F;
    float32 range_                 = 10.0F;
    float32 attenuation_constant_  = 1.0F;
    float32 attenuation_linear_    = 0.09F;
    float32 attenuation_quadratic_ = 0.032F;
};

struct box_collider_component final {
    [[nodiscard]] auto get_extents() const -> const vec3f& {
        return extents_;
    }

    [[nodiscard]] auto get_offset() const -> const vec3f& {
        return offset_;
    }

private:
    friend class physics_system;

    vec3f extents_{1.0F, 1.0F, 1.0F};
    vec3f offset_{0.0F, 0.0F, 0.0F};
};

struct rigid_body_component final {
    [[nodiscard]] auto get_velocity() const -> const vec3f& {
        return velocity_;
    }

    [[nodiscard]] auto get_impulse() const -> const vec3f& {
        return impulse_;
    }

    [[nodiscard]] auto get_gravity_scale() const -> float32 {
        return gravity_scale_;
    }

    [[nodiscard]] auto get_drag() const -> float32 {
        return drag_;
    }

    [[nodiscard]] auto is_grounded() const -> bool {
        return grounded_;
    }

    [[nodiscard]] auto is_frozen() const -> bool {
        return frozen_;
    }

private:
    friend class physics_system;

    vec3f velocity_{0.0F, 0.0F, 0.0F};
    vec3f impulse_{0.0F, 0.0F, 0.0F};
    float32 gravity_scale_ = 1.0F;
    float32 drag_          = 5.0F;
    bool grounded_         = false;
    bool frozen_           = false;
};

using axis_flags = uint8;

namespace axis_flag {
inline constexpr axis_flags none = 0;
inline constexpr axis_flags x    = 1;
inline constexpr axis_flags y    = 2;
inline constexpr axis_flags z    = 4;
inline constexpr axis_flags xz   = x | z;
inline constexpr axis_flags xyz  = x | y | z;
}  // namespace axis_flag

struct movement_intent_component final {
    [[nodiscard]] auto get_wish_velocity() const -> const vec3f& {
        return wish_velocity_;
    }

    [[nodiscard]] auto get_wish_axes() const -> axis_flags {
        return wish_axes_;
    }

private:
    friend class physics_system;
    friend class character_controller_system;

    vec3f wish_velocity_{0.0F, 0.0F, 0.0F};
    axis_flags wish_axes_ = axis_flag::xz;
};

struct character_controller_component final {
    [[nodiscard]] auto get_move_speed() const -> float32 {
        return move_speed_;
    }

    [[nodiscard]] auto get_jump_impulse() const -> float32 {
        return jump_impulse_;
    }

    [[nodiscard]] auto get_rotation_speed() const -> float32 {
        return rotation_speed_;
    }

    [[nodiscard]] auto get_facing_direction() const -> const vec3f& {
        return facing_direction_;
    }

private:
    friend class character_controller_system;

    vec3f move_input_{0.0F, 0.0F, 0.0F};
    vec3f facing_direction_{0.0F, 0.0F, 1.0F};
    float32 move_speed_     = 100.0F;
    float32 jump_impulse_   = 150.0F;
    float32 rotation_speed_ = 5.0F;
    bool jump_requested_    = false;
};

struct socket_point {
    std::string name;
    vec3f position{0.0F, 0.0F, 0.0F};
    quat rotation;
    vec3f scale{1.0F, 1.0F, 1.0F};
    entity attached = invalid_entity;
};

struct socket_component final {
    socket_component() = default;

    explicit socket_component(std::vector<socket_point> sockets) : sockets_(std::move(sockets)) {}

    [[nodiscard]] auto get_sockets() const -> const std::vector<socket_point>& {
        return sockets_;
    }

    [[nodiscard]] auto find(const std::string& name) const -> const socket_point* {
        const auto it =
            std::ranges::find_if(sockets_, [&](const socket_point& sp) { return sp.name == name; });
        return it != sockets_.end() ? &(*it) : nullptr;
    }

    [[nodiscard]] auto is_occupied(const std::string& name) const -> bool {
        const auto* sp = find(name);
        return sp != nullptr && sp->attached.is_valid();
    }

private:
    friend class socket_system;

    std::vector<socket_point> sockets_;
};

struct animation_target_component final {
    [[nodiscard]] auto get_name() const -> const std::string& {
        return target_name_;
    }

    [[nodiscard]] auto get_rest_transform() const -> const transform& {
        return rest_transform_;
    }

private:
    friend class animation_system;

    std::string target_name_;
    transform rest_transform_;
};

struct animation_player_component final {
    [[nodiscard]] auto layer_count() const -> std::size_t {
        return layers_.size();
    }

    [[nodiscard]] auto has_layer(std::size_t index) const -> bool {
        return index < layers_.size();
    }

    [[nodiscard]] auto get_layer(std::size_t index) const -> const asset::animation_layer& {
        return layers_[index];
    }

    [[nodiscard]] auto is_any_playing() const -> bool {
        return std::ranges::any_of(layers_, [](const auto& layer) { return layer.is_active(); });
    }

private:
    friend class animation_system;

    std::vector<asset::animation_layer> layers_;
};

struct animation_fsm_component final {
    [[nodiscard]] auto machine_count() const -> std::size_t {
        return machines_.size();
    }

    [[nodiscard]] auto get_machine(std::size_t index) -> asset::animation_fsm& {
        return machines_[index];
    }

    [[nodiscard]] auto get_machine(std::size_t index) const -> const asset::animation_fsm& {
        return machines_[index];
    }

private:
    friend class animation_fsm_system;

    using trigger_set = asset::animation_fsm::trigger_set;

    std::vector<asset::animation_fsm> machines_;
    trigger_set triggers_;
};

struct world_view_component final {
    [[nodiscard]] auto get_chunk_coord() const -> vec3i {
        return chunk_coord_;
    }

    [[nodiscard]] auto get_view_distance() const -> uint32 {
        return view_distance_;
    }

private:
    friend class world_grid_system;

    vec3i chunk_coord_{0, 0, 0};
    uint32 view_distance_{10};
    bool dirty_ = true;
};

}  // namespace vw::ecs
