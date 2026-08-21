#version 460 core

layout(location = 0) in vec3 fragPos;
layout(location = 1) in vec3 fragNormal;
layout(location = 2) in vec4 fragColor;
layout(location = 3) in float viewDepth;
layout(location = 4) in vec2 fragUV;
layout(location = 5) flat in uint fragCornersMask;
layout(location = 6) flat in uint fragLightMask;
layout(location = 7) flat in uint fragConvexMask;

// Cascaded shadow maps are parked, not deleted. Past the first cascade they
// staircased, and the staircase moved whenever the camera did; what they cost
// meanwhile is a depth pass, five extra culling passes and twelve depth
// compares a pixel. Sky light will do the job the sun's shadow was doing -- a
// pit is dark because little sky reaches it, not because something is in the
// way. See docs/lighting.md. One character brings them back.
#define SHADOW_ENABLED 0

// The cascade count outlives the switch: the uniform block has to match the
// one the renderer writes whether or not anything reads it.
const int SHADOW_CASCADES = 5;

#if SHADOW_ENABLED
// The filter width and the two bias terms arrive in shadow_filter, all three
// measured in shadow texels rather than world units -- a texel is a third of a
// unit in the near cascade and three and a half in the far one, and no single
// distance suits both.
const float SHADOW_SLOPE_LIMIT = 3.0;

// Where the crossfade into the next cascade starts, as a fraction of the split.
const float SHADOW_CASCADE_BLEND = 0.7;
#endif

struct DirectionalLightData {
    mat4 light_space_matrices[SHADOW_CASCADES];

    // Per cascade: x is where it ends in view depth, y is the world covered by
    // one of its shadow texels.
    vec4 cascades[SHADOW_CASCADES];

    // x: penumbra half-width in texels, y: normal offset in texels, z: how much
    // of that offset is added again per unit of slope.
    vec4 shadow_filter;

    vec3 direction;
    vec3 color;
    float intensity;

    // How far past ninety degrees the terminator is dragged. See
    // calculateDirectionalLight.
    float wrap;
};

struct FogData {
    vec3 color;
    float near_distance;
    float far_distance;
    uint enabled;
};

layout(set = 0, binding = 0) uniform UniformBufferObject {
    mat4 view;
    mat4 proj;
    vec3 viewPos;
    DirectionalLightData directional_light;
    vec4 ambient_sky;
    vec4 ambient_ground;

    // x: how far down a fully enclosed corner goes, 0 none to 1 black.
    // y: the exponent the raw 0..1 occlusion is raised to before that. Below
    //    one the shading spreads out over the whole face, above one it pulls
    //    back into the corner. The samples are integers 0..3, so this is the
    //    only control over where the middle of the ramp sits.
    vec4 ao_params;

    // rgb: what lights a place no sky reaches. Its own colour rather than a
    //      fraction of the sky, so a sealed room does not follow the time of
    //      day above it.
    vec4 cave_ambient;

    // x: the exponent the raw 0..1 sky visibility is raised to. Below one the
    //    daylight reaches further in, above one it stops at the mouth.
    vec4 sky_params;

    // rgb: what every light block in the world looks like, colour times
    //      strength. One colour for all of them, as in Minecraft: the level
    //      baked into the quad says how much, this says what.
    // w:   the exponent that level travels. Fifteen linear steps read as a
    //      ramp rather than as a lamp, so this is well above one.
    vec4 lamp_params;

    // x: how much of a block's own glow reaches the frame. Its own vec4 rather
    //    than a spare lane above, because the two are different things: that
    //    one is a light with an occluder, this is a surface that ignores every
    //    occluder there is.
    vec4 glow_params;

    // x: exposure, applied before the tone curve. y: the light level that comes
    // out as exactly one.
    vec4 tonemap_params;

    uint point_lights_count;

    // 0 normal, 1 ambient occlusion alone on white, 2 normals, 3 sky light,
    // 4 convexity, 5 block light.
    uint debug_view;

    FogData fog;
} ubo;

#if SHADOW_ENABLED
layout(set = 2, binding = 0) uniform sampler2DArrayShadow shadowMapArray;
#endif

struct PointLightData {
    vec4 position;
    vec4 color;

    // Peak at the source, 0..1, and how far it carries in world units. Two
    // numbers and not five, because the falloff below is the one the baked
    // channel uses and that is all it needs. See ecs::light_component.
    float intensity;
    float range;
};

layout(set = 3, binding = 0, std430) readonly buffer PointLights {
    PointLightData lights[];
} pointLights;

#if SHADOW_ENABLED
int selectCascade(float viewDepth) {
    for (int i = 0; i < SHADOW_CASCADES - 1; ++i) {
        if (viewDepth < ubo.directional_light.cascades[i].x) {
            return i;
        }
    }
    return SHADOW_CASCADES - 1;
}

// A Poisson disk, turned by an angle that changes from texel to texel. Four
// taps on a ring were enough while the filter was a texel and a half wide; at
// four texels they stop being a blur and become four spots, and the rotation
// that hid the kernel then shows up as boiling instead. Twelve taps fill the
// disk evenly enough that the rotation only has to break the last of the
// banding. The comparison sampler is linear, so each tap is already the
// weighted average of four compares.
const vec2 PCF_DISK[12] = vec2[12](
    vec2(-0.326, -0.406), vec2(-0.840, -0.074), vec2(-0.696,  0.457),
    vec2(-0.203,  0.621), vec2( 0.962, -0.195), vec2( 0.473, -0.480),
    vec2( 0.519,  0.767), vec2( 0.185, -0.893), vec2( 0.507,  0.064),
    vec2( 0.896,  0.412), vec2(-0.322, -0.933), vec2(-0.792, -0.598)
);

// The seed is the shadow texel, not the pixel. gl_FragCoord makes every
// surface point draw a different rotation the instant the camera moves, and
// four taps' worth of variance then boils along every shadow edge -- which
// reads as the edge itself crawling. The shadow grid is snapped to a world
// lattice, so seeding from it holds the pattern still on the surface while the
// camera moves, and still breaks up the banding between neighbouring texels.
float pcfRotation(vec2 seed) {
    return fract(sin(dot(seed, vec2(12.9898, 78.233))) * 43758.5453) * 6.2831853;
}

float calculateShadowForCascade(int cascadeIndex, vec3 normal) {
    // Если свет почти перпендикулярен поверхности, не отбрасываем тени
    float ndot = dot(normal, normalize(-ubo.directional_light.direction));
    if (ndot < 0.01) {
        return 1.0;
    }

    // The offset is measured in shadow texels, not in world units. The depth a
    // texel holds is the depth of one point in it, so a surface tilted away
    // from the light is wrong by up to a texel across its own width -- which is
    // what striped the faces and crawled when the sun moved. The old constant
    // was 0.015 world units against a texel of 0.16 to 1.28.
    float texel  = ubo.directional_light.cascades[cascadeIndex].y;
    float radius = ubo.directional_light.shadow_filter.x;
    float ndotl  = clamp(ndot, 0.05, 1.0);
    float slope  = sqrt(1.0 - ndotl * ndotl) / ndotl;

    // The slope term is multiplied by the filter width: the depth a tap reads
    // is the depth of a point up to `radius` texels away across the surface,
    // and on a slope that is off by `radius * slope` texels. A bias tuned for a
    // one-texel kernel is what turns a wide filter into stripes.
    float bias = ubo.directional_light.shadow_filter.y +
                 (radius * ubo.directional_light.shadow_filter.z *
                  min(slope, SHADOW_SLOPE_LIMIT));

    vec3 newFragPos = fragPos + fragNormal * (texel * bias);

    vec4 fragPosLightSpace =
        ubo.directional_light.light_space_matrices[cascadeIndex] * vec4(newFragPos, 1.0);

    // Проецируем координаты в диапазон [-1, 1]
    vec3 projCoords = fragPosLightSpace.xyz / fragPosLightSpace.w;

    // Преобразуем в диапазон [0, 1] только xy
    projCoords.xy = projCoords.xy * 0.5 + 0.5;

    // Проверяем, что координаты в пределах [0, 1]
    if (
        projCoords.x < 0.0 || projCoords.x > 1.0 ||
        projCoords.y < 0.0 || projCoords.y > 1.0 ||
        projCoords.z < 0.0 || projCoords.z > 1.0
    ) {
        return 1.0;
    }

    vec2 mapSize = vec2(textureSize(shadowMapArray, 0).xy);
    vec2 step    = vec2(1.0) / mapSize;

    float angle = pcfRotation(floor(projCoords.xy * mapSize));
    float sa    = sin(angle);
    float ca    = cos(angle);
    mat2 turn   = mat2(ca, -sa, sa, ca);

    float shadow = 0.0;
    for (int i = 0; i < 12; ++i) {
        vec2 offset = turn * PCF_DISK[i] * step * radius;
        shadow += texture(
            shadowMapArray,
            vec4(projCoords.xy + offset, float(cascadeIndex), projCoords.z)
        );
    }

    return shadow * (1.0 / 12.0);
}

float calculateShadow(vec3 normal, float viewDepth) {
    int cascadeIndex = selectCascade(viewDepth);

    float shadow = calculateShadowForCascade(cascadeIndex, normal);

    if (cascadeIndex < SHADOW_CASCADES - 1) {
        float nextSplit = ubo.directional_light.cascades[cascadeIndex].x;
        float blendStart = nextSplit * SHADOW_CASCADE_BLEND;
        float blendEnd = nextSplit;

        if (viewDepth > blendStart && viewDepth < blendEnd) {
            int nextCascadeIndex = cascadeIndex + 1;
            float nextShadow = calculateShadowForCascade(nextCascadeIndex, normal);

            float blendFactor = smoothstep(blendStart, blendEnd, viewDepth);
            shadow = mix(shadow, nextShadow, blendFactor);
        }
    }

    return shadow;
}
#endif

// Diffuse only. A specular lobe used to ride along at exponent 64, and on an
// untextured cube it reads as a smear of dirt rather than as a highlight:
// there is no surface detail for it to break up against, so it just moves a
// bright patch around a flat colour as the camera turns.
vec3 calculateDirectionalLight(vec3 normal, float shadow) {
    float sunElevation = -ubo.directional_light.direction.y;

    float dayFactor = smoothstep(0.2, 0.4, sunElevation);
    float twilightFactor = smoothstep(0.0, 0.2, sunElevation) * (1.0 - dayFactor);

    vec3 lightDir = normalize(-ubo.directional_light.direction);

    // Wrapped Lambert, not Lambert. Six axis-aligned normals give the dot
    // product six values and no more, and with the sun overhead three of the
    // four vertical faces get exactly zero: +X, -X and -Z then differ by
    // nothing, because the ambient below reads normal.y and cannot separate
    // them either. Wrapping spreads the terminator past ninety degrees and
    // hands each of them a different number. A face pointing fully away still
    // gets nothing -- the cutoff moves out to a hundred and twenty degrees, it
    // does not disappear.
    float wrap = ubo.directional_light.wrap;
    float diff = max((dot(lightDir, normal) + wrap) / (1.0 + wrap), 0.0);

    vec3 sunColor = ubo.directional_light.color * ubo.directional_light.intensity;
    vec3 twilightColor = vec3(1.0, 0.5, 0.2) * ubo.directional_light.intensity * 0.3;

    return diff * shadow * (sunColor * dayFactor + twilightColor * twilightFactor);
}

// The same falloff the baked channel has, so that a torch thrown to the ground
// and turned into a block does not change brightness at the moment it lands.
// That is a requirement of the stage, not a nicety, and the only way to meet it
// is to run one function rather than to tune two into agreement.
//
// The baked level at distance d from a source of emission L is (L - d) / 15,
// which is intensity * (1 - d / range) once intensity is L / 15 and range is
// L times the world's voxel size. Then the same curve on top.
vec3 calculatePointLight(uint lightIndex, vec3 fragPos) {
    PointLightData light = pointLights.lights[lightIndex];

    vec3 offset = light.position.xyz - fragPos;

    // Manhattan, not Euclidean, and this is the other half of the stitch. The
    // flood steps to six neighbours one level at a time, so what it measures is
    // |dx|+|dy|+|dz| and its pools of light are diamonds, not spheres -- the
    // same shape Minecraft's are, for the same reason. A torch that lit a
    // sphere in the air and a diamond on the ground would visibly change shape
    // on landing: along the body diagonal the two differ by a factor of 1.73.
    float reach = abs(offset.x) + abs(offset.y) + abs(offset.z);

    float raw = light.intensity * max(1.0 - (reach / max(light.range, 0.001)), 0.0);
    if (raw <= 0.0) {
        return vec3(0.0);
    }

    // No cosine, and that is not an oversight. A Lambert term was tried here on
    // the theory that it stands in for the geometry a dynamic light cannot
    // walk around; it does not, and it broke the very match this function
    // exists for. The baked channel has no directional term at all -- its shape
    // comes entirely from how far the flood had to travel -- so a flat floor
    // beside a lamp block is evenly lit, and a flat floor under a torch has to
    // be evenly lit too. With the cosine in, a torch two voxels up lit the
    // floor eight voxels out at 0.017 where the block gave 0.160.
    //
    // No specular either. On a flat-coloured cube pow(dot, 64) reads as grease
    // rather than as shine, which is why it left the directional term as well.
    return light.color.xyz * pow(raw, ubo.lamp_params.w);
}

// Everything that is not a light: sky from above, ground bounce from below.
// With no textures this is all that separates one face of a block from another
// wherever the sun does not reach, so it has to be a hemisphere -- a flat
// ambient leaves an unlit voxel a single flat colour.
//
// Both colours come from the frame uniform rather than from constants here.
// They used to be hardcoded daylight, so midnight was lit by an afternoon sky.
vec3 calculateHemisphereAmbient(vec3 normal) {
    return mix(ubo.ambient_ground.rgb, ubo.ambient_sky.rgb, normal.y * 0.5 + 0.5);
}

// Выходной цвет пикселя
layout(location = 0) out vec4 outColor;

void main() {
    vec3 normal = normalize(fragNormal);

#if SHADOW_ENABLED
    float sunElevation = -ubo.directional_light.direction.y;
    float shadowReliability = smoothstep(0.15, 0.3, sunElevation);
    float shadow = mix(1.0, calculateShadow(normal, viewDepth), shadowReliability);
#else
    float shadow = 1.0;
#endif

    // Two bits a corner, 0 open to 3 fully enclosed, interpolated across the
    // rectangle by hand rather than by the rasteriser. That is deliberate: a
    // quad whose corner values differ across a diagonal cannot be drawn by two
    // triangles without the split showing, and every voxel engine that
    // interpolates AO per vertex has to pick which way to cut it. Doing the
    // bilinear here means there is no diagonal to pick.
    uint m = fragCornersMask;
    float a00 = float( m        & 3u) * (1.0 / 3.0);
    float a10 = float((m >> 2)  & 3u) * (1.0 / 3.0);
    float a11 = float((m >> 4)  & 3u) * (1.0 / 3.0);
    float a01 = float((m >> 6)  & 3u) * (1.0 / 3.0);

    float occlusion = mix(mix(a00, a10, fragUV.x), mix(a01, a11, fragUV.x), fragUV.y);
    occlusion = pow(occlusion, ubo.ao_params.y);

    // The other half of the same question, and the reason a hillside reads as a
    // hillside. Occlusion looks at the layer in front of the face, and above a
    // top face that layer is air by definition -- so under open sky, where the
    // light is a flat fifteen everywhere, two plateaus at different heights come
    // out the same colour and the step between them disappears. This looks at
    // the layer the face stands in and finds the drop. Top faces only: the mask
    // is zero on the other five, which is why nothing else changes.
    uint cm = fragConvexMask;
    float x00 = float( cm        & 3u) * (1.0 / 3.0);
    float x10 = float((cm >> 2)  & 3u) * (1.0 / 3.0);
    float x11 = float((cm >> 4)  & 3u) * (1.0 / 3.0);
    float x01 = float((cm >> 6)  & 3u) * (1.0 / 3.0);

    float exposure = mix(mix(x00, x10, fragUV.x), mix(x01, x11, fragUV.x), fragUV.y);
    exposure = pow(exposure, ubo.ao_params.w);

    if (ubo.debug_view == 4u) {
        outColor = vec4(vec3(exposure), 1.0);
        return;
    }

    // Two factors, not one. Occlusion measures how much of the surroundings a
    // point can see, so it belongs to the light arriving from the surroundings
    // and to nothing else. Convexity measures nothing -- it is a shape cue
    // standing in for curvature a cube does not have -- so tying it to the
    // ambient alone was arbitrary, and under a high sun the ambient is a
    // quarter of a top face. That is where the cue was going.
    float aoFactor     = 1.0 - (occlusion * ubo.ao_params.x);
    float convexFactor = 1.0 + (exposure * ubo.ao_params.z);

    if (ubo.debug_view == 1u) {
        outColor = vec4(vec3(aoFactor), 1.0);
        return;
    }
    if (ubo.debug_view == 2u) {
        outColor = vec4((normal * 0.5) + 0.5, 1.0);
        return;
    }

    // Sky light, four bits a corner, through the same bilinear as AO and for the
    // same reason: corner values that differ across a diagonal cannot be split
    // into two triangles without the split showing.
    uint sm = fragLightMask & 0xFFFFu;
    float s00 = float( sm        & 15u) * (1.0 / 15.0);
    float s10 = float((sm >> 4)  & 15u) * (1.0 / 15.0);
    float s11 = float((sm >> 8)  & 15u) * (1.0 / 15.0);
    float s01 = float((sm >> 12) & 15u) * (1.0 / 15.0);

    float skyRaw = mix(mix(s00, s10, fragUV.x), mix(s01, s11, fragUV.x), fragUV.y);

    // Two curves off the one number, because the sky and the sun do not arrive
    // the same way. Sky light is a flood: it bends round a corner, and fifteen
    // voxels into a cave mouth there really is some daylight. The sun does not
    // bend at all, so where the sky is at a half the sun should already be most
    // of the way gone. One exponent each, and the sun's is the steeper.
    float skyReach = pow(skyRaw, ubo.sky_params.x);
    float sunReach = pow(skyRaw, ubo.sky_params.y);

    if (ubo.debug_view == 3u) {
        outColor = vec4(vec3(skyReach), 1.0);
        return;
    }

    // Block light, the other half of the same word, through the same bilinear
    // and for the same reason. Separate from the sky half all the way down:
    // this one is a light and the one above is a visibility, so the time of day
    // multiplies that one and must never touch this one -- summed at bake time
    // a lamp would go out at dusk.
    uint bm = fragLightMask >> 16;
    float l00 = float( bm        & 15u) * (1.0 / 15.0);
    float l10 = float((bm >> 4)  & 15u) * (1.0 / 15.0);
    float l11 = float((bm >> 8)  & 15u) * (1.0 / 15.0);
    float l01 = float((bm >> 12) & 15u) * (1.0 / 15.0);

    float lampRaw   = mix(mix(l00, l10, fragUV.x), mix(l01, l11, fragUV.x), fragUV.y);
    float lampReach = pow(lampRaw, ubo.lamp_params.w);

    if (ubo.debug_view == 5u) {
        outColor = vec4(vec3(lampReach), 1.0);
        return;
    }

    // AO belongs to the ambient term alone. It is a measure of how much of the
    // surroundings a point can see, so it dims the light that arrives from the
    // surroundings; laying it over the sun as well counts the same occlusion
    // twice and grimes up every corner the sun is shining straight into.
    //
    // Sky light is the occluder of everything that comes from the sky, and that
    // is both terms below, not just the ambient one.
    //
    // It used to scale only the ambient, on the grounds that the shadow map
    // already said where the sun could not reach. That was true while there was
    // a shadow map. There is not: SHADOW_ENABLED is off, shadow is a constant
    // one, and the sun was landing on the walls of sealed caves -- so a room the
    // daylight cannot enter still brightened and dimmed as the day turned over
    // the ground above it.
    //
    // Sky light is now the sun's only occluder, which is what taking the
    // cascades out was for. Turning SHADOW_ENABLED back on would put two
    // occluders on one light and darken every overhang twice; whichever of the
    // two is kept, it has to be one.
    vec3 sky = mix(ubo.cave_ambient.rgb, calculateHemisphereAmbient(normal), skyReach);
    vec3 ambient = sky * aoFactor;

    // AO is deliberately not here. It measures how much of the surroundings a
    // point can see, so it belongs to the light that arrives from the
    // surroundings; over the sun it counts the same occlusion twice and grimes
    // up every corner the sun shines straight into.
    vec3 directional = calculateDirectionalLight(normal, shadow) * sunReach;

    // AO is here, and belongs here: a baked lamp is light arriving from the
    // surroundings exactly as the sky is, and the corner it cannot reach into
    // is the same corner. No time of day anywhere in this term -- that is the
    // whole of why the channel is kept apart.
    vec3 lamp = ubo.lamp_params.rgb * lampReach * aoFactor;

    // Point lights
    vec3 pointLighting = vec3(0.0);
    for (uint i = 0; i < ubo.point_lights_count; i++) {
        pointLighting += calculatePointLight(i, fragPos);
    }

    // No rim term and no edge term. Both sat at 0.01, which is invisible, and
    // the edge one never reached the sum at all. Nothing here is a stand-in for
    // something else any more: every term is a light with an occluder.
    vec3 lighting = (ambient + directional + lamp + pointLighting) * convexFactor;
    vec3 result = lighting * fragColor.rgb;

    // Emissive, and outside everything on purpose. No AO, no convexity, no sky
    // light, no time of day: a glowing voxel is not lit, it is a source, and
    // dimming a source by how shut in it is would be saying that lava shines
    // less in a corner.
    //
    // Its own albedo is the colour, so lava glows lava-coloured and a crystal
    // glows its own. Inside the tone curve rather than after it, because it has
    // to roll off the way everything else does -- a source that skipped the
    // curve would be the one thing left in the frame still able to clip.
    result += fragColor.rgb * fragColor.a * ubo.glow_params.x;

    // Extended Reinhard, per channel. Per channel and not on luminance:
    // scaling three channels by one luminance ratio does not stop the brightest
    // of them clipping, and (1.5, 0.3, 0.3) has a luminance of 0.55 yet still
    // comes out over one. Rolling each channel on its own cannot clip below the
    // white point, at the price of extreme highlights drifting toward white --
    // which is what film does, and a far smaller drift than the hard cut it
    // replaces.
    result *= ubo.tonemap_params.x;
    vec3 white = vec3(ubo.tonemap_params.y);
    result = result * (1.0 + (result / (white * white))) / (1.0 + result);

    // Fog after the curve, not before. It converges on the same colour the
    // frame is cleared to, and mapping the terrain but not the clear would open
    // a seam along the horizon.
    if (ubo.fog.enabled != 0u) {
        float fogFactor = clamp(
            (ubo.fog.far_distance - viewDepth) / (ubo.fog.far_distance - ubo.fog.near_distance),
            0.0, 1.0
        );
        result = mix(ubo.fog.color, result, fogFactor);
    }

    outColor = vec4(result, 1.0);
}
