#ifndef RELATIVITY_H
#define RELATIVITY_H

#include "raylib.h"

namespace stellar_agents {

    // ============================================================================
    // EINSTEINIAN LENSING UTILITY INTERFACE
    // Static execution layer designed to resolve non-linear space-time optical path 
    // deflections. Marked final with deleted constructors to guarantee a strict 
    // zero-allocation compile-time utility footprint inside RAM.
    // ============================================================================
    class Relativity final {
    public:
        // Enforce pure functional data transformation constraints by disabling instantiation
        Relativity() = delete;
        ~Relativity() = delete;

        // Disallow copy/move operations across the global pipeline
        Relativity(const Relativity&) = delete;
        Relativity& operator=(const Relativity&) = delete;

        // ============================================================================
        // STANDARD EINSTEIN LENSING PROJECTION SOLVER
        // [[nodiscard]] forces calling pipelines to process the calculated coordinate matrix.
        // Marked noexcept to eliminate exceptional execution overhead inside background worker loops.
        // Parameters are passed by value to utilize direct hardware SIMD vector registers.
        // ============================================================================
        [[nodiscard]] static Vector3 GetApparentPosition(
            const Vector3 realPos,
            const Vector3 camPos,
            const Vector3 holePos,
            const float mass,
            const float eventHorizon) noexcept;
    };

} // namespace stellar_agents

#endif // RELATIVITY_H
