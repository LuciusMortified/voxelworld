#pragma once

#ifndef VW_SPATIAL_FRUSTUM_INL_H
#define VW_SPATIAL_FRUSTUM_INL_H

#include <algorithm>
#include <cmath>
#include <limits>

#include "vw/core/math.h"
#include "vw/spatial/aabb.h"
#include "vw/spatial/frustum.h"
#include "vw/spatial/ray.h"

namespace vw::spatial {

inline frustum frustum::from_view_projection_matrix(
    const mat4f& view_proj
) {
    frustum f;

    {
        vec4f plane_vec{
            view_proj[3, 0] + view_proj[0, 0],
            view_proj[3, 1] + view_proj[0, 1],
            view_proj[3, 2] + view_proj[0, 2],
            view_proj[3, 3] + view_proj[0, 3]
        };
        float length = vw::math::length(vec3f{plane_vec.x, plane_vec.y, plane_vec.z});
        if (length > 0.0f) {
            float inv_length     = 1.0f / length;
            f.planes[0].normal   = vec3f{plane_vec.x, plane_vec.y, plane_vec.z} * inv_length;
            f.planes[0].distance = plane_vec.w * inv_length;
        }
    }

    {
        vec4f plane_vec{
            view_proj[3, 0] - view_proj[0, 0],
            view_proj[3, 1] - view_proj[0, 1],
            view_proj[3, 2] - view_proj[0, 2],
            view_proj[3, 3] - view_proj[0, 3]
        };
        float length = vw::math::length(vec3f{plane_vec.x, plane_vec.y, plane_vec.z});
        if (length > 0.0f) {
            float inv_length     = 1.0f / length;
            f.planes[1].normal   = vec3f{plane_vec.x, plane_vec.y, plane_vec.z} * inv_length;
            f.planes[1].distance = plane_vec.w * inv_length;
        }
    }

    {
        vec4f plane_vec{
            view_proj[3, 0] + view_proj[1, 0],
            view_proj[3, 1] + view_proj[1, 1],
            view_proj[3, 2] + view_proj[1, 2],
            view_proj[3, 3] + view_proj[1, 3]
        };
        float length = vw::math::length(vec3f{plane_vec.x, plane_vec.y, plane_vec.z});
        if (length > 0.0f) {
            float inv_length     = 1.0f / length;
            f.planes[2].normal   = vec3f{plane_vec.x, plane_vec.y, plane_vec.z} * inv_length;
            f.planes[2].distance = plane_vec.w * inv_length;
        }
    }

    {
        vec4f plane_vec{
            view_proj[3, 0] - view_proj[1, 0],
            view_proj[3, 1] - view_proj[1, 1],
            view_proj[3, 2] - view_proj[1, 2],
            view_proj[3, 3] - view_proj[1, 3]
        };
        float length = vw::math::length(vec3f{plane_vec.x, plane_vec.y, plane_vec.z});
        if (length > 0.0f) {
            float inv_length     = 1.0f / length;
            f.planes[3].normal   = vec3f{plane_vec.x, plane_vec.y, plane_vec.z} * inv_length;
            f.planes[3].distance = plane_vec.w * inv_length;
        }
    }

    {
        vec4f plane_vec{
            view_proj[3, 0] + view_proj[2, 0],
            view_proj[3, 1] + view_proj[2, 1],
            view_proj[3, 2] + view_proj[2, 2],
            view_proj[3, 3] + view_proj[2, 3]
        };
        float length = vw::math::length(vec3f{plane_vec.x, plane_vec.y, plane_vec.z});
        if (length > 0.0f) {
            float inv_length     = 1.0f / length;
            f.planes[4].normal   = vec3f{plane_vec.x, plane_vec.y, plane_vec.z} * inv_length;
            f.planes[4].distance = plane_vec.w * inv_length;
        }
    }

    {
        vec4f plane_vec{
            view_proj[3, 0] - view_proj[2, 0],
            view_proj[3, 1] - view_proj[2, 1],
            view_proj[3, 2] - view_proj[2, 2],
            view_proj[3, 3] - view_proj[2, 3]
        };
        float length = vw::math::length(vec3f{plane_vec.x, plane_vec.y, plane_vec.z});
        if (length > 0.0f) {
            float inv_length     = 1.0f / length;
            f.planes[5].normal   = vec3f{plane_vec.x, plane_vec.y, plane_vec.z} * inv_length;
            f.planes[5].distance = plane_vec.w * inv_length;
        }
    }

    return f;
}

inline bool frustum::intersects(
    const vec3f& point
) const {
    for (int i = 0; i < 6; ++i) {
        float dist = math::dot(planes[i].normal, point) + planes[i].distance;
        if (dist < 0.0f) {
            return false;
        }
    }
    return true;
}

inline bool frustum::intersects(
    const aabb& bounds
) const {
    for (int i = 0; i < 6; ++i) {
        const vec3f p_vertex{
            planes[i].normal.x > 0.0f ? bounds.max.x : bounds.min.x,
            planes[i].normal.y > 0.0f ? bounds.max.y : bounds.min.y,
            planes[i].normal.z > 0.0f ? bounds.max.z : bounds.min.z
        };

        const float dist = vw::math::dot(planes[i].normal, p_vertex) + planes[i].distance;
        if (dist < 0.0f) {
            return false;
        }
    }
    return true;
}

inline bool frustum::intersects(
    const ray& r
) const {
    float t_min = 0.0f;
    float t_max = r.length();

    for (int i = 0; i < 6; ++i) {
        const plane& p = planes[i];

        float denom = vw::math::dot(p.normal, r.direction);

        if (std::abs(denom) < 1e-6f) {
            float dist = vw::math::dot(p.normal, r.start) + p.distance;
            if (dist < 0.0f) {
                return false;
            }
            continue;
        }

        float t = -(vw::math::dot(p.normal, r.start) + p.distance) / denom;

        if (denom > 0.0f) {
            t_max = std::min(t_max, t);
        } else {
            t_min = std::max(t_min, t);
        }

        if (t_min > t_max) {
            return false;
        }
    }

    return t_min <= r.length() && t_max >= 0.0f;
}

inline bool frustum::operator==(
    const frustum& other
) const {
    for (int i = 0; i < 6; ++i) {
        if (planes[i].normal != other.planes[i].normal ||
            std::abs(planes[i].distance - other.planes[i].distance) > 1e-5f) {
            return false;
        }
    }
    return true;
}

inline bool frustum::operator!=(
    const frustum& other
) const {
    return !(*this == other);
}

inline bool frustum::approximately_equal(
    const frustum& other, float angle_threshold, float distance_threshold
) const {
    for (int i = 0; i < 6; ++i) {
        const float angle_diff = std::acos(
            math::clamp(math::dot(planes[i].normal, other.planes[i].normal), -1.0f, 1.0f)
        );
        const float distance_diff = std::abs(planes[i].distance - other.planes[i].distance);

        if (angle_diff > angle_threshold || distance_diff > distance_threshold) {
            return false;
        }
    }
    return true;
}

}  // namespace vw::spatial

#endif  // VW_SPATIAL_FRUSTUM_INL_H
