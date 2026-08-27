// ============================================================================
// [AAA/Platform] WINDOW MANAGER INTERFACE
// Description: Encapsulates OS-level window creation, input event polling, 
//              and hardware graphics API bindings (Vulkan via BGFX).
// Standard: ISO C++20
// ============================================================================
#ifndef WINDOW_MANAGER_H
#define WINDOW_MANAGER_H

#include "engine/engine_context.h"

namespace engine {

    class WindowManager {
    public:
        /**
         * Bootstraps the application window and binds the Vulkan rendering surface.
         * Injects the dynamic EngineContext into the window payload for callback access.
         *
         * @param context Reference to the master runtime telemetry payload.
         * @return True if hardware initialization succeeds, false otherwise.
         */
        static bool initialize_display(engine::EngineContext& context);

        /**
         * Dispatches pending OS-level hardware interrupts and peripheral events.
         */
        static void poll_events();

        /**
         * Retrieves the high-resolution hardware chronological timer.
         * @return Current execution time in seconds.
         */
        static double get_time();

        /**
         * Evaluates the OS-level termination signal for the execution loop.
         * @return True while the execution context remains active.
         */
        static bool is_running();

        /**
         * Executes hardware teardown and releases OS window handles.
         */
        static void shutdown();
        /**
         * @brief Retrieves the opaque hardware window handle.
         * @return Void pointer to the underlying OS window surface.
         */
        [[nodiscard]] static void* get_window_handle() noexcept;
    };

} // namespace engine

#endif // WINDOW_MANAGER_H#pragma once
