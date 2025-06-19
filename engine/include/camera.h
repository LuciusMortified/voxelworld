#pragma once
#include "types.h"

namespace voxel {
    class camera {
    public:
        camera(float fov = 45.0f, float aspect = 16.0f/9.0f, float near = 0.1f, float far = 100.0f);

        void set_position(const vec3f& position);
        void set_rotation(float pitch, float yaw);
        void set_aspect_ratio(float aspect);
        
        vec3f get_position() const { return position_; }
        float get_pitch() const { return pitch_; }
        float get_yaw() const { return yaw_; }

        // Получить матрицы
        void get_view_matrix(float* matrix) const;
        void get_projection_matrix(float* matrix) const;
        void get_view_projection_matrix(float* matrix) const;

        // Управление камерой
        void move_forward(float distance);
        void move_right(float distance);
        void move_up(float distance);
        void rotate(float delta_pitch, float delta_yaw);

        // Направления камеры
        vec3f get_forward() const;
        vec3f get_right() const;
        vec3f get_up() const;

    private:
        void update_vectors();

        vec3f position_;
        float pitch_, yaw_;
        float fov_, aspect_, near_, far_;
        
        // Кэшированные направления
        vec3f forward_, right_, up_;
        bool vectors_dirty_;
    };
}