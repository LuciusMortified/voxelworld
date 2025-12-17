#pragma once

#ifndef VW_GFX_APP_H
#define VW_GFX_APP_H

#include <stdexcept>

namespace vw::gfx {

template<typename WC>
class engine;

template<class WC = base_world_components>
class app {
public:
    using engine_type = engine<WC>;

    app()          = default;
    virtual ~app() = default;

    app(const app&)                    = delete;
    auto operator=(const app&) -> app& = delete;

    virtual void setup() {}

    virtual void cleanup() {}

    virtual void render([[maybe_unused]] float delta_time) {}

    void run(engine_type& engine) {
        engine_ = &engine;
    }

protected:
    [[nodiscard]] auto get_engine() const -> engine_type& {
        if (engine_ == nullptr) {
            throw std::runtime_error("engine not set in app");
        }
        return *engine_;
    }

private:
    engine_type* engine_ = nullptr;
};
}  // namespace vw::gfx

#endif  // VW_GFX_APP_H
