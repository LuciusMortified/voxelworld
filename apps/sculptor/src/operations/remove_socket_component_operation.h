#pragma once

#ifndef VW_SCULPTOR_REMOVE_SOCKET_COMPONENT_OPERATION_H
#define VW_SCULPTOR_REMOVE_SOCKET_COMPONENT_OPERATION_H

#include <string>
#include <vector>

#include <vw/gfx/engine/engine.h>
#include <vw/gfx/world/world_components.h>

#include "app/app_state.h"
#include "base_operation.h"

namespace vw::sculptor {

struct remove_socket_component_params {
    std::string name;
};

class remove_socket_component_operation final : public base_operation {
public:
    using engine_type = gfx::engine<>;

    remove_socket_component_operation(
        engine_type& engine, app_state& state, const remove_socket_component_params& params
    );

    void execute() override;
    void undo() override;

private:
    struct saved_socket {
        std::string name;
        vec3f position;
        quat rotation;
        vec3f scale{1.0F, 1.0F, 1.0F};
    };

    engine_type* engine_;
    app_state* state_;
    remove_socket_component_params params_;

    std::vector<saved_socket> saved_sockets_;
};

}  // namespace vw::sculptor

#include "remove_socket_component_operation.inl.h"

#endif  // VW_SCULPTOR_REMOVE_SOCKET_COMPONENT_OPERATION_H
