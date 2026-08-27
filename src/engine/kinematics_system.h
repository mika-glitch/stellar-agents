// ============================================================================
// [AAA/Kinematics] FLIGHT INPUT PROCESSING SYSTEM
// Description: Translates raw peripheral input states (Keyboard) into 6-DOF 
//              spatial translation vectors for the active observer.
// Standard: ISO C++20
// ============================================================================
#ifndef KINEMATICS_SYSTEM_H
#define KINEMATICS_SYSTEM_H

#include "engine/engine_context.h"

namespace engine {

    class KinematicsSystem {
    public:
        /**
         * Evaluates current hardware input states and computes spatial translations.
         * Applies trigonometric projection from Euler angles to Cartesian vectors.
         *
         * @param context    Reference to the master engine context containing telemetry.
         * @param delta_time The clamped frame duration ($\Delta t$) for velocity integration.
         */
        static void process_flight_inputs(engine::EngineContext& context, float delta_time);
    };

} // namespace engine

#endif // KINEMATICS_SYSTEM_H