// ============================================================================
// [AAA/Core] ENGINE CONTEXT & TELEMETRY
// Description: Defines the dynamic runtime state payload passed across 
//              engine subsystems. Encapsulates 6-DOF observer kinematics 
//              and global chronological scaling.
// Standard: ISO C++20
// ============================================================================
#ifndef ENGINE_CONTEXT_H
#define ENGINE_CONTEXT_H

namespace engine {

    // ------------------------------------------------------------------------
    // [AAA/Kinematics] Observer Telemetry
    // Description: Maintains spatial translation matrices and propulsion 
    //              states for the active viewport observer.
    // ------------------------------------------------------------------------
    struct ObserverTelemetry {
        // Spatial vectors (Cartesian coordinates)
        float pos[3] = { 0.0f, -20.0f, -20.0f };
        float forward[3] = { 0.0f, 0.0f, 1.0f };
        float right[3] = { 1.0f, 0.0f, 0.0f };

        // Euler rotation angles (Degrees)
        float yaw = 90.0f;
        float pitch = 40.0f;

        // Propulsion metrics
        float throttle = 10.0f;
        float current_speed = 0.0f;

        // Peripheral input state mapping
        double last_mouse_x = 960.0;
        double last_mouse_y = 540.0;
        bool   first_mouse = true;
    };

    // ------------------------------------------------------------------------
    // [AAA/Core] Primary Engine Context
    // Description: The master runtime payload containing all dynamic state 
    //              data required by the execution loop. Passed by reference 
    //              to avoid global scope pollution.
    // ------------------------------------------------------------------------
    struct EngineContext {
        ObserverTelemetry flight;

        /**
         * Chronological Dilator
         * Multiplier applied to delta time ($\Delta t$) before physics integration.
         * Formula: $t_{sim} = \Delta t \cdot \text{scale}$
         * Default: 1.0f (Real-time baseline).
         */
        float time_scale = 1.0f;
    };

} // namespace engine

#endif // ENGINE_CONTEXT_H