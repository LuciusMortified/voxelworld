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

    app(const app&)            = delete;
    app& operator=(const app&) = delete;

    virtual void setup() {}

    virtual void cleanup() {}

    virtual void update(float delta_time) {}

    virtual void render() {}

    void run(engine& engine) {
        engine_ = &engine;
    }

protected:
    [[nodiscard]]
    engine& get_engine() const {
#ifndef NDEBUG
        if (!engine_) {
            throw std::runtime_error("engine not set in app");
        }
#endif
        return *engine_;
    }

private:
    engine* engine_ = nullptr;
};
}  // namespace vw::gfx

#endif  // VW_GFX_APP_H
