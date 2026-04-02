find_program(GLSLC_EXECUTABLE glslc REQUIRED)

set(VW_SHADERS_SRC_DIR ${CMAKE_SOURCE_DIR}/shaders)
set(VW_SHADERS_BIN_DIR ${CMAKE_BINARY_DIR}/shaders_compiled)

function(vw_compile_shaders)
    if(TARGET vw_compile_shaders)
        return()
    endif()

    file(GLOB SHADER_VERTS CONFIGURE_DEPENDS ${VW_SHADERS_SRC_DIR}/*.vert)
    file(GLOB SHADER_FRAGS CONFIGURE_DEPENDS ${VW_SHADERS_SRC_DIR}/*.frag)
    file(GLOB SHADER_COMPS CONFIGURE_DEPENDS ${VW_SHADERS_SRC_DIR}/*.comp)
    set(SHADER_SOURCES ${SHADER_VERTS} ${SHADER_FRAGS} ${SHADER_COMPS})

    set(SHADER_SPV_OUTPUTS)
    foreach(SHADER ${SHADER_SOURCES})
        get_filename_component(SHADER_NAME ${SHADER} NAME)
        set(SPV_OUT ${VW_SHADERS_BIN_DIR}/${SHADER_NAME}.spv)

        add_custom_command(
                OUTPUT ${SPV_OUT}
                COMMAND ${CMAKE_COMMAND} -E make_directory ${VW_SHADERS_BIN_DIR}
                COMMAND ${GLSLC_EXECUTABLE} ${SHADER} -o ${SPV_OUT}
                DEPENDS ${SHADER}
                VERBATIM
                COMMENT "Compiling shader ${SHADER_NAME}"
        )

        list(APPEND SHADER_SPV_OUTPUTS ${SPV_OUT})
    endforeach()

    add_custom_target(vw_compile_shaders ALL DEPENDS ${SHADER_SPV_OUTPUTS})
endfunction()

function(vw_setup_shaders TARGET)
    vw_compile_shaders()

    add_custom_command(TARGET ${TARGET} POST_BUILD
            COMMAND ${CMAKE_COMMAND} -E make_directory $<TARGET_FILE_DIR:${TARGET}>/shaders
            VERBATIM
    )

    file(GLOB SHADER_SPVS CONFIGURE_DEPENDS ${VW_SHADERS_BIN_DIR}/*.spv)
    set(ALL_SHADERS ${SHADER_SPVS})

    foreach(SHADER ${ALL_SHADERS})
        get_filename_component(SHADER_NAME ${SHADER} NAME)
        add_custom_command(TARGET ${TARGET} POST_BUILD
                COMMAND ${CMAKE_COMMAND} -E echo "Copying shader ${SHADER_NAME}"
                COMMAND ${CMAKE_COMMAND} -E copy_if_different ${SHADER} $<TARGET_FILE_DIR:${TARGET}>/shaders/${SHADER_NAME}
                VERBATIM
        )
    endforeach()

    add_dependencies(${TARGET} vw_compile_shaders)
endfunction()
