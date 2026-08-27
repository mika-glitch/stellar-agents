// ============================================================================
// [Core/CADS] AGENT STATE & PRIMITIVE DEFINITIONS
// Description: Defines foundational mathematical primitives (Vector3, Color) 
//              and classification enumerations optimized for Structure of 
//              Arrays (SoA) memory architectures.
// Standard: ISO C++20
// ============================================================================
#ifndef AGENT_STATE_H
#define AGENT_STATE_H

#include <cstdint>

namespace stellar_agents {

    /**
     * @brief 3-Dimensional Cartesian Vector Structure.
     * @details Provides a lightweight mathematical representation for spatial
     * coordinates and kinematic velocity vectors in 3D Euclidean space.
     * Formula: $\vec{v} = x\hat{i} + y\hat{j} + z\hat{k}$
     */
    struct Vector3 {
        float x{ 0.0f };
        float y{ 0.0f };
        float z{ 0.0f };
    };

    /**
     * @brief RGBA Color Structure.
     * @details Encapsulates normalized 8-bit color channels for rendering pipelines.
     */
    struct Color {
        uint8_t r{ 255 };
        uint8_t g{ 255 };
        uint8_t b{ 255 };
        uint8_t a{ 255 };
    };

    /**
     * @brief Entity Classification Enumeration.
     * @details Identifies the behavioral and physical classification of simulation nodes.
     */
    enum class AgentType : uint8_t {
        ATTRACTOR = 0,
        PASSIVE = 1,
        ADAPTIVE = 2
    };

    /**
     * @brief Host-Side Entity Staging Payload.
     * @details Refactored from the legacy contiguous agent footprint to act
     * as a temporary configuration vessel during initialization, prior to scattering
     * attributes into parallel Structure of Arrays (SoA) memory arenas.
     */
    struct AgentStagingPayload {
        Vector3 position{ 0.0f, 0.0f, 0.0f };
        Vector3 velocity{ 0.0f, 0.0f, 0.0f };
        Color   color{ 255, 255, 255, 255 };
        uint32_t agent_id{ 0 };
        AgentType type{ AgentType::PASSIVE };
        uint8_t is_active{ 1 };
    };

} // namespace stellar_agents

#endif // AGENT_STATE_H