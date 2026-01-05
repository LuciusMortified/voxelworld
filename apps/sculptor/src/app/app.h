#pragma once

#ifndef VW_SCULPTOR_APP_H
#define VW_SCULPTOR_APP_H

#include <vw/gfx.h>

namespace vw::sculptor {

class app final : public gfx::app<> {
public:
    explicit app(gfx::engine<>& eng);

    void render(float delta_time) override;
};

}  // namespace vw::sculptor

#include "app.inl.h"

#endif  // VW_SCULPTOR_APP_H
