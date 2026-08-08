#pragma once

#ifndef VW_GFX_APP_H
#define VW_GFX_APP_H

#include <stdexcept>


namespace vw::gfx {

using namespace ::vw::ecs;

class engine;

class app {
public:
    using engine_type = engine;

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
