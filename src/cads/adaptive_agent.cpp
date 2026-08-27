// ============================================================================
// [Core/Simulation] ADAPTIVE AGENT STATE MUTATOR (STRUCTURE OF ARRAYS)
// Description: Computes homeostatic decision-making and thruster adjustments 
//              for autonomous navigation entities within gravitational boundaries.
//              Implements a discrete 3-phase finite state machine for routing.
// Standard: ISO C++20
// ============================================================================
#include "engine/engine_config.h"
#include "cads/environment_matrix.h"
#include <cmath>

namespace stellar_agents {

    enum class ShipClassSoA : uint8_t {
        LIGHT_INTERCEPTOR = 0,
        HEAVY_CORVETTE = 1,
        CAPITAL_CRUISER = 2
    };

    // Evaluates propulsion magnitude limits mapped to structural mass classes.
    [[nodiscard]] static constexpr float GetThrustPowerSoA(uint32_t agent_id) noexcept {
        ShipClassSoA type = static_cast<ShipClassSoA>(agent_id % 3);
        if (type == ShipClassSoA::HEAVY_CORVETTE)  return 7.5f;
        if (type == ShipClassSoA::CAPITAL_CRUISER) return 24.0f;
        return 4.2f;
    }

    // Generates deterministic pseudo-random float [0.0, 1.0] for stochastic routing.
    [[nodiscard]] static inline float FastHashToFloatSoA(uint32_t seed) noexcept {
        seed = seed * 1664525u + 1013904223u;
        return static_cast<float>(seed & 0x00FFFFFFu) / static_cast<float>(0x01000000u);
    }

    void MutateAdaptiveAgentSoA(
        EnvironmentBuffersSoA& buffer,
        uint64_t i,
        float field_acc_x, float field_acc_y, float field_acc_z,
        float target_planet_x, float target_planet_y, float target_planet_z,
        float execution_time,
        float delta_time) noexcept
    {
        if (buffer.is_active[i] == 0) [[unlikely]] {
            return;
        }

        // 1. Extract kinematic state from memory arena
        float px = buffer.pos_x[i];
        float py = buffer.pos_y[i];
        float pz = buffer.pos_z[i];

        float vx = buffer.vel_x[i];
        float vy = buffer.vel_y[i];
        float vz = buffer.vel_z[i];

        // 2. Extract autonomous navigation state
        uint8_t state = buffer.agent_state[i];
        float timer = buffer.behavior_timer[i];
        float tx = buffer.target_pos_x[i];
        float ty = buffer.target_pos_y[i];
        float tz = buffer.target_pos_z[i];

        // Initialization safety catch (if spawned without an objective)
        if (tx == 0.0f && ty == 0.0f && tz == 0.0f) [[unlikely]] {
            tx = config::astrodynamics::inner_body_pos[0];
            ty = config::astrodynamics::inner_body_pos[1];
            tz = config::astrodynamics::inner_body_pos[2];
        }

        // Compute spatial delta and distance to objective
        float dx = tx - px;
        float dy = ty - py;
        float dz = tz - pz;
        float dist_target = std::sqrt(dx * dx + dy * dy + dz * dz);

        constexpr float convergence_radius = 8.0f;
        constexpr float loiter_duration = 15.0f;

        // ====================================================================
        // FINITE STATE MACHINE (FSM) EVALUATION
        // ====================================================================

        if (state == 0) {
            // Phase 0: Transit to Objective
            if (dist_target < convergence_radius) {
                state = 1;
                timer = loiter_duration;
            }
        }
        else if (state == 1) {
            // Phase 1: Localized Station-Keeping / Loitering
            timer -= delta_time;
            if (timer <= 0.0f) {
                state = 2;
                // Update objective to Spatial Portal Manifold
                tx = config::astrodynamics::gateway_node_pos[0];
                ty = config::astrodynamics::gateway_node_pos[1];
                tz = config::astrodynamics::gateway_node_pos[2];
            }
        }
        else if (state == 2) {
            // Phase 2: Return Transit to Gateway
            if (dist_target < convergence_radius) {
                state = 0;

                // Stochastic routing decision for next operational cycle
                uint32_t r_seed = buffer.agent_id[i] ^ static_cast<uint32_t>(execution_time * 1000.0f);
                float rand_val = FastHashToFloatSoA(r_seed);

                if (rand_val < 0.33f) {
                    // Target Inner Compact Mass Node (Planet 1)
                    tx = buffer.pos_x[1]; ty = buffer.pos_y[1]; tz = buffer.pos_z[1];
                }
                else if (rand_val < 0.66f) {
                    // Target Outer Volumetric Mass Node (Planet 2)
                    tx = buffer.pos_x[2]; ty = buffer.pos_y[2]; tz = buffer.pos_z[2];
                }
                else {
                    // Target Synthetic Geodesic Coordinate (Asteroid Field)
                    // Avoids O(N) memory scans by mathematically generating a valid intercept point.
                    float angle = FastHashToFloatSoA(r_seed + 1) * 6.28318530718f;
                    float r_min = config::simulation::asteroid_spawn_radius_min;
                    float r_max = config::simulation::asteroid_spawn_radius_max;
                    float radius = r_min + FastHashToFloatSoA(r_seed + 2) * (r_max - r_min);

                    tx = radius * std::cos(angle);
                    ty = (FastHashToFloatSoA(r_seed + 3) * 2.0f - 1.0f) * 0.5f;
                    tz = radius * std::sin(angle);
                }
            }
        }

        // ====================================================================
        // PROPULSION & KINEMATIC INTEGRATION
        // ====================================================================

        uint32_t agent_id = buffer.agent_id[i];
        float base_thrust = GetThrustPowerSoA(agent_id);
        float thrust_x = 0.0f, thrust_y = 0.0f, thrust_z = 0.0f;

        float dir_x = (dist_target > 0.001f) ? (dx / dist_target) : 0.0f;
        float dir_y = (dist_target > 0.001f) ? (dy / dist_target) : 0.0f;
        float dir_z = (dist_target > 0.001f) ? (dz / dist_target) : 0.0f;

        if (state == 0 || state == 2) {
            /**
             * Proportional Navigation Guidance Law with Velocity Damping
             * Formula: $\vec{a} = k_{nav} \hat{u}_{target} - k_{damp} \vec{v}$
             */
            constexpr float k_nav = 1.2f;
            constexpr float k_damp = 0.6f;

            thrust_x = (dir_x * base_thrust * k_nav) - (vx * k_damp);
            thrust_y = (dir_y * base_thrust * k_nav) - (vy * k_damp);
            thrust_z = (dir_z * base_thrust * k_nav) - (vz * k_damp);
        }
        else {
            // Loitering: Executing orbital weave maneuvers to maintain kinetic stability
            float perp_x = -dir_z;
            float perp_y = dir_y * 0.5f;
            float perp_z = dir_x;

            float dynamic_offset = std::sin(execution_time * 3.0f + agent_id) * 0.8f;

            thrust_x = (dir_x * base_thrust * 0.4f) + (perp_x * base_thrust * 1.5f * dynamic_offset) - (vx * 0.4f);
            thrust_y = (dir_y * base_thrust * 0.4f) + (perp_y * base_thrust * 1.5f * dynamic_offset) - (vy * 0.4f);
            thrust_z = (dir_z * base_thrust * 0.4f) + (perp_z * base_thrust * 1.5f * dynamic_offset) - (vz * 0.4f);
        }

        // ====================================================================
        // RELATIVISTIC EMERGENCY OVERRIDE
        // ====================================================================

        float range_to_bh = std::sqrt(px * px + py * py + pz * pz);
        float emergency_threshold = config::physics::rs_horizon * 5.0f;

        if (range_to_bh < emergency_threshold) [[unlikely]] {
            float inv_range = (range_to_bh > 0.0001f) ? (1.0f / range_to_bh) : 0.0f;
            float escape_x = -px * inv_range;
            float escape_y = -py * inv_range;
            float escape_z = -pz * inv_range;

            float critical_thrust = base_thrust * 6.0f;
            thrust_x += escape_x * critical_thrust;
            thrust_y += escape_y * critical_thrust;
            thrust_z += escape_z * critical_thrust;
        }

        // Apply external gravitational field tensors and local propulsion
        float total_acc_x = field_acc_x + thrust_x;
        float total_acc_y = field_acc_y + thrust_y;
        float total_acc_z = field_acc_z + thrust_z;

        vx += total_acc_x * delta_time;
        vy += total_acc_y * delta_time;
        vz += total_acc_z * delta_time;

        px += vx * delta_time;
        py += vy * delta_time;
        pz += vz * delta_time;

        // 3. Write integrated state back to memory arena
        buffer.pos_x[i] = px;
        buffer.pos_y[i] = py;
        buffer.pos_z[i] = pz;

        buffer.vel_x[i] = vx;
        buffer.vel_y[i] = vy;
        buffer.vel_z[i] = vz;

        buffer.agent_state[i] = state;
        buffer.behavior_timer[i] = timer;
        buffer.target_pos_x[i] = tx;
        buffer.target_pos_y[i] = ty;
        buffer.target_pos_z[i] = tz;
    }

} // namespace stellar_agents