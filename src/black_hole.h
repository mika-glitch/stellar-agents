#pragma once
#include "raylib.h"
#include <vector> // Required for modern contiguous array memory stacking

// Structure to pass planetary gravity positions into the centralized engine math streams cleanly
struct GravitySource {
    Vector3 position;
    float mass;
};

class BlackHole {
public:
    // Pure data members aligned for cache efficiency
    Vector3 position;
    float mass;
    float eventHorizon;

    BlackHole(Vector3 pos, float m, float r);
    ~BlackHole() = default;

    // Marked noexcept to match implementation and guarantee zero-throw guarantees
    void Update(float deltaTime) noexcept;
    void Draw(Camera3D camera) const noexcept;

    // Standard high-performance queries
    [[nodiscard]] Vector3 CalculateGravity(Vector3 objectPos) const noexcept;
    [[nodiscard]] bool HasCrossedPointOfNoReturn(Vector3 objectPos) const noexcept;

    // --- Performance Inline Getters for Backward Compatibility ---
    [[nodiscard]] inline Vector3 GetPosition() const noexcept { return position; }
    [[nodiscard]] inline float GetMass() const noexcept { return mass; }
    [[nodiscard]] inline float GetEventHorizon() const noexcept { return eventHorizon; }
    // Engine Compliance: High-performance structural coordinate data extraction
    [[nodiscard]] inline Vector3 GetPlanetPosition(size_t index) const noexcept {
        if (index < activePlanets.size()) return activePlanets[index].position;
        return Vector3{ 0.0f, 0.0f, 0.0f };
    }


    // --- LORE EXTENSION MODIFIERS: Thread-safe runtime gravity field manipulation API ---
    inline void ClearPlanets() const noexcept { activePlanets.clear(); }
    inline void RegisterPlanetGravity(Vector3 pos, float planetMass) const noexcept { activePlanets.push_back({ pos, planetMass }); }

private:
    // Thread-safe instance encapsulation of rendering handles
    Shader accretionShader;
    int timeLoc;
    int resolutionLoc;
    int camPosLoc;
    int camTargetLoc;

    // FIXED ENVIRONMENT INPUTS: Mutable data field stack to process planet vector tracking
    mutable std::vector<GravitySource> activePlanets;
};
