project(sculptor VERSION 0.2.0 LANGUAGES CXX)

add_executable(${PROJECT_NAME}
        src/main.cpp
        src/app/app.h
        src/app/app.inl.h
        src/app/app_state.h
        src/ui/tool_panel.h
        src/ui/tool_panel.inl.h
        src/ui/color_palette_panel.h
        src/ui/color_palette_panel.inl.h
        src/ui/entity_properties_panel.h
        src/ui/entity_properties_panel.inl.h
        src/ui/entity_tree_panel.h
        src/ui/entity_tree_panel.inl.h
        src/ui/ui_utils.h
        src/operations/base_operation.h
        src/operations/operation_manager.h
        src/operations/operation_manager.inl.h
        src/operations/create_entity_operation.h
        src/operations/create_entity_operation.inl.h
        src/operations/delete_entity_operation.h
        src/operations/delete_entity_operation.inl.h
        src/ui/create_entity_modal.h
        src/ui/create_entity_modal.inl.h
        src/ui/menu_bar.h
        src/ui/menu_bar.inl.h
        src/ui/delete_entity_modal.h
        src/ui/delete_entity_modal.inl.h
        src/operations/set_transform_operation.h
        src/operations/set_transform_operation.inl.h
        src/tools/base_tool.h
        src/tools/dummy_tool.h
        src/tools/remove_voxel_tool.h
        src/tools/remove_voxel_tool.inl.h
        src/operations/remove_voxel_operation.h
        src/operations/remove_voxel_operation.inl.h
        src/tools/add_voxel_tool.h
        src/tools/add_voxel_tool.inl.h
        src/tools/paint_tool.h
        src/tools/paint_tool.inl.h
        src/operations/add_voxel_operation.h
        src/operations/add_voxel_operation.inl.h
        src/operations/paint_voxel_operation.h
        src/operations/paint_voxel_operation.inl.h
        src/ui/startup_modal.h
        src/ui/startup_modal.inl.h
        src/ui/new_file_modal.h
        src/ui/new_file_modal.inl.h
        src/ui/open_file_modal.h
        src/ui/open_file_modal.inl.h
        src/operations/expand_model_operation.h
        src/operations/expand_model_operation.inl.h
)

target_link_libraries(${PROJECT_NAME} vwengine)
target_compile_features(${PROJECT_NAME} PRIVATE cxx_std_23)

if (MSVC)
    target_compile_options(${PROJECT_NAME} PRIVATE /bigobj)
endif()

configure_file(
        ${CMAKE_CURRENT_SOURCE_DIR}/src/app/sculptor_version.h.in
        ${CMAKE_CURRENT_BINARY_DIR}/include/sculptor_version.h
        @ONLY
)

target_include_directories(${PROJECT_NAME} PRIVATE
        ${CMAKE_CURRENT_BINARY_DIR}/include
)

target_include_directories(${PROJECT_NAME} PRIVATE
        ${CMAKE_CURRENT_SOURCE_DIR}/src
)

include(shaders)
vw_setup_shaders(${PROJECT_NAME})

include(static-analyzers)
vw_setup_static_analyzers(${PROJECT_NAME})
