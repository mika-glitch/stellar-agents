#ifndef AGENT_STATE_H
#define AGENT_STATE_H

#include "raylib.h"
#include <cstdint>

namespace stellar_agents {

    // ============================================================================
    // CADS AGENT CLASSIFICATION TAXONOMY
    // Defines the behavioral processing group of an individual adaptive entity.
    // Gates compilation routing inside the parallel optimization layers.
    // ============================================================================
    enum class AgentType : uint8_t {
        ATTRACTOR,  // Macroscopic phase-space anchors (Stars, Black Holes) enforcing global boundaries.
        PASSIVE,    // Non-adaptive entities (Asteroids, Space Debris) driven entirely by conservative field potentials.
        ADAPTIVE    // Complex homeostatic intelligence (Ships, Autonomous Swarms) running dissipative loops.
    };

    // ============================================================================
    // SOLID CACHE-ALIGNED AGENT STATE RECORD
    // Flat, layout-stable structure designed for high-throughput stream processing.
    // Suppresses polymorphic overhead (No virtual tables) to maximize L1/L2 data cache hit rates.
    // ============================================================================
    struct AgentState {
        Vector3 position{ 0.0f, 0.0f, 0.0f };    // 3D Cartesian coordinates in unwarped Euclidean space
        Vector3 velocity{ 0.0f, 0.0f, 0.0f };    // Instantaneous linear velocity translation vectors
        Color color{ WHITE };                    // Normalized standard hardware draw color payload
        uint32_t agent_id{ 0 };                  // Globally unique entity identification register index
        AgentType type{ AgentType::PASSIVE };    // Dynamic behavioral routing classifier token
        bool is_active{ true };                  // Thread-safe operational visibility and simulation status flag
    };

} // namespace stellar_agents

#endif // AGENT_STATE_H
#pragma once
