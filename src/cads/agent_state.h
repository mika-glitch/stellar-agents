#ifndef AGENT_STATE_H
#define AGENT_STATE_H

#include "raylib.h"
#include <cstdint>

namespace stellar_agents {

    // Unique core identification markers mapped from your functional pipeline
    enum class AgentType : uint8_t {
        ATTRACTOR = 0,
        PASSIVE = 1,
        ADAPTIVE = 2
    };

    // ============================================================================
    // CADER AGENT CORE DATA STRUCTURE (FLAT CUSTOM DATA-ORIENTED DESIGN)
    // Clean-Code enforcement: Strictly standalone block to suppress C1014 loops.
    // Maps perfectly to the usage in your physics and render subsystems.
    // ============================================================================
    struct AgentState {
        Vector3 position{ 0.0f, 0.0f, 0.0f };    // 12 Bytes
        Vector3 velocity{ 0.0f, 0.0f, 0.0f };    // 12 Bytes
        Color color{ WHITE };                    // 4 Bytes
        uint32_t agent_id{ 0 };                  // 4 Bytes
        AgentType type{ AgentType::PASSIVE };    // 1 Byte
        bool is_active{ true };                  // 1 Byte
    };

} // namespace stellar_agents

#endif // AGENT_STATE_H
