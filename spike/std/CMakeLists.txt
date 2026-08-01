add_library(vw_std INTERFACE)

if(CMAKE_CXX_MODULE_STD)
    return()
endif()

if(NOT DEFINED ENV{VCToolsInstallDir})
    message(FATAL_ERROR
        "VCToolsInstallDir is not set: run from a Visual Studio developer shell "
        "so the MS STL module sources can be located")
endif()

file(TO_CMAKE_PATH "$ENV{VCToolsInstallDir}" VW_VC_TOOLS_DIR)
set(VW_STD_MODULES_DIR "${VW_VC_TOOLS_DIR}/modules")

if(NOT EXISTS "${VW_STD_MODULES_DIR}/std.ixx")
    message(FATAL_ERROR "MS STL module source not found: ${VW_STD_MODULES_DIR}/std.ixx")
endif()

add_library(vw_std_msvc STATIC)

target_sources(vw_std_msvc
    PUBLIC
        FILE_SET CXX_MODULES BASE_DIRS "${VW_STD_MODULES_DIR}" FILES
            "${VW_STD_MODULES_DIR}/std.ixx"
)

target_compile_features(vw_std_msvc PUBLIC cxx_std_23)

target_compile_options(vw_std_msvc PRIVATE
    -Wno-reserved-module-identifier
    -Wno-include-angled-in-module-purview
)

target_link_libraries(vw_std INTERFACE vw_std_msvc)
