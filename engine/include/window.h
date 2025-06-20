#pragma once
#include <string_view>
#include <vector>
#include "input.h"

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

        int get_width() const { return width_; }
        int get_height() const { return height_; }
        
        bool is_key_pressed(input::key key) const;
        bool is_mouse_button_pressed(input::mouse_button button) const;
        void get_cursor_pos(double* xpos, double* ypos) const;
        void set_cursor_pos(double xpos, double ypos);
        
        void set_cursor_mode(input::cursor_mode mode);
        void set_input_mode(input::input_mode mode, int value);
        
        void set_title(std::string_view title);
        void set_size(int width, int height);
        void set_position(int xpos, int ypos);
        void maximize();
        void minimize();
        void restore();

        GLFWwindow* get_handle() const { return window_; }

    private:
        GLFWwindow* window_;
        int width_, height_;
        
        mutable double last_cursor_x_ = 0.0;
        mutable double last_cursor_y_ = 0.0;
    };
}