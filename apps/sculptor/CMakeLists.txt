project(sculptor VERSION 0.0.1 LANGUAGES CXX)

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
        src/tools/select_entity_tool.h
        src/tools/select_entity_tool.inl.h
        src/tools/remove_voxel_tool.h
        src/tools/remove_voxel_tool.inl.h
        src/operations/remove_voxel_operation.h
        src/operations/remove_voxel_operation.inl.h
        src/tools/add_voxel_tool.h
        src/tools/add_voxel_tool.inl.h
)

target_link_libraries(${PROJECT_NAME} vwengine)
target_compile_features(${PROJECT_NAME} PRIVATE cxx_std_23)

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

find_program(GLSLC_EXECUTABLE glslc REQUIRED)
set(SHADERS_SRC_DIR ${CMAKE_SOURCE_DIR}/shaders)
set(SHADERS_BIN_DIR ${CMAKE_CURRENT_BINARY_DIR}/shaders_compiled)

file(GLOB SHADER_VERTS CONFIGURE_DEPENDS ${SHADERS_SRC_DIR}/*.vert)
file(GLOB SHADER_FRAGS CONFIGURE_DEPENDS ${SHADERS_SRC_DIR}/*.frag)
set(SHADER_SOURCES ${SHADER_VERTS} ${SHADER_FRAGS})

set(SHADER_SPV_OUTPUTS)
foreach(SHADER ${SHADER_SOURCES})
    get_filename_component(SHADER_NAME ${SHADER} NAME) # e.g. mesh.vert
    set(SPV_OUT ${SHADERS_BIN_DIR}/${SHADER_NAME}.spv)  # e.g. mesh.vert.spv

    add_custom_command(
            OUTPUT ${SPV_OUT}
            COMMAND ${CMAKE_COMMAND} -E make_directory ${SHADERS_BIN_DIR}
            COMMAND ${GLSLC_EXECUTABLE} ${SHADER} -o ${SPV_OUT}
            DEPENDS ${SHADER}
            VERBATIM
            COMMENT "Compiling shader ${SHADER_NAME}"
    )

    list(APPEND SHADER_SPV_OUTPUTS ${SPV_OUT})
endforeach()

add_custom_target(compile_shaders ALL DEPENDS ${SHADER_SPV_OUTPUTS})

add_custom_command(TARGET ${PROJECT_NAME} POST_BUILD
        COMMAND ${CMAKE_COMMAND} -E make_directory $<TARGET_FILE_DIR:${PROJECT_NAME}>/shaders
        COMMAND ${CMAKE_COMMAND} -E copy_directory ${SHADERS_SRC_DIR} $<TARGET_FILE_DIR:${PROJECT_NAME}>/shaders
        COMMAND ${CMAKE_COMMAND} -E copy_directory ${SHADERS_BIN_DIR} $<TARGET_FILE_DIR:${PROJECT_NAME}>/shaders
        COMMENT "Copying shaders (and .spv) to ${PROJECT_NAME} build directory"
)

add_dependencies(${PROJECT_NAME} compile_shaders)