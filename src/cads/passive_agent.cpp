#include "agent_state.h"
#include "raymath.h"
#include <cmath>

namespace stellar_agents {

    // ============================================================================
    // CADS PASSIVE AGENT DYNAMICS EVOLUTION ENGINE
    // Standard compliance: Fully encapsulated inside the functional project namespace.
    // Processes non-adaptive kinematic updates under local field potentials.
    // ============================================================================

    // ============================================================================
    // DETERMINISTIC STATE MUTATION OPERATOR
    // Executes discrete chronological integrations across non-conservative particle streams.
    // Marked noexcept to guarantee zero execution context switches inside thread tasks.
    // ============================================================================
    void MutatePassiveAgentState(
        const AgentState& p_current_state,
        AgentState& p_next_state,
        const Vector3& p_accumulated_field_acceleration,
        const float p_delta_time) noexcept
    {
        // Fast-track evaluation: Terminate structural update cycles for absorbed entities
        if (!p_current_state.is_active) [[unlikely]] {
            p_next_state.is_active = false;
            return;
        }

        // Register-caching the incoming spatial translation parameters
        Vector3 operationalPosition = p_current_state.position;
        Vector3 operationalVelocity = p_current_state.velocity;

        // 1. Classical Euler-Cromer chronological phase-space integration step
        // High performance vectorized integration pattern native to hardware floating-point lines
        operationalVelocity = Vector3Add(operationalVelocity, Vector3Scale(p_accumulated_field_acceleration, p_delta_time));
        operationalPosition = Vector3Add(operationalPosition, Vector3Scale(operationalVelocity, p_delta_time));

        // 2. Flush mutated system states cleanly down to the isolated shadow slice buffer
        p_next_state.position = operationalPosition;
        p_next_state.velocity = operationalVelocity;
        p_next_state.color = p_current_state.color; // Pass color definitions unchanged across frames
        p_next_state.agent_id = p_current_state.agent_id;
        p_next_state.type = p_current_state.type;
        p_next_state.is_active = true;
    }

} // namespace stellar_agents