option(ENABLE_CLANG_TIDY "Enable static analysis with clang-tidy" OFF)

function(vw_setup_static_analyzers target)
    if(TARGET vw_setup_static_analyzers)
        return()
    endif()

    if (ENABLE_CLANG_TIDY)
        find_program(CLANG_TIDY clang-tidy REQUIRED)

        get_target_property(TARGET_SOURCES ${target} SOURCES)
        get_target_property(TARGET_SOURCE_DIR ${target} SOURCE_DIR)
        list(FILTER TARGET_SOURCES INCLUDE REGEX ".*\\.cpp$")

        set(ABSOLUTE_SOURCES "")
        foreach(src ${TARGET_SOURCES})
            cmake_path(ABSOLUTE_PATH src BASE_DIRECTORY ${TARGET_SOURCE_DIR})
            cmake_path(RELATIVE_PATH src BASE_DIRECTORY ${CMAKE_SOURCE_DIR})
            list(APPEND ABSOLUTE_SOURCES ${src})
        endforeach()

        add_custom_target(${target}_clang_tidy
            COMMAND ${CLANG_TIDY}
            -p ${CMAKE_BINARY_DIR}
            --header-filter=.
            ${ABSOLUTE_SOURCES}
            WORKING_DIRECTORY ${CMAKE_SOURCE_DIR}
            USES_TERMINAL
        )
    endif()
endfunction()