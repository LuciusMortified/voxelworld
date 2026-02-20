#pragma once

#ifndef VW_SCULPTOR_REMOVE_SOCKET_OPERATION_H
#define VW_SCULPTOR_REMOVE_SOCKET_OPERATION_H

#include <vw/gfx/engine/engine.h>
#include <vw/gfx/world/world_components.h>

#include "app/app_state.h"
#include "base_operation.h"

namespace vw::sculptor {

struct remove_socket_params {
    std::string entity_name;
    std::string socket_name;
};

class remove_socket_operation final : public base_operation {
public:
    using engine_type = gfx::engine<>;

    remove_socket_operation(engine_type& engine, app_state& st, const remove_socket_params& params);

    void execute() override;
    void undo() override;

private:
    engine_type* engine_;
    app_state* state_;
    remove_socket_params params_;
    vec3f saved_position_;
    vec3f saved_rotation_;
    vec3f saved_scale_{1.0F, 1.0F, 1.0F};
};

}  // namespace vw::sculptor

#include "remove_socket_operation.inl.h"

#endif  // VW_SCULPTOR_REMOVE_SOCKET_OPERATION_H
