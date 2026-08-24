#ifndef ENGINE_CONFIG_H
#define ENGINE_CONFIG_H

namespace config {
    namespace physics {
        // Core Scaling Multipliers
        inline constexpr float k_mass = 40.0f;          // Scales M_sol into stable runtime floats
        inline constexpr float k_scale_mm = 1.0f;      // 1.0f Unit in simulation = 1.0 Megameter (MM)

        // ========================================================================
        // MASS LAYOUT (CALIBRATED IN SOLAR MASSES - M_SOL)
        // ========================================================================
        // Object 0: Central Black Hole (Fixed Attractor at Origin)
        inline constexpr float black_hole_mass_sol = 15.000000f;
        inline constexpr float black_hole_runtime_mass = black_hole_mass_sol * k_mass; // Evaluates to 600.0f

        // Object 1: Diamond Core Planet (Dynamic Orbital Agent - Mass omitted for O(N) performance)
        inline constexpr float planet_diamond_core_mass_sol = 0.000025f; // DOCUMENTARY ONLY
        inline constexpr float planet_diamond_core_runtime_mass = 0.0f;  // Rest mass zero in active kinetics

        // Object 2: Gas Giant Planet (Dynamic Orbital Agent - Mass omitted for O(N) performance)
        inline constexpr float planet_gas_giant_mass_sol = 0.000266f;    // DOCUMENTARY ONLY
        inline constexpr float planet_gas_giant_runtime_mass = 0.0f;     // Rest mass zero in active kinetics

        // Object 3: Wormhole Gateway Node (Massless Node / Adaptive Spawner)
        inline constexpr float gateway_wormhole_runtime_mass = 0.0f;

        // ========================================================================
        // RELATIVISTIC THRESHOLDS & BOUNDARIES (UNIT: MEGAMETERS)
        // ========================================================================
        inline constexpr float rs_horizon = 0.0443f;     // Schwarzschild Radius (~44.3 km)
        inline constexpr float photon_sphere = 0.0664f;  // Light Instability Boundary (~66.4 km)
        inline constexpr float isco_limit = 0.1329f;     // Innermost Stable Circular Orbit (~132.9 km)
    }
}

#endif // ENGINE_CONFIG_H
