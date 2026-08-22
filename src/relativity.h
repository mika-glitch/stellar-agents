#ifndef RELATIVITY_H
#define RELATIVITY_H

#include "raylib.h"

namespace godot_hpc {

    class Relativity final {
    public:
        // Enforces zero-allocation utility architectures by deleting constructors
        Relativity() = delete;
        ~Relativity() = delete;

        // Standardized Einstein lensing projection pass running with strict exception-free modifiers
        [[nodiscard]] static Vector3 GetApparentPosition(
            Vector3 realPos,
            Vector3 camPos,
            Vector3 holePos,
            float mass,
            float eventHorizon) noexcept;
    };

} // namespace godot_hpc

#endif // RELATIVITY_H
