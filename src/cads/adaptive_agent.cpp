#include "agent_state.h"
#include "raymath.h"
#include <cmath>

namespace stellar_agents {

    // ============================================================================
    // CADS ADAPTIVE AGENT INTELLIGENCE ENGINE (HOMEOSTATIC STEERING CORTERS)
    // Standard compliance: Scoped inside the functional project namespace.
    // Processes active dissipative behavioral loops using localized state feedback.
    // ============================================================================

    // Unique structural flags to retain your custom ship specifications inside flat memory layout
    enum class ShipClass : uint8_t {
        LIGHT_FIGHTER = 0,
        HEAVY_FIGHTER = 1,
        CAPITAL_VESSEL = 2
    };

    // Static helper to map flat numerical IDs to physical ship characteristics without allocation
    [[nodiscard]] static constexpr float GetThrustPowerRating(const uint32_t p_agent_id) noexcept {
        const ShipClass type = static_cast<ShipClass>(p_agent_id % 3);
        if (type == ShipClass::HEAVY_FIGHTER)  return 7.5f;
        if (type == ShipClass::CAPITAL_VESSEL) return 24.0f;
        return 4.2f; // Default Light Fighter spec
    }

    // ============================================================================
    // ADAPTIVE BEHAVIORAL INTEGRATION MATRIX
    // Evaluates external gravitational vector fields against internal homeostatic goals.
    // Marked noexcept for frictionless distribution across the asynchronous jthread pool.
    // ============================================================================
    void MutateAdaptiveAgentState(
        const AgentState& p_current_state,
        AgentState& p_next_state,
        const Vector3& p_accumulated_gravity,
        const Vector3& p_attractor_position,
        const Vector3& p_target_planet_position,
        const float p_event_horizon_radius,
        const float p_execution_time,
        const float p_delta_time) noexcept
    {
        if (!p_current_state.is_active) [[unlikely]] {
            p_next_state.is_active = false;
            return;
        }

        // Register-caching physical core properties
        const Vector3 operationalPos = p_current_state.position;
        const Vector3 operationalVel = p_current_state.velocity;
        const uint32_t currentID = p_current_state.agent_id;
        const ShipClass shipType = static_cast<ShipClass>(currentID % 3);

        const float baseThrustPower = GetThrustPowerRating(currentID);
        const float currentRangeToBH = Vector3Distance(operationalPos, p_attractor_position);

        // 1. EXTRACT ENVIRONMENT FEEDBACK RADIAL DIRECTIONS
        const Vector3 outwardEscapeDirection = Vector3Normalize(Vector3Scale(p_accumulated_gravity, -1.0f));
        const Vector3 forwardFlightDirection = (Vector3LengthSqr(operationalVel) > 0.001f) ? Vector3Normalize(operationalVel) : Vector3Zero();
        Vector3 generatedThrustVector = Vector3Zero();

        // 2. KI-DECISION MATRIX: HOMEOSATIC AFTERBURNER SENSOR
        const float emergencyThreshold = p_event_horizon_radius * 5.0f;
        const bool emergencyAfterburnerActive = (currentRangeToBH < emergencyThreshold);

        if (emergencyAfterburnerActive) [[unlikely]] {
            // High-Alert Dissipative Loop: Channel maximum energy to override gravitational pull
            const float criticalAfterburnerThrust = baseThrustPower * 4.0f;
            generatedThrustVector = Vector3Add(
                Vector3Scale(forwardFlightDirection, baseThrustPower * 0.3f),
                Vector3Scale(outwardEscapeDirection, criticalAfterburnerThrust)
            );
        }
        else [[likely]] {
            // Standard Adaptive Trajectory Binding Pass
            const Vector3 vectorToPlanet = Vector3Subtract(p_target_planet_position, operationalPos);
            const float distanceToPlanet = Vector3Length(vectorToPlanet);
            const Vector3 directionToPlanet = (distanceToPlanet > 0.01f) ? Vector3Scale(vectorToPlanet, 1.0f / distanceToPlanet) : Vector3Zero();

            // High-Performance Coupling Path: Active tracking attraction field pulling toward Tamsa II
            constexpr float orbitHoldCoeff = 2.5f;
            const Vector3 planetAttractionThrust = Vector3Scale(directionToPlanet, (baseThrustPower * orbitHoldCoeff) / (distanceToPlanet + 0.5f));

            // Structural Weave Engine: Simulates avoidance vectors crossing passing asteroid streams
            const float weaveFrequency = (shipType == ShipClass::LIGHT_FIGHTER) ? 6.0f : 2.5f;
            const float weaveMagnitude = (shipType == ShipClass::LIGHT_FIGHTER) ? 2.0f : 0.5f;
            const float dynamicOffset = std::sin(p_execution_time * weaveFrequency + currentRangeToBH) * weaveMagnitude;

            Vector3 perpendicularEvasionVector = Vector3{ -operationalVel.z, operationalVel.y, operationalVel.x };
            if (Vector3LengthSqr(perpendicularEvasionVector) > 0.001f) {
                perpendicularEvasionVector = Vector3Normalize(perpendicularEvasionVector);
            }
            else {
                perpendicularEvasionVector = Vector3Zero();
            }

            // Combine standard baseline cruising thrust parameters
            generatedThrustVector = Vector3Add(
                Vector3Scale(forwardFlightDirection, baseThrustPower * 0.8f),
                Vector3Scale(outwardEscapeDirection, baseThrustPower * 1.2f)
            );

            // Inject homeostatic planetary binding and environmental wave mutations simultaneously
            generatedThrustVector = Vector3Add(generatedThrustVector, planetAttractionThrust);
            generatedThrustVector = Vector3Add(generatedThrustVector, Vector3Scale(perpendicularEvasionVector, dynamicOffset));
        }

        // 3. Classical Euler-Cromer chronological phase-space integration step
        const Vector3 totalMaturedAcceleration = Vector3Add(p_accumulated_gravity, generatedThrustVector);
        Vector3 nextVelocity = Vector3Add(operationalVel, Vector3Scale(totalMaturedAcceleration, p_delta_time));
        Vector3 nextPosition = Vector3Add(operationalPos, Vector3Scale(nextVelocity, p_delta_time));

        // Flush active parameters clean to the secondary shadow slice buffer
        p_next_state.position = nextPosition;
        p_next_state.velocity = nextVelocity;
        p_next_state.color = p_current_state.color;
        p_next_state.agent_id = currentID;
        p_next_state.type = p_current_state.type;
        p_next_state.is_active = true;
    }

} // namespace stellar_agents
