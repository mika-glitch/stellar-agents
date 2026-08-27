// ============================================================================
// [Core] SYSTEM ENTRY NODE
// Description: Master orchestration loop for the Complex Adaptive Dynamic 
//              Systems engine. Handles anonymized data seeding, 
//              chrono time-scaling, and synchronous memory-barrier dispatch.
// Standard: ISO C++20 | Layout: Data-Oriented Design / SoA-ready
// ============================================================================
#define NOMINMAX
#define WIN32_LEAN_AND_MEAN

// --- SECTOR 2: COMPLEX ADAPTIVE DYNAMIC SYSTEMS (CADS) ---
#include "cads/environment_matrix.h"
#include "engine/universe_seeder.h"

// --- SECTOR 1: SIMULATION INFRASTRUCTURE (ENGINE) ---
#include "engine/window_manager.h"
#include "engine/engine_config.h"
#include "engine/engine_context.h"
#include "engine/kinematics_system.h"
#include "engine/physics_evolution.h"
#include "engine/render_core.h"
#include "engine/telemetry_hud.h"

#include <bgfx/bgfx.h>
#include <algorithm>

using namespace stellar_agents;

int main() {
    // ------------------------------------------------------------------------
    // PHASE 1: SUBSYSTEM INITIALIZATION
    // ------------------------------------------------------------------------

    engine::EngineContext context;

    if (!engine::WindowManager::initialize_display(context)) {
        return -1;
    }
	// Configure BGFX debug text overlay for telemetry readouts
    bgfx::setDebug(BGFX_DEBUG_TEXT);

    stellar_agents::EnvironmentMatrix state_matrix(config::simulation::total_agent_capacity);

    // Korrigierter Namespace-Aufruf für den Seeder
    stellar_agents::UniverseSeeder::seed_initial_conditions(state_matrix);

    stellar_agents::PhysicsEvolution  evolution_engine;
    stellar_agents::RenderCore        graphics_engine;
    engine::TelemetryHUD              diagnostics_hud;

    double previous_time = engine::WindowManager::get_time();
    context.time_scale = 1.0f;

    // ------------------------------------------------------------------------
    // PHASE 2: MAIN EXECUTION LOOP
    // ------------------------------------------------------------------------

    while (engine::WindowManager::is_running()) {

        engine::WindowManager::poll_events();

        const double current_time = engine::WindowManager::get_time();

        /**
         * Delta Time Clamping
         * Formula: $\Delta t = \min(t_{current} - t_{previous}, \Delta t_{max})$
         * Description: Clamps delta time to a maximum of 33.3ms (30 FPS equivalent)
         * to prevent numerical explosion in the physics integrator during OS-level lag spikes.
         */
        const float delta_time = std::min(static_cast<float>(current_time - previous_time), 0.0333f);
        previous_time = current_time;

        engine::KinematicsSystem::process_flight_inputs(context, delta_time);

        evolution_engine.execute_asynchronous_tick(delta_time * context.time_scale, state_matrix);

        bgfx::touch(0);

        /**
         * 6-DOF Camera Matrix Formulation
         * Defines the observer's spatial translation and directional vector:
         * $[\vec{P}_x, \vec{P}_y, \vec{P}_z, \vec{P}_x + \vec{V}_{fwd,x}, \vec{P}_y + \vec{V}_{fwd,y}, \vec{P}_z + \vec{V}_{fwd,z}]$
         */
        const float camera_data[6] = {
            context.flight.pos[0], context.flight.pos[1], context.flight.pos[2],
            context.flight.pos[0] + context.flight.forward[0],
            context.flight.pos[1] + context.flight.forward[1],
            context.flight.pos[2] + context.flight.forward[2]
        };

        graphics_engine.draw_composite_scene(camera_data, state_matrix);
        diagnostics_hud.render_overlay(context);
        bgfx::frame();
    }

    // ------------------------------------------------------------------------
    // PHASE 3: TEARDOWN & GARBAGE COLLECTION
    // ------------------------------------------------------------------------

    bgfx::shutdown();
    engine::WindowManager::shutdown();

    return 0;
}