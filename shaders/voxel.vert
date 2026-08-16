#version 460 core

layout(location = 2) in uint inInstanceIndex;

struct DirectionalLightData {
    mat4 light_space_matrices[4];
    vec4 cascade_splits;
    vec3 direction;
    vec3 color;
    float intensity;
};

layout(set = 0, binding = 0) uniform UniformBufferObject {
    mat4 view;
    mat4 proj;
    vec3 viewPos;
    DirectionalLightData directional_light;
    uint point_lights_count;
} ubo;

layout(set = 1, binding = 0, std430) readonly buffer ModelMatrices {
    mat4 models[];
} modelMatrices;

layout(set = 1, binding = 1, std430) readonly buffer NormalMatrices {
    mat4 normals[];
} normalMatrices;

// One record per greedy rectangle; six vertices are unrolled out of each.
// Packing mirrors gfx::quad::pack.
struct Quad {
    uint data0;
    uint data1;
};

layout(set = 1, binding = 2, std430) readonly buffer Quads {
    Quad quads[];
};

layout(set = 4, binding = 0, std430) readonly buffer PaletteBuffer {
    uint palette[];
};

layout(location = 0) out vec3 fragPos;
layout(location = 1) out vec3 fragNormal;
layout(location = 2) out vec3 fragColor;
layout(location = 3) out float viewDepth;
layout(location = 4) out vec2 fragUV;
layout(location = 5) flat out uint fragCornersMask;

const vec3 NORMALS[6] = vec3[6](
    vec3( 1,  0,  0),
    vec3(-1,  0,  0),
    vec3( 0,  1,  0),
    vec3( 0, -1,  0),
    vec3( 0,  0,  1),
    vec3( 0,  0, -1)
);

vec3 unpackPaletteColor(uint packedColor) {
    float r = float((packedColor >> 24) & 0xFFu) / 255.0;
    float g = float((packedColor >> 16) & 0xFFu) / 255.0;
    float b = float((packedColor >>  8) & 0xFFu) / 255.0;
    return vec3(r, g, b);
}

// Which corner of the rectangle each of the six vertices takes, and which end
// of the box each corner takes its components from. Both tables are the ones
// detail::add_quad used to bake into the vertex buffer.
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

    uint normal_id      = (q.data0 >> 21) & 0x7u;
    uint corners_dark   = (q.data0 >> 24) & 0xFu;
    uint corners_bright = (q.data0 >> 28) & 0xFu;
    uint palette_idx    = (q.data1 >> 21) & 0xFFu;

    uvec3 pick = FACE_VERTS[normal_id][corner_id];

    mat4 model = modelMatrices.models[inInstanceIndex];

    vec3 localPos = vec3(mix(mn, mx, bvec3(pick)));
    vec4 worldPos = model * vec4(localPos, 1.0);
    fragPos = worldPos.xyz;

    fragNormal = normalize(mat3(normalMatrices.normals[inInstanceIndex]) * NORMALS[normal_id]);

    fragColor = unpackPaletteColor(palette[palette_idx]);

    vec2 corner_uvs[4] = vec2[4](
        vec2(0.0, 0.0),
        vec2(1.0, 0.0),
        vec2(1.0, 1.0),
        vec2(0.0, 1.0)
    );
    fragUV = corner_uvs[corner_id];
    fragCornersMask = corners_dark | (corners_bright << 4);

    viewDepth = -(ubo.view * worldPos).z;

    gl_Position = ubo.proj * ubo.view * worldPos;
}
