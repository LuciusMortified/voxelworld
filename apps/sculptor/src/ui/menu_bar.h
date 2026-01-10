#pragma once

#ifndef VW_SCULPTOR_MENU_BAR_H
#define VW_SCULPTOR_MENU_BAR_H

#include <vw/gfx/engine/engine.h>
#include <vw/gfx/world/world_components.h>

#include "app/app_state.h"
#include "operations/operation_manager.h"

namespace vw::sculptor {

template <typename WC = gfx::base_world_components>
class menu_bar final {
public:
    using engine_type = gfx::engine<WC>;

    menu_bar(engine_type& eng, app_state& state, operation_manager& op_manager);

    void render(float delta_time);

private:
    engine_type* engine_;
    app_state* state_;
    operation_manager* op_manager_;
};

}  // namespace vw::sculptor

#include "menu_bar.inl.h"

#endif  // VW_SCULPTOR_MENU_BAR_H
