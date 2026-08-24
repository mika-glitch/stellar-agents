#include "agent_state.h"     // ZWINGEND: Liefert dem Compiler die Struktur AgentState
#include "engine_config.h"   // Holt config::physics::rs_horizon und die Massen-Konstanten
#include "raymath.h"         // Liefert Vector3Subtract, Vector3LengthSqr etc.

namespace stellar_agents {

    // ============================================================================
    // DETERMINISTIC STATE MUTATION OPERATOR (STATIC SCHWARZSCHILD MODEL)
    // Executes discrete chronological integrations across massless particle streams.
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

        // ========================================================================
        // MASSLESS CORE PHYSICS PASS: CENTRAL BLACK HOLE FIELD INJECTION
        // ========================================================================
        // Target: Fixed Origin Vector [0.0f, 0.0f, 0.0f] for the Central Black Hole
        constexpr Vector3 blackHolePosition = { 0.0f, 0.0f, 0.0f };
        Vector3 relativePosition = Vector3Subtract(blackHolePosition, operationalPosition);
        float distanceSquared = Vector3LengthSqr(relativePosition);

        // RELATIVISTIC DEALLOCATION GATEWAY (EVENT HORIZON KILL TRIGGER)
        // Hard boundary check against the Schwarzschild Radius compiled from config
        if (distanceSquared < (config::physics::rs_horizon * config::physics::rs_horizon)) [[unlikely]] {
            p_next_state.is_active = false; // Physisches Swap-and-Pop-Löschgatter
            return;
        }

        // Standard radial Newtonian attraction calculation serving as Schwarzschild baseline
        float gravityMagnitude = config::physics::black_hole_runtime_mass / distanceSquared;
        Vector3 gravityAcceleration = Vector3Scale(Vector3Normalize(relativePosition), gravityMagnitude);

        // Accumulate baseline incoming forces with local static central acceleration
        Vector3 totalAcceleration = Vector3Add(p_accumulated_field_acceleration, gravityAcceleration);

        // ========================================================================
        // CLASSICAL EULER-CROMER CHRONOLOGICAL PHASE-SPACE INTEGRATION STEP
        // ========================================================================
        operationalVelocity = Vector3Add(operationalVelocity, Vector3Scale(totalAcceleration, p_delta_time));
        operationalPosition = Vector3Add(operationalPosition, Vector3Scale(operationalVelocity, p_delta_time));

        // Flush mutated system states cleanly down to the isolated shadow slice buffer
        p_next_state.position = operationalPosition;
        p_next_state.velocity = operationalVelocity;
        p_next_state.color = p_current_state.color; // Pass color definitions unchanged across frames
        p_next_state.agent_id = p_current_state.agent_id;
        p_next_state.type = p_current_state.type;
        p_next_state.is_active = true;
    }

} // namespace stellar_agents
