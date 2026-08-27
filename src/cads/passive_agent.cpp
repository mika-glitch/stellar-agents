// ============================================================================
// [Core/Simulation] PASSIVE AGENT STATE MUTATOR (STRUCTURE OF ARRAYS)
// Description: Evaluates geodesic trajectories within gravitational fields 
//              and handles horizon culling or dynamic disc respawning.
//              Adapted from original passive logic.
// Standard: ISO C++20
// ============================================================================
#include "engine/engine_config.h"
#include "cads/environment_matrix.h"
#include <cmath>

namespace stellar_agents {

    [[nodiscard]] static inline float FastHashToFloatSoA(uint32_t& seed) noexcept {
        seed = seed * 1664525u + 1013904223u;
        return static_cast<float>(seed & 0x00FFFFFFu) / static_cast<float>(0x01000000u);
    }

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
        if (buffer.is_active[i] == 0) [[unlikely]] {
            return;
        }

        float px = buffer.pos_x[i];
        float py = buffer.pos_y[i];
        float pz = buffer.pos_z[i];

        float vx = buffer.vel_x[i];
        float vy = buffer.vel_y[i];
        float vz = buffer.vel_z[i];

        float dist_sq = px * px + py * py + pz * pz;

        constexpr float max_system_radius_sq = 120.0f * 120.0f;
        constexpr float event_horizon_sq = config::physics::rs_horizon * config::physics::rs_horizon;

        /**
         * Event Horizon and Boundary Culling Check
         * Formula: $r^2 \le r_s^2$ (Schwarzschild capture threshold)
         */
        if (dist_sq <= event_horizon_sq || dist_sq > max_system_radius_sq) [[unlikely]] {
            RespawnInPlasmaDiskSoA(buffer, i);
            return;
        }

        /**
         * Newtonian Gravitational Acceleration Field
         * Formula: $\vec{a} = -\frac{G \cdot M}{|\vec{r}|^3} \vec{r}$
         */
        float dist = std::sqrt(dist_sq + 0.0001f);
        float gravity_magnitude = config::physics::primary_attractor_runtime_mass / (dist_sq + 0.0001f);

        float norm_x = -px / dist;
        float norm_y = -py / dist;
        float norm_z = -pz / dist;

        float total_acc_x = field_acc_x + (norm_x * gravity_magnitude);
        float total_acc_y = field_acc_y + (norm_y * gravity_magnitude);
        float total_acc_z = field_acc_z + (norm_z * gravity_magnitude);

        vx += total_acc_x * delta_time;
        vy += total_acc_y * delta_time;
        vz += total_acc_z * delta_time;

        px += vx * delta_time;
        py += vy * delta_time;
        pz += vz * delta_time;

        buffer.pos_x[i] = px;
        buffer.pos_y[i] = py;
        buffer.pos_z[i] = pz;

        buffer.vel_x[i] = vx;
        buffer.vel_y[i] = vy;
        buffer.vel_z[i] = vz;
    }

} // namespace stellar_agents