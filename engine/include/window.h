#pragma once
#include <string_view>

#define GLFW_INCLUDE_VULKAN
#include <GLFW/glfw3.h>

namespace voxel {
    class window {
    public:
        window(int width, int height, std::string_view title);
        ~window();

        // Запретить копирование
        window(const window&) = delete;
        window& operator=(const window&) = delete;

        bool should_close() const;
        void poll_events();
        void get_framebuffer_size(int* width, int* height) const;
        
        VkSurfaceKHR create_surface(VkInstance instance);
        std::vector<const char*> get_required_extensions() const;

        GLFWwindow* get_handle() const { return window_; }

    private:
        GLFWwindow* window_;
        int width_, height_;
    };
}