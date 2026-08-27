#ifndef RELATIVITY_H
#define RELATIVITY_H

#include "agent_state.h" // Provides our custom Vector3 definition

namespace stellar_agents {

    // ============================================================================
    // MATHEMATICAL SCHWARZSCHILD MATRIX RESOLUTION INDEX
    // Standard compliance: Fully scoped within the unified project namespace.
    // Handles relativistic ray-bending approximations over vector registers.
    // ============================================================================
    class Relativity final {
    public:
        /**
         * @brief Computes apparent relativistic position adjustments under gravitational lensing.
         * @param realPos Actual spatial coordinates of the entity.
         * @param camPos Current observer/camera world position.
         * @param holePos Center point of the gravitational singularity.
         * @param mass Effective mass driving the deflection metric.
         * @param eventHorizon Schwarzschild radius boundary limit.
         * @return Vector3 Apparent position distorted by space-time curvature.
         */
        [[nodiscard]] static Vector3 GetApparentPosition(
            const Vector3 realPos,
            const Vector3 camPos,
            const Vector3 holePos,
            float mass,
            float eventHorizon) noexcept;
    };

} // namespace stellar_agents

#endif // RELATIVITY_H