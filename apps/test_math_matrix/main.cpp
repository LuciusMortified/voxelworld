#include <iostream>

#include <vw/core.h>

using namespace vw;

int main() {
    float angle = math::radians(90.0f);
    mat4f ry = math::rotation_matrix_y(angle);
    vec3f v{1, 0, 0};
    vec3f vr = ry * v;

    std::cout << "Original vector: (" << v.x << ", " << v.y << ", " << v.z << ")\n";
    std::cout << "Rotated vector: (" << vr.x << ", " << vr.y << ", " << vr.z << ")\n";
}