#version 460 core

const int SHADOW_CASCADES = 5;

layout(location = 2) in uint inInstanceIndex;

layout(set = 0, binding = 0) uniform ShadowUniformBufferObject {
    mat4 light_space_matrices[SHADOW_CASCADES];
} shadowUbo;

layout(set = 1, binding = 0, std430) readonly buffer ModelMatrices {
    mat4 models[];
} modelMatrices;

layout(set = 1, binding = 1, std430) readonly buffer NormalMatrices {
    mat4 normals[];
} normalMatrices;

// Same records the colour pass reads, same unrolling; the shadow pass only
// needs the position out of them.
struct Quad {
    uint data0;
    uint data1;

    // Shadows do not read the light, but the record has to be the size the
    // buffer was written at.
    uint data2;
};

layout(set = 1, binding = 2, std430) readonly buffer Quads {
    Quad quads[];
};

layout(push_constant) uniform ShadowPushConstants {
    uint cascadeIndex;
} pushConstants;

const uvec3 FACE_VERTS[6][4] = uvec3[6][4](
    uvec3[4](uvec3(1, 0, 0), uvec3(1, 0, 1), uvec3(1, 1, 1), uvec3(1, 1, 0)),  // +X
    uvec3[4](uvec3(0, 0, 0), uvec3(0, 1, 0), uvec3(0, 1, 1), uvec3(0, 0, 1)),  // -X
    uvec3[4](uvec3(0, 1, 0), uvec3(1, 1, 0), uvec3(1, 1, 1), uvec3(0, 1, 1)),  // +Y
    uvec3[4](uvec3(0, 0, 0), uvec3(0, 0, 1), uvec3(1, 0, 1), uvec3(1, 0, 0)),  // -Y
    uvec3[4](uvec3(0, 0, 1), uvec3(0, 1, 1), uvec3(1, 1, 1), uvec3(1, 0, 1)),  // +Z
    uvec3[4](uvec3(1, 0, 0), uvec3(1, 1, 0), uvec3(0, 1, 0), uvec3(0, 0, 0))   // -Z
);

void main() {
    Quad q = quads[uint(gl_VertexIndex) / 4u];
    uint corner_id = uint(gl_VertexIndex) % 4u;

    uvec3 mn = uvec3(q.data0 & 0x7Fu, (q.data0 >> 7) & 0x7Fu, (q.data0 >> 14) & 0x7Fu);
    uvec3 mx = uvec3(q.data1 & 0x7Fu, (q.data1 >> 7) & 0x7Fu, (q.data1 >> 14) & 0x7Fu);

    uint normal_id = (q.data0 >> 21) & 0x7u;
    uvec3 pick     = FACE_VERTS[normal_id][corner_id];

    mat4 model = modelMatrices.models[inInstanceIndex];

    vec3 localPos = vec3(mix(mn, mx, bvec3(pick)));
    vec4 worldPos = model * vec4(localPos, 1.0);

    mat4 lightSpaceMatrix = shadowUbo.light_space_matrices[pushConstants.cascadeIndex];
    gl_Position = lightSpaceMatrix * worldPos;
}
