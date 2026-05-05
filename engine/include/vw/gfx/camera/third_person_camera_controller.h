#pragma once

#ifndef VW_GFX_CAMERA_THIRD_PERSON_CAMERA_CONTROLLER_H
#define VW_GFX_CAMERA_THIRD_PERSON_CAMERA_CONTROLLER_H

#include <cmath>

#include "vw/core.h"
#include "vw/gfx/camera/camera.h"
#include "vw/gfx/player/player_input_state.h"
#include "vw/ecs/spatial/spatial_layer.h"
#include "vw/ecs/world.h"
#include "vw/spatial/ray.h"

namespace vw::gfx {

using namespace ::vw::ecs;

struct third_person_camera_params {
    float32 arm_length     = 10.0f;
    float32 arm_length_min = 2.0f;
    float32 arm_length_max = 50.0f;
    vec3f target_offset    = {0.0f, 8.0f, 0.0f};
    float32 pitch_min      = -89.0f;
    float32 pitch_max      = 89.0f;
    float32 zoom_speed     = 2.0f;
    float32 collision_skin = 0.3f;
};

/// Third-person camera that follows an entity with arm length, offset, and voxel collision.
template <typename WC = base_world_def>
class third_person_camera_controller {
public:
    using world_type = world<WC>;
    explicit third_person_camera_controller(
        camera& camera,
        world_type& world,
        third_person_camera_params params = {}
    );

    void update(const player_input_state& input, entity target);

    [[nodiscard]] auto get_params() -> third_person_camera_params&;
    [[nodiscard]] auto get_pitch() const -> float32;
    [[nodiscard]] auto get_yaw() const -> float32;
    [[nodiscard]] auto get_actual_arm_length() const -> float32;

private:
    camera* camera_;
    world_type* world_;
    third_person_camera_params params_;

    float32 pitch_ = 20.0f;
    float32 yaw_   = 0.0f;
    float32 actual_arm_length_ = 0.0f;
};

template <typename WC>
struct third_person_camera_controller_from_tuple;

template <typename... Cs>
struct third_person_camera_controller_from_tuple<std::tuple<Cs...>> {
    using type = third_person_camera_controller<std::tuple<Cs...>>;
};

}  // namespace vw::gfx

#include "vw/gfx/camera/third_person_camera_controller.inl.h"

#endif  // VW_GFX_CAMERA_THIRD_PERSON_CAMERA_CONTROLLER_H
