#pragma once

#ifndef VW_SCULPTOR_ADD_SOCKET_OPERATION_H
#define VW_SCULPTOR_ADD_SOCKET_OPERATION_H

#include <vw/gfx/engine/engine.h>
#include <vw/ecs/base_world_def.h>

#include "app/app_state.h"
#include "base_operation.h"

namespace vw::sculptor {

struct add_socket_params {
    std::string entity_name;
    std::string socket_name;
    vec3f position{};
    quat rotation{};
    vec3f scale{1.0F, 1.0F, 1.0F};
};

class add_socket_operation final : public base_operation {
public:
    using engine_type = gfx::engine<>;

    add_socket_operation(engine_type& engine, app_state& st, const add_socket_params& params);

    void execute() override;
    void undo() override;

private:
    engine_type* engine_;
    app_state* state_;
    add_socket_params params_;
};

}  // namespace vw::sculptor

#include "add_socket_operation.inl.h"

#endif  // VW_SCULPTOR_ADD_SOCKET_OPERATION_H
