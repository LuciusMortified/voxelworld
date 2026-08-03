add_library(vw_core STATIC)

target_sources(vw_core
    PUBLIC
        FILE_SET CXX_MODULES BASE_DIRS src FILES
            src/core.cppm
            src/types.cppm
            src/timing.cppm
)

target_compile_features(vw_core PUBLIC cxx_std_23)

# `import std` waits for M6: while consumers still include standard headers
# textually, importing the std module into the same TU collides with them.
# Until then module units take the standard library via global module fragment.
