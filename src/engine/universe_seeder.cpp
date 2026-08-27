// ============================================================================
// [Core/Simulation] UNIVERSE SEEDER IMPLEMENTATION
// Description: Calculates Keplerian orbital mechanics and populates 
//              parallel Structure of Arrays (SoA) memory channels.
// Standard: ISO C++20
// ============================================================================
#define NOMINMAX
#define WIN32_LEAN_AND_MEAN

#include "engine/universe_seeder.h"
#include "engine/engine_config.h"
#include <random>
#include <cmath>

namespace stellar_agents {

    void UniverseSeeder::seed_initial_conditions(stellar_agents::EnvironmentMatrix& matrix) {
        auto& buffer = matrix.get_write_buffer();
        uint64_t idx = 0;

        /**
         * Helper Lambda: Direct SoA Vector Ingestion
         * Maps individual entity parameters directly into parallel contiguous memory blocks
         * to prevent dynamic reallocations and cache fragmentation during seeding.
         * Extended to support Autonomous Navigation FSM channels.
         */
        auto set_entity = [&](uint64_t i, float px, float py, float pz,
            float vx, float vy, float vz,
            uint8_t cr, uint8_t cg, uint8_t cb, uint8_t ca,
            uint32_t id, stellar_agents::AgentType type, uint8_t active,
            uint8_t state = 0, float timer = 0.0f, float tx = 0.0f, float ty = 0.0f, float tz = 0.0f) {

                buffer.pos_x[i] = px;
                buffer.pos_y[i] = py;
                buffer.pos_z[i] = pz;

                buffer.vel_x[i] = vx;
                buffer.vel_y[i] = vy;
                buffer.vel_z[i] = vz;

                buffer.color_r[i] = cr;
                buffer.color_g[i] = cg;
                buffer.color_b[i] = cb;
                buffer.color_a[i] = ca;

                buffer.agent_id[i] = id;
                buffer.agent_type[i] = static_cast<uint8_t>(type);
                buffer.is_active[i] = active;

                // Autonomous Navigation State Channels
                buffer.agent_state[i] = state;
                buffer.behavior_timer[i] = timer;
                buffer.target_pos_x[i] = tx;
                buffer.target_pos_y[i] = ty;
                buffer.target_pos_z[i] = tz;
            };

        /**
         * Keplerian Orbital Velocity Solver
         * Computes the tangential velocity vector for a stable circular orbit.
         * Formula: $v = \sqrt{\frac{G \cdot M}{r}}$
         */
        auto calc_orbit_vel = [](float rx, float rz) -> stellar_agents::Vector3 {
            const float radius = std::sqrt(rx * rx + rz * rz);
            const float velocity_scalar = std::sqrt(config::physics::primary_attractor_runtime_mass / radius) * config::simulation::orbital_velocity_multiplier;

            const float norm_x = rx / radius;
            const float norm_z = rz / radius;

            return { -velocity_scalar * norm_z, 0.0f, velocity_scalar * norm_x };
            };

        // Node 0: Primary Gravitational Singularity (Central Attractor - STATIONARY)
        {
            uint32_t current_id = static_cast<uint32_t>(idx++);
            set_entity(current_id,
                0.0f, 0.0f, 0.0f,
                0.0f, 0.0f, 0.0f,
                0, 0, 0, 255,
                current_id,
                stellar_agents::AgentType::ATTRACTOR,
                1
            );
        }

        // Node 1: Inner High-Density Mass Node (DYNAMIC)
        {
            uint32_t current_id = static_cast<uint32_t>(idx++);
            const float px = config::astrodynamics::inner_body_pos[0];
            const float pz = config::astrodynamics::inner_body_pos[2];
            const stellar_agents::Vector3 vel = calc_orbit_vel(px, pz);
            set_entity(current_id,
                px, 0.0f, pz,
                vel.x, vel.y, vel.z,
                210, 245, 255, 255,
                current_id,
                stellar_agents::AgentType::ATTRACTOR,
                1
            );
        }

        // Node 2: Outer Volumetric Gas Mass Node (DYNAMIC)
        {
            uint32_t current_id = static_cast<uint32_t>(idx++);
            const float px = config::astrodynamics::outer_body_pos[0];
            const float pz = config::astrodynamics::outer_body_pos[2];
            const stellar_agents::Vector3 vel = calc_orbit_vel(px, pz);
            set_entity(current_id,
                px, 0.0f, pz,
                vel.x, vel.y, vel.z,
                160, 32, 240, 255,
                current_id,
                stellar_agents::AgentType::ATTRACTOR,
                1
            );
        }

        // Node 3: Transgalactic Gateway Interface (STATIONARY PERIPHERAL)
        {
            uint32_t current_id = static_cast<uint32_t>(idx++);
            const float px = config::astrodynamics::gateway_node_pos[0];
            const float py = config::astrodynamics::gateway_node_pos[1];
            const float pz = config::astrodynamics::gateway_node_pos[2];

            set_entity(current_id,
                px, py, pz,
                0.0f, 0.0f, 0.0f,
                255, 69, 0, 255,
                current_id,
                stellar_agents::AgentType::ATTRACTOR,
                1
            );
        }

        std::mt19937 prng(1337);
        std::uniform_real_distribution<float> angle_dist(0.0f, 6.283185f);
        std::uniform_real_distribution<float> ast_r_dist(config::simulation::asteroid_spawn_radius_min, config::simulation::asteroid_spawn_radius_max);
        std::uniform_real_distribution<float> height_dist(-0.25f, 0.25f);
        std::uniform_real_distribution<float> target_dist(0.0f, 1.0f);

        // 1. SEEDING: Passive Debris Belt (Accretion Disk Particles)
        for (uint64_t i = 0; i < config::simulation::total_asteroids; ++i) {
            const float a = angle_dist(prng);
            const float r = ast_r_dist(prng);
            const float px = r * std::cos(a);
            const float pz = r * std::sin(a);
            const float py = height_dist(prng) * (r * 0.05f);

            const stellar_agents::Vector3 vel = calc_orbit_vel(px, pz);
            const uint32_t current_id = static_cast<uint32_t>(idx++);

            set_entity(current_id,
                px, py, pz,
                vel.x, vel.y, vel.z,
                200, 220, 255, 255,
                current_id,
                stellar_agents::AgentType::PASSIVE,
                1
            );
        }

        // Pre-fetch gateway coordinates for fleet spawn anchoring
        const float gw_x = config::astrodynamics::gateway_node_pos[0];
        const float gw_y = config::astrodynamics::gateway_node_pos[1];
        const float gw_z = config::astrodynamics::gateway_node_pos[2];

        // 2. SEEDING: Adaptive Navigational Entities (Autonomous Fleet Clusters)
        for (uint64_t i = 0; i < config::simulation::total_ships; ++i) {
            // Anchor spawn sequence to the gateway node with minor spatial jitter
            const float px = gw_x + height_dist(prng) * 2.0f;
            const float py = gw_y + height_dist(prng) * 2.0f;
            const float pz = gw_z + height_dist(prng) * 2.0f;

            // Initialize with zero velocity; proportional navigation FSM assumes immediate thrust control
            const float vx = 0.0f;
            const float vy = 0.0f;
            const float vz = 0.0f;

            // Stochastic routing decision for the initial objective
            float tx = 0.0f, ty = 0.0f, tz = 0.0f;
            float rand_target = target_dist(prng);

            if (rand_target < 0.33f) {
                // Target Inner Compact Mass Node (Planet 1)
                tx = config::astrodynamics::inner_body_pos[0];
                ty = config::astrodynamics::inner_body_pos[1];
                tz = config::astrodynamics::inner_body_pos[2];
            }
            else if (rand_target < 0.66f) {
                // Target Outer Volumetric Mass Node (Planet 2)
                tx = config::astrodynamics::outer_body_pos[0];
                ty = config::astrodynamics::outer_body_pos[1];
                tz = config::astrodynamics::outer_body_pos[2];
            }
            else {
                // Target synthetic geodesic coordinate (Asteroid debris field)
                float a = angle_dist(prng);
                float r = ast_r_dist(prng);
                tx = r * std::cos(a);
                ty = height_dist(prng) * (r * 0.05f);
                tz = r * std::sin(a);
            }

            const uint32_t current_id = static_cast<uint32_t>(idx++);
            const bool is_industrial_class = (i % 3 == 1);
            const uint8_t cr = is_industrial_class ? 255 : 80;
            const uint8_t cg = is_industrial_class ? 170 : 210;
            const uint8_t cb = is_industrial_class ? 40 : 255;

            // Transmit entity state including initialized FSM routing channels
            set_entity(current_id,
                px, py, pz,
                vx, vy, vz,
                cr, cg, cb, 255,
                current_id,
                stellar_agents::AgentType::ADAPTIVE,
                1,
                0, 0.0f, tx, ty, tz // Phase 0 (Transit), 0 timer, specific spatial target
            );
        }

        matrix.FlipBuffers();
    }

} // namespace stellar_agents