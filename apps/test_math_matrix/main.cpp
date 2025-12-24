#include <vw/core.h>
#include <vw/log.h>

using namespace vw;

int main() {
    float angle = math::radians(90.0f);
    mat4f ry    = math::rotation_matrix_y(angle);
    vec3f v{1, 0, 0};
    vec3f vr = ry * v;

    log::info("Original vector: ({}, {}, {})", v.x, v.y, v.z);
    log::info("Rotated vector: ({}, {}, {})", vr.x, vr.y, vr.z);
}