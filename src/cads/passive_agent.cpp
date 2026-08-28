// ============================================================================
// [Core/Simulation] PASSIVE KINEMATIC AGENT MUTATOR (STRUCTURE OF ARRAYS)
// Description: Evaluates boundary conditions for non-reactive kinematic entities.
//              Delegates primary integration (Euler-Cromer) and gravitational
//              acceleration to the core physics execution thread to maintain
//              instruction cache linearity.
// Standard: ISO C++20
// ============================================================================
#include "engine/engine_config.h"
#include "cads/environment_matrix.h"
#include <cmath>
#include <cstdint>

namespace stellar_agents {

    /**
     * Fast pseudo-random float generator based on LCG hashing algorithms.
     */
    [[nodiscard]] static inline float FastHashToFloatSoA(uint32_t& seed) noexcept {
        seed = seed * 1664525u + 1013904223u;
        return static_cast<float>(seed & 0x00FFFFFFu) / static_cast<float>(0x01000000u);
    }

    /**
     * Reinitializes an entity's kinematic state to form a stable Keplerian orbit
     * within the primary accretion plane.
     */
    static inline void RespawnInPlasmaDiskSoA(EnvironmentBuffersSoA& buffer, uint64_t i) noexcept {
        uint32_t seed = buffer.agent_id[i] ^ 0x9E3779B9u;

        float u1 = FastHashToFloatSoA(seed);
        float u2 = FastHashToFloatSoA(seed);
        float u3 = FastHashToFloatSoA(seed);

        float angle = u1 * 6.28318530718f;
        float r_min = config::simulation::asteroid_spawn_radius_min;
        float r_max = config::simulation::asteroid_spawn_radius_max;
        float radius = r_min + u2 * (r_max - r_min);
        float height = (u3 * 2.0f - 1.0f) * 0.25f;

        buffer.pos_x[i] = radius * std::cos(angle);
        buffer.pos_y[i] = height;
        buffer.pos_z[i] = radius * std::sin(angle);

        /**
         * Tangential Keplerian Orbital Velocity Formulation
         * Formula: $v = \sqrt{\frac{G \cdot M}{r}}$
         */
        float v_orbit = std::sqrt(config::physics::primary_attractor_runtime_mass / radius) *
            config::simulation::orbital_velocity_multiplier;

        buffer.vel_x[i] = -v_orbit * std::sin(angle);
        buffer.vel_y[i] = 0.0f;
        buffer.vel_z[i] = v_orbit * std::cos(angle);
        buffer.is_active[i] = 1;
    }

    void MutatePassiveAgentSoA(
        EnvironmentBuffersSoA& buffer,
        uint64_t i,
        float field_acc_x, float field_acc_y, float field_acc_z,
        float delta_time) noexcept
    {
        // Execution guard for inactive entities to prevent processing deprecated state data
        if (buffer.is_active[i] == 0) [[unlikely]] {
            return;
        }

        const float px = buffer.pos_x[i];
        const float py = buffer.pos_y[i];
        const float pz = buffer.pos_z[i];

        const float dist_sq = px * px + py * py + pz * pz;
        constexpr float max_system_radius_sq = 120.0f * 120.0f;

        /**
         * Outer Boundary Culling Evaluation
         * Triggers entity recycling if spatial coordinates exceed the maximum simulation domain.
         */
        if (dist_sq > max_system_radius_sq) [[unlikely]] {
            RespawnInPlasmaDiskSoA(buffer, i);
        }

        // NOTE: Newtonian gravitation and explicit kinematic integration are intentionally 
        // omitted here. They are executed centrally by the concurrent worker threads 
        // to maintain uniform SoA pipeline efficiency.
    }

} // namespace stellar_agents