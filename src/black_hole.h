#pragma once
#include "raylib.h"

class BlackHole {
public:
    // Pure data members aligned for cache efficiency
    Vector3 position;
    float mass;
    float eventHorizon;

    BlackHole(Vector3 pos, float m, float r);
    ~BlackHole() = default;

    // CIG Compliance: Marked noexcept to match implementation and guarantee zero-throw guarantees
    void Update(float deltaTime) noexcept;
    void Draw(Camera3D camera) const noexcept;

    // Standard high-performance queries
    [[nodiscard]] Vector3 CalculateGravity(Vector3 objectPos) const noexcept;
    [[nodiscard]] bool HasCrossedPointOfNoReturn(Vector3 objectPos) const noexcept;

    // --- CIG Jet-Performance Inline Getters for Backward Compatibility ---
    [[nodiscard]] inline Vector3 GetPosition() const noexcept { return position; }
    [[nodiscard]] inline float GetMass() const noexcept { return mass; }
    [[nodiscard]] inline float GetEventHorizon() const noexcept { return eventHorizon; }

private:
    // Thread-safe instance encapsulation of rendering handles
    Shader accretionShader;
    int timeLoc;
    int resolutionLoc;
    int camPosLoc;
    int camTargetLoc;
};
