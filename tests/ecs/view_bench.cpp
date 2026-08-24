#include <chrono>
#include <cstdio>

import vw.core;
import vw.ecs;

using namespace vw;
using namespace vw::ecs;

namespace {

struct pos_component {
    float x = 0.f, y = 0.f, z = 0.f;
};

struct vel_component {
    float dx = 0.f, dy = 0.f, dz = 0.f;
};

struct tag_component {
    int id = 0;
};

constexpr int entity_count = 200000;
constexpr int rounds       = 200;

template <typename Fn>
auto measure(const char* label, Fn&& body) -> void {
    double sum       = 0.0;
    const auto start = std::chrono::steady_clock::now();
    for (int r = 0; r < rounds; ++r) {
        body(sum);
    }
    const double ms =
        std::chrono::duration<double, std::milli>(std::chrono::steady_clock::now() - start)
            .count();
    std::printf("%-10s %.3f ms/round, checksum %.0f\n", label, ms / rounds, sum);
}

}  // namespace

auto main() -> int {
    registry reg;

    for (int i = 0; i < entity_count; ++i) {
        auto e = reg.create();
        reg.add<pos_component>(e, {static_cast<float>(i), 0.f, 0.f});
        if (i % 5 != 0) {
            reg.add<vel_component>(e, {1.f, 0.f, 0.f});
        }
        if (i % 3 == 0) {
            reg.add<tag_component>(e, {i});
        }
    }

    measure("view", [&](double& sum) {
        for (const auto& [ent, p, vl] : reg.view<pos_component, vel_component>()) {
            sum += static_cast<double>(p.x) * vl.dx;
        }
    });

    measure("for_each", [&](double& sum) {
        reg.for_each<pos_component, vel_component>(
            [&](entity, const pos_component& p, const vel_component& vl) {
                sum += static_cast<double>(p.x) * vl.dx;
            });
    });

    return 0;
}
