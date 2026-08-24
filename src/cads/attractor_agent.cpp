#include "agent_state.h"
#include "engine_config.h"
#include "raymath.h"
#include <cmath>

namespace stellar_agents {

    // ============================================================================
    // CADS ATTRACTOR AGENT STATE MANAGEMENT OPERATOR
    // Standard compliance: Fully scoped within the core functional namespace.
    // Updates macro-nodes (Planets) via synchronous deterministic orbit-tracks.
    // ============================================================================
    void MutateAttractorAgentState(
        const AgentState& p_current_state,
        AgentState& p_next_state,
        const float p_execution_time) noexcept
    {
        // Object 0 (Black Hole) rests absolute static at the origin vector
        if (p_current_state.agent_id == 0) [[unlikely]] {
            p_next_state = p_current_state;
            return;
        }

        Vector3 nextPosition = p_current_state.position;
        Vector3 nextVelocity = p_current_state.velocity;

        // Deterministic Keplerian orbit rail logic for the anonymized planetary cores
        if (p_current_state.agent_id == 1) {
            // Object 1: Diamond Core Planet (Static Radius: 650 Megameters)
            constexpr float orbitRadius = 650.0f;
            // Angular velocity derived from the pre-scaled central black hole mass constant
            float angularSpeed = std::sqrt(config::physics::black_hole_runtime_mass / (orbitRadius * orbitRadius * orbitRadius));
            float currentAngle = p_execution_time * angularSpeed;

            nextPosition = Vector3{ orbitRadius * std::cos(currentAngle), 0.0f, orbitRadius * std::sin(currentAngle) };
            nextVelocity = Vector3{ -orbitRadius * angularSpeed * std::sin(currentAngle), 0.0f, orbitRadius * angularSpeed * std::cos(currentAngle) };
        }
        else if (p_current_state.agent_id == 2) {
            // Object 2: Gas Giant Planet (Static Radius: 2000 Megameters)
            constexpr float orbitRadius = 2000.0f;
            float angularSpeed = std::sqrt(config::physics::black_hole_runtime_mass / (orbitRadius * orbitRadius * orbitRadius));
            float currentAngle = (p_execution_time * angularSpeed) + PI; // Phase-shifted by 180 degrees

            nextPosition = Vector3{ orbitRadius * std::cos(currentAngle), 0.0f, orbitRadius * std::sin(currentAngle) };
            nextVelocity = Vector3{ -orbitRadius * angularSpeed * std::sin(currentAngle), 0.0f, orbitRadius * angularSpeed * std::cos(currentAngle) };
        }

        // Flush the calculated orbital rail coordinates cleanly to the shadow buffer slice
        p_next_state.position = nextPosition;
        p_next_state.velocity = nextVelocity;
        p_next_state.color = p_current_state.color;
        p_next_state.agent_id = p_current_state.agent_id;
        p_next_state.type = p_current_state.type;
        p_next_state.is_active = true;
    }

} // namespace stellar_agents
