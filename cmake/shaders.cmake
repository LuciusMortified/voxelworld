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

# Stages every compiled shader next to the executable, before the executable is
# considered built.
#
# Two traps live here and both have cost a day. The staging used to be a
# POST_BUILD command, which runs only when the target relinks -- so a change to
# a shader alone recompiled the .spv and never delivered it, and the application
# went on running the previous one. And the list of shaders used to be a glob of
# the build directory taken at configure time, so a newly added shader was
# missing until somebody reconfigured after building it once.
#
# A custom target the executable depends on has neither problem: it runs
# whenever the executable is built, by any target name, and it copies whatever
# the compile step produced rather than whatever configure happened to see.
function(vw_setup_shaders TARGET)
    vw_compile_shaders()

    add_custom_target(${TARGET}_shaders
            COMMAND ${CMAKE_COMMAND} -E make_directory $<TARGET_FILE_DIR:${TARGET}>/shaders
            COMMAND ${CMAKE_COMMAND} -E copy_directory_if_different
                    ${VW_SHADERS_BIN_DIR} $<TARGET_FILE_DIR:${TARGET}>/shaders
            VERBATIM
            COMMENT "Staging shaders for ${TARGET}"
    )

    add_dependencies(${TARGET}_shaders vw_compile_shaders)
    add_dependencies(${TARGET} ${TARGET}_shaders)
endfunction()
