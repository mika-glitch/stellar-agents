// ============================================================================
// [Core/Simulation] ATTRACTOR AGENT STATE MUTATOR (STRUCTURE OF ARRAYS)
// Description: Mutates gravitational attractor states using Keplerian orbital mechanics.
//              Adapted from original attractor logic[cite: 15].
// Standard: ISO C++20
// ============================================================================
#include "engine/engine_config.h"
#include "cads/environment_matrix.h"
#include <cmath>

namespace stellar_agents {

    // ============================================================================
    // ATTRACTOR AGENT STATE MUTATION OPERATOR (SoA INDEX-BASED)
    // ============================================================================
    void MutateAttractorAgentSoA(
        EnvironmentBuffersSoA& buffer,
        uint64_t i,
        float execution_time) noexcept
    {
        uint32_t agent_id = buffer.agent_id[i];

        // Primary singularity remains strictly static at the coordinate origin[cite: 15]
        if (agent_id == 0) [[unlikely]] {
            return;
        }

        float x = 0.0f;
        float y = 0.0f;
        float z = 0.0f;
        float time_scale = 1.0f;

        // Map identifier nodes to configured ephemeris parameters[cite: 15]
        if (agent_id == 1) {
            x = config::astrodynamics::inner_body_pos[0];
            y = config::astrodynamics::inner_body_pos[1];
            z = config::astrodynamics::inner_body_pos[2];
            time_scale = 1.0f;
        }
        else if (agent_id == 2) {
            x = config::astrodynamics::outer_body_pos[0];
            y = config::astrodynamics::outer_body_pos[1];
            z = config::astrodynamics::outer_body_pos[2];
            time_scale = 0.7f;
        }
        else if (agent_id == 3) {
            x = config::astrodynamics::gateway_node_pos[0];
            y = config::astrodynamics::gateway_node_pos[1];
            z = config::astrodynamics::gateway_node_pos[2];
            time_scale = 0.5f;
        }
        else {
            return;
        }

        const float orbit_radius = std::sqrt(x * x + y * y + z * z);
        if (orbit_radius < 0.0001f) [[unlikely]] {
            return;
        }

        /**
         * Keplerian Angular Speed Formulation
         * Formula: $\omega = \sqrt{\frac{G \cdot M}{r^3}}$
         */
        const float initial_angle = std::atan2(z, x);
        const float angular_speed = std::sqrt(config::physics::primary_attractor_runtime_mass / (orbit_radius * orbit_radius * orbit_radius));
        const float current_angle = initial_angle + (execution_time * angular_speed * time_scale);

        // Update parallel SoA position and velocity channels directly
        buffer.pos_x[i] = orbit_radius * std::cos(current_angle);
        buffer.pos_y[i] = y;
        buffer.pos_z[i] = orbit_radius * std::sin(current_angle);

        buffer.vel_x[i] = -orbit_radius * angular_speed * time_scale * std::sin(current_angle);
        buffer.vel_y[i] = 0.0f;
        buffer.vel_z[i] = orbit_radius * angular_speed * time_scale * std::cos(current_angle);
    }

} // namespace stellar_agents