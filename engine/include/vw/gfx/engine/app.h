#pragma once

#ifndef VW_GFX_APP_H
#define VW_GFX_APP_H

#include <stdexcept>

namespace vw::gfx {

class engine;

class app {
public:
    app()          = default;
    virtual ~app() = default;

    app(const app&)                    = delete;
    auto operator=(const app&) -> app& = delete;

    virtual void setup() {}

    virtual void cleanup() {}

    virtual void render([[maybe_unused]] float delta_time) {}

    void run(engine& engine) {
        engine_ = &engine;
    }

protected:
    [[nodiscard]] auto get_engine() const -> engine& {
        if (engine_ == nullptr) {
            throw std::runtime_error("engine not set in app");
        }
        return *engine_;
    }

private:
    engine* engine_ = nullptr;
};
}  // namespace vw::gfx

#endif  // VW_GFX_APP_H
