#pragma once
#include "raylib.h"
#include "black_hole.h"
#include <vector>
#include <thread>
#include <mutex>
#include <condition_variable>
#include <atomic>

// High-performance cache-aligned layout for massively parallel simulation passes
struct Asteroid {
    Vector3 position;
    Vector3 velocity;
    Color color;
    bool active;
};

// Canonical Star Citizen lore planetary system coordinates (Tamsa System baseline)
struct Planet {
    Vector3 position;
    Vector3 velocity;
    float radius;
    Color color;
    const char* name;
};

// Main simulation pipeline coordinator designed to scale across high-core-count processors
class Universe {
private:
    const BlackHole& blackHole;
    std::vector<Asteroid> asteroids;
    std::vector<Planet> planets;

    int maxAsteroids;
    int currentActiveCount;
    int livingAsteroidCount;
    unsigned int numThreads;

    // --- CIG Static Worker Pool Synchronization States ---
    std::vector<std::thread> workerPool;
    std::mutex poolMutex;
    std::condition_variable cvStart;
    std::condition_variable cvEnd;
    std::atomic<bool> stopPool{ false };
    std::atomic<int> activeWorkers{ 0 };
    float currentDeltaTime{ 0.0f };
    std::atomic<int> completedTasks{ 0 };
    int currentFrameSignal{ 0 };

    // --- CIG High-Performance Dynamic CPU Buffers ---
    mutable Vector3 currentCameraPos{ 0.0f, 0.0f, 0.0f };
    mutable std::vector<float> vertexBuffer;

    // Concurrent thread worker task executing slicing operations on dense memory arrays
    void WorkerLoop(unsigned int threadId);

public:
    // Lifecycle setup establishing memory reservations and system configurations
    Universe(const BlackHole& hole, int maxParticles);
    ~Universe();

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
