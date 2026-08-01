import std;
import vulkan;

auto main() -> int {
    const vk::ApplicationInfo app_info{
        .pApplicationName = "vw_spike",
        .applicationVersion = 1,
        .pEngineName = "voxelworld",
        .engineVersion = 1,
        .apiVersion = vk::ApiVersion13,
    };

    std::println("vk::ApplicationInfo: {} api={}", app_info.pApplicationName,
                 app_info.apiVersion);

    vk::detail::defaultDispatchLoaderDynamic.init();

    const auto version = vk::enumerateInstanceVersion();
    if (version.result != vk::Result::eSuccess) {
        std::println("enumerateInstanceVersion failed: {}", vk::to_string(version.result));
        return 1;
    }

    const auto raw = version.value;
    std::println("loader instance version: {}.{}.{}", (raw >> 22U) & 0x7FU,
                 (raw >> 12U) & 0x3FFU, raw & 0xFFFU);
    return 0;
}
