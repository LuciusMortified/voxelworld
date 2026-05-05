#pragma once

#ifndef VW_GFX_APP_H
#define VW_GFX_APP_H

#include <stdexcept>

#include "vw/ecs/base_world_def.h"

namespace vw::gfx {

using namespace ::vw::ecs;

template <typename WC>
class engine;

template <typename WC = base_world_def>
class app {
public:
    using engine_type = engine<WC>;

    explicit app(
        engine_type& eng
    )
        : engine_(&eng) {}

    virtual ~app() = default;

    app(const app&)                    = delete;
    auto operator=(const app&) -> app& = delete;

    virtual void render(
        [[maybe_unused]] float delta_time
    ) {}

protected:
    [[nodiscard]] auto get_engine() const -> engine_type& {
        return *engine_;
    }

private:
    engine_type* engine_ = nullptr;
};
}  // namespace vw::gfx

#endif  // VW_GFX_APP_H
