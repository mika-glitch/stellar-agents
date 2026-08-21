#pragma once
#include "raylib.h"
#include "black_hole.h"
#include <vector>
#include <thread>

// Cache-aligned particle layout for maximum performance
struct Asteroid {
    Vector3 position;
    Vector3 velocity;
    Color color;
    bool active;
};

// Canonical Star Citizen lore planetary system coordinates
struct Planet {
    Vector3 position;
    Vector3 velocity;
    float radius;
    Color color;
    const char* name;
};

class Universe {
private:
    const BlackHole& blackHole;
    std::vector<Asteroid> asteroids;
    std::vector<Planet> planets;

    int maxAsteroids;
    int currentActiveCount;
    int livingAsteroidCount;
    unsigned int numThreads;

    // Internal thread worker task from your working version
    void ProcessAsteroidChunk(size_t startIdx, size_t endIdx, float dt);

public:
    Universe(const BlackHole& hole, int maxParticles);

    void Update(float dt);
    void Draw3D(Camera3D camera) const;
    void DrawHUD(Camera3D camera) const;

    // Interactive regulator controls
    void IncreaseParticleLoad();
    void DecreaseParticleLoad();

    // Infrastructure telemetry metrics
    int GetActiveParticleCount() const { return currentActiveCount; }
    int GetLivingParticleCount() const { return livingAsteroidCount; }
    int GetMaxParticleCount() const { return maxAsteroids; }
    unsigned int GetThreadCount() const { return numThreads; }
};
