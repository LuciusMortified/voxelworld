#pragma once

#ifndef VW_SCULPTOR_SET_SOCKET_TRANSFORM_OPERATION_H
#define VW_SCULPTOR_SET_SOCKET_TRANSFORM_OPERATION_H

#include <vw/gfx/engine/engine.h>
#include <vw/gfx/world/world_components.h>

#include "app/app_state.h"
#include "base_operation.h"

namespace vw::sculptor {

struct set_socket_transform_params {
    std::string entity_name;
    std::string socket_name;
    vec3f position;
    vec3f rotation;
    vec3f scale{1.0F, 1.0F, 1.0F};
};

class set_socket_transform_operation final : public base_operation {
public:
    using engine_type = gfx::engine<>;

    set_socket_transform_operation(engine_type& engine, app_state& st,
                                   const set_socket_transform_params& params);

    void execute() override;
    void undo() override;

private:
    void update_attached_(const vec3f& position, const vec3f& rotation, const vec3f& scale);
    void update_preview_(const vec3f& position, const vec3f& rotation, const vec3f& scale);

    engine_type* engine_;
    app_state* state_;
    set_socket_transform_params params_;
    vec3f previous_position_;
    vec3f previous_rotation_;
    vec3f previous_scale_{1.0F, 1.0F, 1.0F};
};

}  // namespace vw::sculptor

#include "set_socket_transform_operation.inl.h"

#endif  // VW_SCULPTOR_SET_SOCKET_TRANSFORM_OPERATION_H
