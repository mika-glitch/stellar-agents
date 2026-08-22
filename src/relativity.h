#ifndef RELATIVITY_H
#define RELATIVITY_H

#include "raylib.h"

namespace godot_hpc { // Clean explicit namespace mapping to block symbol pollution

    // ============================================================================
    // EINSTEINIAN LENSING UTILITY INTERFACE
    // 'final' class modifier signals the compiler that no downstream inheritance 
    // occurs, maximizing structural dead-code elimination passes.
    // ============================================================================
    class Relativity final {
    public:
        // ============================================================================
        // STATIC UTILITY ARCHITECTURE LOCK
        // Explicitly deletes the default constructor and destructor registers.
        // This enforces a strict zero-allocation utility footprint in RAM.
        // ============================================================================
        Relativity() = delete;
        ~Relativity() = delete;

        // ============================================================================
        // STANDARD EINSTEIN LENSING PROJECTION PROTOTYPE
        // [[nodiscard]] forces caller logic to evaluate the computed coordinate matrix.
        // Passed by copy to leverage direct hardware CPU SIMD vector registers.
        // ============================================================================
        [[nodiscard]] static Vector3 GetApparentPosition(
            const Vector3 realPos,
            const Vector3 camPos,
            const Vector3 holePos,
            const float mass,
            const float eventHorizon) noexcept; // Guarantees function never throws execution interrupts
    };

} // namespace sterllar_agents

#endif // RELATIVITY_H
