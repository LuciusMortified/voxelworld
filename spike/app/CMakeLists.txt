add_executable(vw_spike_app main.cpp)

target_link_libraries(vw_spike_app PRIVATE vw_core_spike)
target_compile_features(vw_spike_app PRIVATE cxx_std_23)

add_executable(vw_spike_vulkan vulkan_smoke.cpp)

target_link_libraries(vw_spike_vulkan PRIVATE VulkanHppModule)
target_compile_features(vw_spike_vulkan PRIVATE cxx_std_23)

add_test(NAME spike_app COMMAND vw_spike_app)
add_test(NAME spike_vulkan COMMAND vw_spike_vulkan)
