#pragma once

#ifndef VW_SCULPTOR_SOCKET_PANEL_H
#define VW_SCULPTOR_SOCKET_PANEL_H

#include "app/app_state.h"
#include "operations/add_socket_operation.h"
#include "operations/remove_socket_operation.h"
#include "operations/set_socket_transform_operation.h"
#include "ui_utils.h"

namespace vw::sculptor {

class socket_panel final {
public:
    using engine_type = gfx::engine<>;

    socket_panel(engine_type& eng, app_state& st, operation_manager& op_manager);

    void render(float delta_time);

private:
    void render_socket_(gfx::entity ent, const gfx::socket_point& sp);
    void render_add_socket_(gfx::entity ent);
    void render_preview_file_list_();

    void load_preview_(const std::string& key, const std::string& filename);
    void unload_preview_(const std::string& key);
    void update_preview_transform_(const std::string& key, const vec3f& position,
                                   const vec3f& rotation, const vec3f& scale);

    engine_type* engine_;
    app_state* state_;
    operation_manager* op_manager_;

    std::string new_socket_name_;
    bool need_preview_modal_ = false;
    std::string preview_modal_key_;
    std::vector<std::string> vox_filenames_;
};

}  // namespace vw::sculptor

#include "socket_panel.inl.h"

#endif  // VW_SCULPTOR_SOCKET_PANEL_H
