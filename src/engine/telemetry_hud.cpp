// ============================================================================
// [Core/UI] TELEMETRY HUD IMPLEMENTATION
// Description: Formats and streams spatial coordinates, flight metrics, 
//              hardware debug toggles, and compact control references.
// Standard: ISO C++20
// ============================================================================
#define NOMINMAX
#define WIN32_LEAN_AND_MEAN

#include "engine/telemetry_hud.h"
#include "engine/window_manager.h"
#include <GLFW/glfw3.h>
#include <bgfx/bgfx.h>

namespace engine {

    void TelemetryHUD::render_overlay(const engine::EngineContext& context) {
        // --------------------------------------------------------------------
        // HARDWARE DEBUG PROFILER TOGGLE (F1 / F2)
        // --------------------------------------------------------------------
        static uint32_t debug_flags = BGFX_DEBUG_TEXT;
        static bool key_f1_latch = false;
        static bool key_f2_latch = false;

        if (auto* current_window = static_cast<GLFWwindow*>(engine::WindowManager::get_window_handle())) {
            const bool f1_down = (glfwGetKey(current_window, GLFW_KEY_F1) == GLFW_PRESS);
            if (f1_down && !key_f1_latch) {
                debug_flags ^= BGFX_DEBUG_STATS;
                bgfx::setDebug(debug_flags);
            }
            key_f1_latch = f1_down;

            const bool f2_down = (glfwGetKey(current_window, GLFW_KEY_F2) == GLFW_PRESS);
            if (f2_down && !key_f2_latch) {
                debug_flags ^= BGFX_DEBUG_WIREFRAME;
                bgfx::setDebug(debug_flags);
            }
            key_f2_latch = f2_down;
        }

        // --------------------------------------------------------------------
        // COMPACT TELEMETRY & CONTROLS OVERLAY
        // --------------------------------------------------------------------
        const engine::ObserverTelemetry& flight = context.flight;

        bgfx::dbgTextClear();

        // Top left: Single-line controls & debug guide
        bgfx::dbgTextPrintf(1, 1, 0x0e, "[F1] TELEMETRY | [F2] WIREFRAME | WASD + SPACE/LCTRL + MOUSE: MOVE | SHIFT: BOOST | SPACE: UP");

        // Kinematic metrics
        bgfx::dbgTextPrintf(1, 3, 0x0f, "POS X: %8.2f MM", flight.pos[0]);
        bgfx::dbgTextPrintf(1, 4, 0x0f, "POS Y: %8.2f MM", flight.pos[1]);
        bgfx::dbgTextPrintf(1, 5, 0x0f, "POS Z: %8.2f MM", flight.pos[2]);
        bgfx::dbgTextPrintf(1, 6, 0x1e, "SPD:   %8.2f MM/s", flight.current_speed);
        bgfx::dbgTextPrintf(1, 7, 0x0a, "THR:     %3.0f%%", flight.throttle * 100.0f);
    }

} // namespace engine