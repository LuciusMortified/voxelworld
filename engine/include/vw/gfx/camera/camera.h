#pragma once

#ifndef VW_GFX_CAMERA_H
#define VW_GFX_CAMERA_H

#include "vw/core.h"

namespace vw::gfx {
class camera {
public:
    explicit camera(
        float fov = 45.0f, float aspect = 16.0f / 9.0f, float near = 0.1f, float far = 1000.0f
    );

    void set_position(const vec3f& position);
    void set_rotation(float pitch, float yaw);
    void set_aspect_ratio(float aspect);

    [[nodiscard]] vec3f get_position() const;
    [[nodiscard]] float get_pitch() const;
    [[nodiscard]] float get_yaw() const;

    mat4f get_view_matrix() const;
    mat4f get_projection_matrix() const;
    mat4f get_view_projection_matrix() const;

    void move_forward(float distance);
    void move_right(float distance);
    void move_up(float distance);
    void rotate(float delta_pitch, float delta_yaw);

    vec3f get_forward() const;
    vec3f get_right() const;
    vec3f get_up() const;

private:
    void update_vectors() const;
    void update_view_matrix() const;
    void update_projection_matrix() const;

    vec3f position_;
    float pitch_, yaw_;
    float fov_, aspect_, near_, far_;

    mutable vec3f forward_, right_, up_;
    mutable bool vectors_dirty_;

    mutable mat4f view_matrix_;
    mutable mat4f projection_matrix_;
    mutable bool view_matrix_dirty_;
    mutable bool projection_matrix_dirty_;
};
}  // namespace vw::gfx

#include "vw/gfx/camera/camera.inl.h"

#endif  // VW_GFX_CAMERA_H
