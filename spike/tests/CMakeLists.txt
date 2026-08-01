find_package(Catch2 3 CONFIG REQUIRED)

include(CTest)
include(Catch)

add_executable(vw_spike_tests spike.test.cpp)

target_link_libraries(vw_spike_tests PRIVATE
    vw_core_spike
    vw_std
    Catch2::Catch2WithMain
)

target_compile_features(vw_spike_tests PRIVATE cxx_std_23)

catch_discover_tests(vw_spike_tests)
