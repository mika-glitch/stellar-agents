// ============================================================================
// [Core/Platform] WINDOW MANAGER IMPLEMENTATION
// Description: Platform-specific GLFW and BGFX bootstrapping procedures.
//              Handles hardware context creation and input telemetry callbacks.
// Standard: ISO C++20
// ============================================================================
#define NOMINMAX
#define WIN32_LEAN_AND_MEAN

#include "engine/window_manager.h"
#include "engine/engine_config.h"
#include "engine/engine_context.h"

#include <GLFW/glfw3.h>
#define GLFW_EXPOSE_NATIVE_WIN32
#include <GLFW/glfw3native.h>
#include <bgfx/bgfx.h>

#include <iostream>
#include <algorithm>

namespace engine {

    // Internal isolated pointer to prevent global scope pollution
    static GLFWwindow* s_window = nullptr;

    // ------------------------------------------------------------------------
    // [Core/Platform] Hardware Error Intercept
    // ------------------------------------------------------------------------
    static void glfw_error_callback(int error, const char* description) {
        std::cerr << "[FATAL/GLFW] Code " << error << ": " << description << std::endl;
    }

    // ------------------------------------------------------------------------
    // [Core/Input] Peripheral Telemetry Callback
    // Description: Translates 2D mouse deltas into 3D Euler coordinate shifts.
    // ------------------------------------------------------------------------
    static void mouse_callback(GLFWwindow* window, double xpos, double ypos) {
        // Retrieve the dynamic engine context payload injected during initialization
        engine::EngineContext* context = static_cast<engine::EngineContext*>(glfwGetWindowUserPointer(window));
        if (!context) return;

        engine::ObserverTelemetry& flight = context->flight;

        // Initialize relative tracking anchor on first intercept
        if (flight.first_mouse) {
            flight.last_mouse_x = xpos;
            flight.last_mouse_y = ypos;
            flight.first_mouse = false;
        }

        /**
         * Input Delta Formulation
         * Multiplier 0.1f acts as hardware sensitivity dampening.
         */
        const float delta_x = static_cast<float>(xpos - flight.last_mouse_x) * 0.1f;
        const float delta_y = static_cast<float>(flight.last_mouse_y - ypos) * 0.1f;

        flight.last_mouse_x = xpos;
        flight.last_mouse_y = ypos;

        /**
         * Euler Angle Transformation
         * Yaw: Standard horizontal projection (positive screen delta subtracts yaw).
         * Pitch: Clamped to the bound interval $[-89^\circ, 89^\circ]$ to mathematically
         * prevent gimbal lock inversion in the projection matrix.
         */
        flight.yaw += delta_x;
        flight.pitch = std::clamp(flight.pitch - delta_y, -89.0f, 89.0f);
    }

    // ------------------------------------------------------------------------
    // [Core/Platform] Hardware Initialization
    // ------------------------------------------------------------------------
    bool WindowManager::initialize_display(engine::EngineContext& context) {
        if (!glfwInit()) return false;

        glfwSetErrorCallback(glfw_error_callback);
        glfwWindowHint(GLFW_CLIENT_API, GLFW_NO_API);
        glfwWindowHint(GLFW_RESIZABLE, GLFW_FALSE);

        // Instantiating the primary OS surface wrapper
        s_window = glfwCreateWindow(
            static_cast<int>(config::visual::screen_width),
            static_cast<int>(config::visual::screen_height),
            "STELLAR AGENTS :: ENGINE KERNEL",
            nullptr,
            nullptr
        );

        if (!s_window) return false;

        // Inject the dynamic context payload for callback retrieval
        glfwSetWindowUserPointer(s_window, &context);

        // Bind peripheral listeners
        glfwSetCursorPosCallback(s_window, mouse_callback);
        glfwSetInputMode(s_window, GLFW_CURSOR, GLFW_CURSOR_DISABLED);

        // Formulate BGFX Platform Data for Vulkan binding
        bgfx::PlatformData platform_data{};
#if defined(_WIN32)
        platform_data.nwh = glfwGetWin32Window(s_window);
#endif

        bgfx::Init bgfx_init;
        bgfx_init.type = bgfx::RendererType::Vulkan;
        bgfx_init.platformData = platform_data;
        bgfx_init.resolution.width = static_cast<uint32_t>(config::visual::screen_width);
        bgfx_init.resolution.height = static_cast<uint32_t>(config::visual::screen_height);
        bgfx_init.resolution.reset = BGFX_RESET_VSYNC | BGFX_RESET_MSAA_X4;

        if (!bgfx::init(bgfx_init)) {
            glfwDestroyWindow(s_window);
            return false;
        }

        // Configure View 0: Volumetric background pass (clears framebuffer & depth buffer)
        bgfx::setViewRect(0, 0, 0, bgfx_init.resolution.width, bgfx_init.resolution.height);
        bgfx::setViewClear(0, BGFX_CLEAR_COLOR | BGFX_CLEAR_DEPTH, 0x000000FF, 1.0f, 0);

        // Configure View 1: Particle point cloud pass (no clear, draws over View 0)
        bgfx::setViewRect(1, 0, 0, bgfx_init.resolution.width, bgfx_init.resolution.height);
        bgfx::setViewClear(1, BGFX_CLEAR_NONE);

        return true;
    }

    void WindowManager::poll_events() {
        glfwPollEvents();
    }

    double WindowManager::get_time() {
        return glfwGetTime();
    }

    void* WindowManager::get_window_handle() noexcept {
        return s_window;
    }

    bool WindowManager::is_running() {
        if (!s_window) return false;
        return !glfwWindowShouldClose(s_window);
    }

    void WindowManager::shutdown() {
        if (s_window) {
            glfwDestroyWindow(s_window);
            s_window = nullptr;
        }
        glfwTerminate();
    }

} // namespace engine