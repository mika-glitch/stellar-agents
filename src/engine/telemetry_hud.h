// ============================================================================
// [AAA/UI] TELEMETRY HUD INTERFACE
// Description: Manages the real-time presentation of flight dynamics and 
//              system diagnostics via the BGFX text rendering pipeline.
// Standard: ISO C++20
// ============================================================================
#ifndef TELEMETRY_HUD_H
#define TELEMETRY_HUD_H

#include "engine/engine_context.h"

namespace engine {

    class TelemetryHUD {
    public:
        /**
         * Submits diagnostic strings and spatial tracking data to the
         * BGFX debug text buffer for immediate screen composition.
         *
         * @param context    Reference to the master runtime telemetry state.
         * @param time_scale Current chronological dilation factor.
         */
        static void render_overlay(const engine::EngineContext& context);
    };

} // namespace engine

#endif // TELEMETRY_HUD_H