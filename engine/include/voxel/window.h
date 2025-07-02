#pragma once
#include <string>
#include <string_view>
#include <vector>

#define GLFW_INCLUDE_VULKAN
#include <GLFW/glfw3.h>

#include "input.h"
#include "events.h"

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
        
        // Публичный доступ к диспетчеру событий
        events::event_dispatcher& get_event_dispatcher() { return event_dispatcher_; }
        
        // Удобный метод для подписки на события
        template<events::event_type E, events::event_callback<E> F>
        events::sub_id on(F&& callback) {
            return event_dispatcher_.on<E>(std::forward<F>(callback));
        }

    private:
        // GLFW callback функции
        static void key_callback(GLFWwindow* window, int key, int scancode, int action, int mods);
        static void mouse_button_callback(GLFWwindow* window, int button, int action, int mods);
        static void mouse_motion_callback(GLFWwindow* window, double xpos, double ypos);
        static void mouse_scroll_callback(GLFWwindow* window, double xoffset, double yoffset);
        static void window_size_callback(GLFWwindow* window, int width, int height);
        static void window_focus_callback(GLFWwindow* window, int focused);
        static void window_close_callback(GLFWwindow* window);

        GLFWwindow* window_;
        int width_, height_;
        std::string title_;
        
        mutable double last_cursor_x_ = 0.0;
        mutable double last_cursor_y_ = 0.0;
        
        // Система событий
        events::event_dispatcher event_dispatcher_;
    };
}