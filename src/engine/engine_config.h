// ============================================================================
// [AAA/Core] ENGINE CONFIGURATION
// Description: Static compile-time constants for visual metrics, physical 
//              scaling, spatial boundaries, and SoA capacity bounds.
// Standard: ISO C++20 
// ============================================================================
#ifndef ENGINE_CONFIG_H
#define ENGINE_CONFIG_H

#include <cstdint> // Required for fixed-width integer types (uint64_t)

namespace config {

    namespace visual {
        // Core Display Metrics
        inline constexpr float screen_width = 1920.0f;
        inline constexpr float screen_height = 1080.0f;
        inline constexpr float aspect_ratio = screen_width / screen_height;
        inline constexpr float fov_degrees = 80.0f;

        // Volumetric Raymarching Radii
        inline constexpr float radius_inner_body = 0.5f;
        inline constexpr float radius_outer_body = 2.0f;
        inline constexpr float radius_gateway_node = 1.0f;
    }

    namespace physics {
        // Engine Scaling Constants
        inline constexpr float k_mass = 40.0f;
        inline constexpr float k_scale_mm = 1.0f;

        // Mass Distribution (Solar Masses)
        // Stabilized mass configuration: Base mass scalar reduced to prevent 
        // integrator instability at high time dilation.
        inline constexpr float primary_attractor_mass_sol = 6.0f;
        inline constexpr float primary_attractor_runtime_mass = primary_attractor_mass_sol * k_mass;

        inline constexpr float inner_body_mass_sol = 0.000025f;
        inline constexpr float inner_body_runtime_mass = 0.2f;

        inline constexpr float outer_body_mass_sol = 0.000266f;
        inline constexpr float outer_body_runtime_mass = 0.8f;

        inline constexpr float gateway_node_runtime_mass = 0.0f;

        /**
         * Schwarzschild Metric Boundaries
         * Normalized boundary metrics relative to the 1.0f mass scalar.
         *
         * Formula: $r_s = \frac{2GM}{c^2}$ (Event Horizon)
         * Formula: $r_{ps} = \frac{3GM}{c^2}$ (Photon Sphere)
         * Formula: $r_{isco} = \frac{6GM}{c^2}$ (Innermost Stable Circular Orbit)
         */
        inline constexpr float rs_horizon = 0.00443f;
        inline constexpr float photon_sphere = 0.00664f;
        inline constexpr float isco_limit = 0.01329f;
    }

    namespace simulation {
        // Data-Oriented Array Bounds (SoA Capacities)
        inline constexpr uint64_t total_asteroids = 1500000 + 4 ;
        inline constexpr uint64_t total_ships = 1000 + 4;
        
        // Total capacity includes standard agents + 4 primary gravitational attractors
        inline constexpr uint64_t total_agent_capacity = total_asteroids + total_ships + 4;

        // Kinematic Spawning Logistics
        inline constexpr float asteroid_spawn_radius_min = 8.0f;
        inline constexpr float asteroid_spawn_radius_max = 50.0f;
        inline constexpr float ship_spawn_radius_min = 0.1f;
        inline constexpr float ship_spawn_radius_max = 0.2f;
        inline constexpr float orbital_velocity_multiplier = 1.2f;
    }

    namespace astrodynamics {
        /**
         * Orbital Ephemeris (in Megameters)
         * Coordinates defined relative to the primary barycenter.
         * Spatial distance formula: $r = \sqrt{x^2 + y^2 + z^2}$
         */

         // Inner Attractor: ~50% within the accretion disk boundary (Radius = 15.0 MM)
        inline constexpr float inner_body_pos[3] = { 25.0f, 0.0f, 0.0f };

        // Outer Attractor: Marginally outside the accretion disk (Radius = ~32.0 MM)
        // Calculated via: $\sqrt{(-22.627)^2 + (22.627)^2} = 32.0$
        inline constexpr float outer_body_pos[3] = { -40.0f, 0.0f, 40.0f };

        // Trans-Orbital Gateway Node: Deep space vector (Radius = 40.0 MM)
        // Calculated via: $\sqrt{(28.284)^2 + (-28.284)^2} = 40.0$
        inline constexpr float gateway_node_pos[3] = { 50.0f, 0.0f, -50.0f };
    }

} // namespace config

#endif // ENGINE_CONFIG_H