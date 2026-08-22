#pragma once
#include "raylib.h"

namespace Relativity {

    // FULL SIGNATURE
    // Calculates the light deflection using all physical parameters of the singularity.
    [[nodiscard]] Vector3 GetApparentPosition(
        Vector3 realPos,
        Vector3 camPos,
        Vector3 holePos,
        float mass,
        float eventHorizon
    ) noexcept;

    // OVERLOAD FOR BACKWARD COMPATIBILITY
    // Used by universe.cpp when only object position and camera position are provided.
    // Automatically drops deflection or uses standardized static baseline values.
    [[nodiscard]] inline Vector3 GetApparentPosition(Vector3 realPos, Vector3 camPos) noexcept {
        // CIG Performance-Pass: If no black hole parameters are supplied, 
        // light travels linearly without gravitational bending.
        [[maybe_unused]] Vector3 fallbackModifier = camPos;
        return realPos;
    }

}
