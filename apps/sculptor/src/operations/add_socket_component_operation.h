#pragma once

#ifndef VW_SCULPTOR_ADD_SOCKET_COMPONENT_OPERATION_H
#define VW_SCULPTOR_ADD_SOCKET_COMPONENT_OPERATION_H

#include <vw/gfx/engine/engine.h>
#include <vw/ecs/world_components.h>

#include "app/app_state.h"
#include "base_operation.h"

namespace vw::sculptor {

struct add_socket_component_params {
    std::string name;
};

class add_socket_component_operation final : public base_operation {
public:
    using engine_type = gfx::engine<>;

    add_socket_component_operation(
        engine_type& engine, app_state& state, const add_socket_component_params& params
    );

    void execute() override;
    void undo() override;

private:
    engine_type* engine_;
    app_state* state_;
    add_socket_component_params params_;
};

}  // namespace vw::sculptor

#include "add_socket_component_operation.inl.h"

#endif  // VW_SCULPTOR_ADD_SOCKET_COMPONENT_OPERATION_H
