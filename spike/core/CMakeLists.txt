add_library(vw_core_spike STATIC)

target_sources(vw_core_spike
    PUBLIC
        FILE_SET CXX_MODULES BASE_DIRS src FILES
            src/core.cppm
            src/types.cppm
            src/math.cppm
    PRIVATE
        src/math.cpp
)

target_compile_features(vw_core_spike PUBLIC cxx_std_23)
target_link_libraries(vw_core_spike PUBLIC vw_std)
