#include <vw/core.h>
#include <vw/log.h>
#include <cassert>

using namespace vw;

void test_constexpr() {
    log::info("=== Testing constexpr ===");

    // constexpr конструкторы
    constexpr vec3f cv1{1.0f, 2.0f, 3.0f};
    constexpr vec3f cv2{4.0f, 5.0f, 6.0f};

    // constexpr операции
    constexpr vec3f cv_sum = cv1 + cv2;
    static_assert(cv_sum.x == 5.0f && cv_sum.y == 7.0f && cv_sum.z == 9.0f, "constexpr vec3 addition");

    constexpr vec3f cv_diff = cv2 - cv1;
    static_assert(cv_diff.x == 3.0f && cv_diff.y == 3.0f && cv_diff.z == 3.0f, "constexpr vec3 subtraction");

    constexpr vec3f cv_scale = cv1 * 2.0f;
    static_assert(cv_scale.x == 2.0f && cv_scale.y == 4.0f && cv_scale.z == 6.0f, "constexpr vec3 scale");

    constexpr vec3f cv_div = cv2 / 2.0f;
    static_assert(cv_div.x == 2.0f && cv_div.y == 2.5f && cv_div.z == 3.0f, "constexpr vec3 division");

    // constexpr radians/degrees
    constexpr float rad90 = math::radians(90.0f);
    constexpr float deg90 = math::degrees(rad90);
    static_assert(deg90 > 89.9f && deg90 < 90.1f, "constexpr radians/degrees");

    log::info("All constexpr tests passed!");
}

void test_type_conversion() {
    log::info("=== Testing type conversion ===");

    // vec3i -> vec3f
    vec3i vi{10, 20, 30};
    vec3f vf(vi);
    assert(vf.x == 10.0f && vf.y == 20.0f && vf.z == 30.0f);
    log::info("vec3i -> vec3f: OK ({}, {}, {})", vf.x, vf.y, vf.z);

    // vec3f -> vec3i (truncation)
    vec3f vf2{1.7f, 2.3f, 3.9f};
    vec3i vi2(vf2);
    assert(vi2.x == 1 && vi2.y == 2 && vi2.z == 3);
    log::info("vec3f -> vec3i: OK ({}, {}, {})", vi2.x, vi2.y, vi2.z);

    // vec2 conversion
    vec2i v2i{5, 10};
    vec2f v2f(v2i);
    assert(v2f.x == 5.0f && v2f.y == 10.0f);
    log::info("vec2i -> vec2f: OK ({}, {})", v2f.x, v2f.y);

    // vec4 conversion
    vec4i v4i{1, 2, 3, 4};
    vec4f v4f(v4i);
    assert(v4f.x == 1.0f && v4f.y == 2.0f && v4f.z == 3.0f && v4f.w == 4.0f);
    log::info("vec4i -> vec4f: OK ({}, {}, {}, {})", v4f.x, v4f.y, v4f.z, v4f.w);

    log::info("All type conversion tests passed!");
}

void test_operators() {
    log::info("=== Testing new operators ===");

    // Division operators
    vec3f v1{10.0f, 20.0f, 30.0f};
    vec3f v_div = v1 / 2.0f;
    assert(v_div.x == 5.0f && v_div.y == 10.0f && v_div.z == 15.0f);
    log::info("vec3 / scalar: OK ({}, {}, {})", v_div.x, v_div.y, v_div.z);

    // Left multiplication
    vec3f v_left = 2.0f * v1;
    assert(v_left.x == 20.0f && v_left.y == 40.0f && v_left.z == 60.0f);
    log::info("scalar * vec3: OK ({}, {}, {})", v_left.x, v_left.y, v_left.z);

    // Component-wise multiplication
    vec3f v2{2.0f, 3.0f, 4.0f};
    vec3f v_hadamard = v1 * v2;
    assert(v_hadamard.x == 20.0f && v_hadamard.y == 60.0f && v_hadamard.z == 120.0f);
    log::info("vec3 * vec3 (component-wise): OK ({}, {}, {})", v_hadamard.x, v_hadamard.y, v_hadamard.z);

    // Component-wise division
    vec3f v_cdiv = v1 / v2;
    assert(v_cdiv.x == 5.0f && math::approx_equal(v_cdiv.y, 6.666f, 0.01f) && v_cdiv.z == 7.5f);
    log::info("vec3 / vec3 (component-wise): OK ({}, {}, {})", v_cdiv.x, v_cdiv.y, v_cdiv.z);

    // Index access
    vec3f v3{1.0f, 2.0f, 3.0f};
    assert(v3[0] == 1.0f && v3[1] == 2.0f && v3[2] == 3.0f);
    v3[1] = 100.0f;
    assert(v3[1] == 100.0f);
    log::info("vec3 operator[]: OK");

    // Compound assignment operators
    vec3f v4{1.0f, 2.0f, 3.0f};
    v4 -= vec3f{0.5f, 0.5f, 0.5f};
    assert(v4.x == 0.5f && v4.y == 1.5f && v4.z == 2.5f);
    log::info("vec3 operator-=: OK ({}, {}, {})", v4.x, v4.y, v4.z);

    v4 *= 2.0f;
    assert(v4.x == 1.0f && v4.y == 3.0f && v4.z == 5.0f);
    log::info("vec3 operator*= (scalar): OK ({}, {}, {})", v4.x, v4.y, v4.z);

    v4 /= 2.0f;
    assert(v4.x == 0.5f && v4.y == 1.5f && v4.z == 2.5f);
    log::info("vec3 operator/= (scalar): OK ({}, {}, {})", v4.x, v4.y, v4.z);

    log::info("All operator tests passed!");
}

void test_vec2_functions() {
    log::info("=== Testing vec2 functions ===");

    vec2f v1{3.0f, 4.0f};

    // length
    float len = math::length(v1);
    assert(math::approx_equal(len, 5.0f));
    log::info("vec2 length: OK ({})", len);

    // length_squared
    float len_sq = math::length_squared(v1);
    assert(math::approx_equal(len_sq, 25.0f));
    log::info("vec2 length_squared: OK ({})", len_sq);

    // normalize
    vec2f n = math::normalize(v1);
    assert(math::approx_equal(n.x, 0.6f) && math::approx_equal(n.y, 0.8f));
    log::info("vec2 normalize: OK ({}, {})", n.x, n.y);

    // dot
    vec2f v2{1.0f, 0.0f};
    float d = math::dot(v1, v2);
    assert(math::approx_equal(d, 3.0f));
    log::info("vec2 dot: OK ({})", d);

    // cross (returns scalar)
    vec2f a{1.0f, 0.0f};
    vec2f b{0.0f, 1.0f};
    float cross_result = math::cross(a, b);
    assert(math::approx_equal(cross_result, 1.0f));
    log::info("vec2 cross: OK ({})", cross_result);

    // lerp
    vec2f lerp_result = math::lerp(vec2f{0.0f, 0.0f}, vec2f{10.0f, 10.0f}, 0.5f);
    assert(math::approx_equal(lerp_result.x, 5.0f) && math::approx_equal(lerp_result.y, 5.0f));
    log::info("vec2 lerp: OK ({}, {})", lerp_result.x, lerp_result.y);

    // min/max
    vec2f min_result = math::min(vec2f{1.0f, 5.0f}, vec2f{3.0f, 2.0f});
    assert(min_result.x == 1.0f && min_result.y == 2.0f);
    log::info("vec2 min: OK ({}, {})", min_result.x, min_result.y);

    vec2f max_result = math::max(vec2f{1.0f, 5.0f}, vec2f{3.0f, 2.0f});
    assert(max_result.x == 3.0f && max_result.y == 5.0f);
    log::info("vec2 max: OK ({}, {})", max_result.x, max_result.y);

    log::info("All vec2 function tests passed!");
}

void test_vec4_functions() {
    log::info("=== Testing vec4 functions ===");

    vec4f v1{1.0f, 2.0f, 2.0f, 0.0f};

    // length
    float len = math::length(v1);
    assert(math::approx_equal(len, 3.0f));
    log::info("vec4 length: OK ({})", len);

    // normalize
    vec4f n = math::normalize(v1);
    assert(math::approx_equal(math::length(n), 1.0f));
    log::info("vec4 normalize: OK ({}, {}, {}, {})", n.x, n.y, n.z, n.w);

    // dot
    vec4f v2{1.0f, 0.0f, 0.0f, 0.0f};
    float d = math::dot(v1, v2);
    assert(math::approx_equal(d, 1.0f));
    log::info("vec4 dot: OK ({})", d);

    // lerp
    vec4f lerp_result = math::lerp(vec4f{0.0f, 0.0f, 0.0f, 0.0f}, vec4f{10.0f, 20.0f, 30.0f, 40.0f}, 0.5f);
    assert(math::approx_equal(lerp_result.x, 5.0f));
    assert(math::approx_equal(lerp_result.y, 10.0f));
    assert(math::approx_equal(lerp_result.z, 15.0f));
    assert(math::approx_equal(lerp_result.w, 20.0f));
    log::info("vec4 lerp: OK ({}, {}, {}, {})", lerp_result.x, lerp_result.y, lerp_result.z, lerp_result.w);

    log::info("All vec4 function tests passed!");
}

void test_vec3_utilities() {
    log::info("=== Testing vec3 utilities ===");

    // distance
    vec3f a{0.0f, 0.0f, 0.0f};
    vec3f b{3.0f, 4.0f, 0.0f};
    float dist = math::distance(a, b);
    assert(math::approx_equal(dist, 5.0f));
    log::info("vec3 distance: OK ({})", dist);

    // reflect
    vec3f incident{1.0f, -1.0f, 0.0f};
    vec3f normal{0.0f, 1.0f, 0.0f};
    vec3f reflected = math::reflect(incident, normal);
    assert(math::approx_equal(reflected.x, 1.0f) && math::approx_equal(reflected.y, 1.0f));
    log::info("vec3 reflect: OK ({}, {}, {})", reflected.x, reflected.y, reflected.z);

    // project
    vec3f v{3.0f, 4.0f, 0.0f};
    vec3f onto{1.0f, 0.0f, 0.0f};
    vec3f projected = math::project(v, onto);
    assert(math::approx_equal(projected.x, 3.0f) && math::approx_equal(projected.y, 0.0f));
    log::info("vec3 project: OK ({}, {}, {})", projected.x, projected.y, projected.z);

    // reject
    vec3f rejected = math::reject(v, onto);
    assert(math::approx_equal(rejected.x, 0.0f) && math::approx_equal(rejected.y, 4.0f));
    log::info("vec3 reject: OK ({}, {}, {})", rejected.x, rejected.y, rejected.z);

    // angle
    vec3f x_axis{1.0f, 0.0f, 0.0f};
    vec3f y_axis{0.0f, 1.0f, 0.0f};
    float angle_rad = math::angle(x_axis, y_axis);
    assert(math::approx_equal(angle_rad, math::pi / 2.0f, 0.01f));
    log::info("vec3 angle: OK ({} rad = {} deg)", angle_rad, math::degrees(angle_rad));

    // abs, floor, ceil, round
    vec3f v_neg{-1.5f, -2.3f, 3.7f};
    vec3f v_abs = math::abs(v_neg);
    assert(v_abs.x == 1.5f && v_abs.y == 2.3f && v_abs.z == 3.7f);
    log::info("vec3 abs: OK ({}, {}, {})", v_abs.x, v_abs.y, v_abs.z);

    vec3f v_floor = math::floor(v_neg);
    assert(v_floor.x == -2.0f && v_floor.y == -3.0f && v_floor.z == 3.0f);
    log::info("vec3 floor: OK ({}, {}, {})", v_floor.x, v_floor.y, v_floor.z);

    vec3f v_ceil = math::ceil(v_neg);
    assert(v_ceil.x == -1.0f && v_ceil.y == -2.0f && v_ceil.z == 4.0f);
    log::info("vec3 ceil: OK ({}, {}, {})", v_ceil.x, v_ceil.y, v_ceil.z);

    // fract
    vec3f v_fract = math::fract(vec3f{1.7f, 2.3f, -0.5f});
    assert(math::approx_equal(v_fract.x, 0.7f, 0.01f));
    assert(math::approx_equal(v_fract.y, 0.3f, 0.01f));
    assert(math::approx_equal(v_fract.z, 0.5f, 0.01f));  // fract(-0.5) = 0.5
    log::info("vec3 fract: OK ({}, {}, {})", v_fract.x, v_fract.y, v_fract.z);

    // sign
    vec3f v_sign = math::sign(vec3f{-5.0f, 0.0f, 3.0f});
    assert(v_sign.x == -1.0f && v_sign.y == 0.0f && v_sign.z == 1.0f);
    log::info("vec3 sign: OK ({}, {}, {})", v_sign.x, v_sign.y, v_sign.z);

    // Template sign for scalars
    assert(math::sign(-10.0f) == -1.0f);
    assert(math::sign(0.0f) == 0.0f);
    assert(math::sign(10.0f) == 1.0f);
    assert(math::sign(-5) == -1);
    assert(math::sign(0) == 0);
    assert(math::sign(5) == 1);
    log::info("sign<T>: OK");

    log::info("All vec3 utility tests passed!");
}

void test_mat4_operators() {
    log::info("=== Testing mat4 operators ===");

    mat4f m1 = math::identity_matrix();
    mat4f m2 = math::identity_matrix();

    // Addition
    mat4f m_sum = m1 + m2;
    float sum_00 = m_sum[0, 0];
    float sum_11 = m_sum[1, 1];
    assert(sum_00 == 2.0f && sum_11 == 2.0f);
    log::info("mat4 operator+: OK");

    // Subtraction
    mat4f m_diff = m1 - m2;
    float diff_00 = m_diff[0, 0];
    float diff_11 = m_diff[1, 1];
    assert(diff_00 == 0.0f && diff_11 == 0.0f);
    log::info("mat4 operator-: OK");

    // Scalar multiplication
    mat4f m_scale = m1 * 3.0f;
    float scale_00 = m_scale[0, 0];
    float scale_11 = m_scale[1, 1];
    assert(scale_00 == 3.0f && scale_11 == 3.0f);
    log::info("mat4 * scalar: OK");

    // Left scalar multiplication
    mat4f m_left = 2.0f * m1;
    float left_00 = m_left[0, 0];
    float left_11 = m_left[1, 1];
    assert(left_00 == 2.0f && left_11 == 2.0f);
    log::info("scalar * mat4: OK");

    // Scalar division
    mat4f m_div = m_scale / 3.0f;
    float div_00 = m_div[0, 0];
    float div_11 = m_div[1, 1];
    assert(math::approx_equal(div_00, 1.0f) && math::approx_equal(div_11, 1.0f));
    log::info("mat4 / scalar: OK");

    // Unary minus
    mat4f m_neg = -m1;
    float neg_00 = m_neg[0, 0];
    float neg_11 = m_neg[1, 1];
    assert(neg_00 == -1.0f && neg_11 == -1.0f);
    log::info("mat4 unary minus: OK");

    // Compound operators
    mat4f m3 = math::identity_matrix();
    m3 += m1;
    float m3_00 = m3[0, 0];
    assert(m3_00 == 2.0f);
    log::info("mat4 operator+=: OK");

    m3 -= m1;
    m3_00 = m3[0, 0];
    assert(m3_00 == 1.0f);
    log::info("mat4 operator-=: OK");

    m3 *= 2.0f;
    m3_00 = m3[0, 0];
    assert(m3_00 == 2.0f);
    log::info("mat4 operator*= (scalar): OK");

    m3 /= 2.0f;
    m3_00 = m3[0, 0];
    assert(m3_00 == 1.0f);
    log::info("mat4 operator/= (scalar): OK");

    log::info("All mat4 operator tests passed!");
}

void test_approx_equal() {
    log::info("=== Testing approx_equal ===");

    // Float - разница меньше epsilon (1e-5)
    assert(math::approx_equal(1.0f, 1.000001f));
    assert(!math::approx_equal(1.0f, 1.1f));
    log::info("approx_equal(float, float): OK");

    // Vec2
    assert(math::approx_equal(vec2f{1.0f, 2.0f}, vec2f{1.000001f, 2.000001f}));
    assert(!math::approx_equal(vec2f{1.0f, 2.0f}, vec2f{1.1f, 2.0f}));
    log::info("approx_equal(vec2f, vec2f): OK");

    // Vec3
    assert(math::approx_equal(vec3f{1.0f, 2.0f, 3.0f}, vec3f{1.000001f, 2.000001f, 3.000001f}));
    assert(!math::approx_equal(vec3f{1.0f, 2.0f, 3.0f}, vec3f{1.0f, 2.0f, 3.1f}));
    log::info("approx_equal(vec3f, vec3f): OK");

    // Vec4
    assert(math::approx_equal(vec4f{1.0f, 2.0f, 3.0f, 4.0f}, vec4f{1.000001f, 2.000001f, 3.000001f, 4.000001f}));
    log::info("approx_equal(vec4f, vec4f): OK");

    // Mat4
    mat4f m1 = math::identity_matrix();
    mat4f m2 = math::identity_matrix();
    m2[0] = 1.000001f;  // m2[0,0] через одномерный доступ
    assert(math::approx_equal(m1, m2));
    log::info("approx_equal(mat4f, mat4f): OK");

    // Quat
    quat q1{0.0f, 0.0f, 0.0f, 1.0f};
    quat q2{0.000001f, 0.000001f, 0.000001f, 1.000001f};
    assert(math::approx_equal(q1, q2));
    log::info("approx_equal(quat, quat): OK");

    log::info("All approx_equal tests passed!");
}

void test_template_clamp() {
    log::info("=== Testing template clamp ===");

    // float
    float f = math::clamp(5.0f, 0.0f, 10.0f);
    assert(f == 5.0f);
    f = math::clamp(-5.0f, 0.0f, 10.0f);
    assert(f == 0.0f);
    f = math::clamp(15.0f, 0.0f, 10.0f);
    assert(f == 10.0f);
    log::info("clamp<float>: OK");

    // int
    int i = math::clamp(5, 0, 10);
    assert(i == 5);
    i = math::clamp(-5, 0, 10);
    assert(i == 0);
    i = math::clamp(15, 0, 10);
    assert(i == 10);
    log::info("clamp<int>: OK");

    // double
    double d = math::clamp(5.0, 0.0, 10.0);
    assert(d == 5.0);
    log::info("clamp<double>: OK");

    // vec3f clamp
    vec3f v = math::clamp(vec3f{-1.0f, 5.0f, 15.0f}, vec3f{0.0f, 0.0f, 0.0f}, vec3f{10.0f, 10.0f, 10.0f});
    assert(v.x == 0.0f && v.y == 5.0f && v.z == 10.0f);
    log::info("clamp(vec3f): OK ({}, {}, {})", v.x, v.y, v.z);

    log::info("All template clamp tests passed!");
}

void test_smoothstep_mix() {
    log::info("=== Testing smoothstep and mix ===");

    // smoothstep scalar
    float s0 = math::smoothstep(0.0f, 1.0f, 0.0f);  // edge0
    float s1 = math::smoothstep(0.0f, 1.0f, 1.0f);  // edge1
    float s_mid = math::smoothstep(0.0f, 1.0f, 0.5f);  // middle
    assert(math::approx_equal(s0, 0.0f));
    assert(math::approx_equal(s1, 1.0f));
    assert(math::approx_equal(s_mid, 0.5f, 0.01f));  // Hermite curve at 0.5 is 0.5
    log::info("smoothstep(float): OK (0={}, 0.5={}, 1={})", s0, s_mid, s1);

    // smoothstep vec3
    vec3f edge0{0.0f, 0.0f, 0.0f};
    vec3f edge1{1.0f, 2.0f, 4.0f};
    vec3f x{0.5f, 1.0f, 2.0f};
    vec3f s_vec = math::smoothstep(edge0, edge1, x);
    assert(math::approx_equal(s_vec.x, 0.5f, 0.01f));
    assert(math::approx_equal(s_vec.y, 0.5f, 0.01f));
    assert(math::approx_equal(s_vec.z, 0.5f, 0.01f));
    log::info("smoothstep(vec3f): OK ({}, {}, {})", s_vec.x, s_vec.y, s_vec.z);

    // mix (синоним lerp)
    float m_f = math::mix(0.0f, 10.0f, 0.5f);
    assert(math::approx_equal(m_f, 5.0f));
    log::info("mix(float): OK ({})", m_f);

    vec3f m_v = math::mix(vec3f{0.0f, 0.0f, 0.0f}, vec3f{10.0f, 20.0f, 30.0f}, 0.5f);
    assert(math::approx_equal(m_v.x, 5.0f));
    assert(math::approx_equal(m_v.y, 10.0f));
    assert(math::approx_equal(m_v.z, 15.0f));
    log::info("mix(vec3f): OK ({}, {}, {})", m_v.x, m_v.y, m_v.z);

    log::info("All smoothstep/mix tests passed!");
}

int main() {
    log::info("========================================");
    log::info("  VW Math Library Tests");
    log::info("========================================");

    test_constexpr();
    test_type_conversion();
    test_operators();
    test_vec2_functions();
    test_vec4_functions();
    test_vec3_utilities();
    test_mat4_operators();
    test_approx_equal();
    test_template_clamp();
    test_smoothstep_mix();

    log::info("========================================");
    log::info("  ALL TESTS PASSED!");
    log::info("========================================");

    return 0;
}