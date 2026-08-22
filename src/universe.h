#pragma once
#include "raylib.h"
#include "black_hole.h"
#include <vector>
#include <thread>

// High-performance cache-aligned layout for massively parallel simulation passes
struct Asteroid {
    Vector3 position;   // 12 bytes - SIMD-friendly continuous alignment
    Vector3 velocity;   // 12 bytes - Linear momentum vectors
    Color color;        // 4 bytes  - Render color profile
    bool active;        // 1 byte   - Lifecycle state flag for thread pruning
};

// Canonical Star Citizen lore planetary system coordinates (Tamsa System baseline)
struct Planet {
    Vector3 position;   // 12 bytes - Orbital world coordinates
    Vector3 velocity;   // 12 bytes - Keplerian velocity metrics
    float radius;       // 4 bytes  - Physical scale bounds
    Color color;        // 4 bytes  - Mesh/Sphere coloration
    const char* name;   // 8 bytes  - UI HUD localization anchor text
};

// Main simulation pipeline coordinator designed to scale across high-core-count processors
class Universe {
private:
    const BlackHole& blackHole;         // Central singularity mass modifier
    std::vector<Asteroid> asteroids;    // Packed Structure of Arrays (SoA) layout for hot-loop cache locality
    std::vector<Planet> planets;        // Tracked planetary instances orbiting the core

    int maxAsteroids;                   // Hard memory allocation boundary limits
    int currentActiveCount;             // Dynamically dispatched worker iteration count
    int livingAsteroidCount;            // Verified survival count post-event-horizon pass
    unsigned int numThreads;            // Evaluated physical/logical core layout footprint

    // Concurrent thread worker task executing slicing operations on dense memory arrays
    void ProcessAsteroidChunk(size_t startIdx, size_t endIdx, float dt);

public:
    // Lifecycle setup establishing memory reservations and system configurations
    Universe(const BlackHole& hole, int maxParticles);
    ~Universe() = default;

    // Core processing and integration routines executed every frame step
    void Update(float dt);

    // Low-level graphics pipeline execution stages (Const-qualified for thread-safe rendering jobs)
    void Draw3D(Camera3D camera) const;
    void DrawHUD(Camera3D camera) const;

    // Dynamic scale regulators for testing core thread workloads under varying pressure
    void IncreaseParticleLoad();
    void DecreaseParticleLoad();

    // Mission control telemetry metrics for system profiling and optimization audits
    [[nodiscard]] int GetActiveParticleCount() const noexcept { return currentActiveCount; }
    [[nodiscard]] int GetLivingParticleCount() const noexcept { return livingAsteroidCount; }
    [[nodiscard]] int GetMaxParticleCount() const noexcept { return maxAsteroids; }
    [[nodiscard]] unsigned int GetThreadCount() const noexcept { return numThreads; }
};
