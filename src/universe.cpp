#include "universe.h"
#include "relativity.h"
#include "rlgl.h"       
#include "raymath.h"    // Required to resolve Vector3Scale, Vector3Add, and Vector3Subtract
#include <cstdlib>
#include <cmath>

// CIG Style Compliance: Standardized member initialization list layout.
Universe::Universe(const BlackHole& hole, int maxParticles)
    : blackHole(hole)
    , maxAsteroids(maxParticles)
    , currentActiveCount(40000)
    , livingAsteroidCount(40000)
    , currentFrameSignal(0)
    , currentDeltaTime(0.0f)
{
    // 1. ALLOCATE ASTEROID VECTOR STORAGE
    asteroids.reserve(maxAsteroids);
    for (int i = 0; i < maxAsteroids; i++) {
        float radius = 4.0f + (float)(rand() % 160) * 0.1f;
        float angle = (float)(rand() % 360) * DEG2RAD;

        Vector3 pos = { sinf(angle) * radius, ((float)(rand() % 20) - 10.0f) * 0.03f, cosf(angle) * radius };

        // Packed orbital mechanics setup
        float speed = sqrtf(blackHole.GetMass() / radius) * 1.0f;
        if (i % 3 == 0) speed *= 0.42f; // Decay spiral mechanics

        Vector3 vel = { -cosf(angle) * speed, 0.0f, sinf(angle) * speed };
        vel.y += ((float)(rand() % 20) - 10.0f) * 0.005f;

        int type = rand() % 4;
        Color astColor = GRAY;
        if (type == 0) astColor = LIGHTGRAY;
        if (type == 1) astColor = DARKGRAY;
        if (type == 2) astColor = Color{ 165, 200, 255, 255 };

        asteroids.push_back(Asteroid{ pos, vel, astColor, true });
    }

    // 2. INITIALIZE CELESTIAL BODIES
    float r1 = 6.5f;
    float speed1 = sqrtf(blackHole.GetMass() / r1) * 1.0f;
    planets.push_back(Planet{ { 0.0f, 0.0f, r1 }, { speed1, 0.0f, 0.0f }, 0.25f, Color{ 200, 240, 255, 255 }, "TAMSA I (Chthonic Diamond Core)" });

    float r2 = 12.0f;
    float speed2 = sqrtf(blackHole.GetMass() / r2) * 1.0f;
    planets.push_back(Planet{ { 0.0f, 0.0f, -r2 }, { -speed2, 0.0f, 0.0f }, 0.75f, Color{ 110, 90, 210, 255 }, "TAMSA II (Gas Giant)" });

    // 3. THREAD WORKER POOL SPIN-UP
    numThreads = std::thread::hardware_concurrency();
    if (numThreads == 0) numThreads = 8;

    vertexBuffer.resize(maxAsteroids * 3, 0.0f);

    workerPool.reserve(numThreads);
    for (unsigned int i = 0; i < numThreads; ++i) {
        workerPool.push_back(std::thread(&Universe::WorkerLoop, this, i));
    }
}

Universe::~Universe() {
    stopPool = true;
    cvStart.notify_all();
    for (auto& worker : workerPool) {
        if (worker.joinable()) worker.join();
    }
}

void Universe::WorkerLoop(unsigned int threadId) {
    int lastProcessedFrame = 0;

    while (!stopPool) {
        std::unique_lock<std::mutex> lock(poolMutex);

        // Block threads until frame signal increments
        cvStart.wait(lock, [this, lastProcessedFrame]() {
            return currentFrameSignal > lastProcessedFrame || stopPool;
            });

        if (stopPool) break;
        lastProcessedFrame = currentFrameSignal;

        size_t totalCount = static_cast<size_t>(currentActiveCount);
        size_t chunkSize = totalCount / numThreads;
        size_t startIdx = threadId * chunkSize;
        size_t endIdx = (threadId == numThreads - 1) ? totalCount : startIdx + chunkSize;

        // Extract central singularity data to pass down to localized register lookups
        const Vector3 corePos = blackHole.GetPosition();
        const float coreMass = blackHole.GetMass();
        const float coreHorizon = blackHole.GetEventHorizon();
        const Vector3 activeCamPos = currentCameraPos;

        lock.unlock();

        float dt = currentDeltaTime;
        for (size_t i = startIdx; i < endIdx; ++i) {
            auto& ast = asteroids[i];
            size_t vIdx = i * 3;

            if (!ast.active) {
                vertexBuffer[vIdx] = 99999.0f; // Clip off-screen mapping target
                continue;
            }

            if (blackHole.HasCrossedPointOfNoReturn(ast.position)) {
                ast.active = false;
                vertexBuffer[vIdx] = 99999.0f;
                continue;
            }

            // A) Background Parallel Physics Calculations
            Vector3 gravityAcceleration = blackHole.CalculateGravity(ast.position);
            ast.velocity = Vector3Add(ast.velocity, Vector3Scale(gravityAcceleration, dt));
            ast.position = Vector3Add(ast.position, Vector3Scale(ast.velocity, dt));

            // B) Background Parallel Relativistic Optical Transformation Pass
            Vector3 apparentPos = Relativity::GetApparentPosition(ast.position, activeCamPos, corePos, coreMass, coreHorizon);

            vertexBuffer[vIdx] = apparentPos.x;
            vertexBuffer[vIdx + 1] = apparentPos.y;
            vertexBuffer[vIdx + 2] = apparentPos.z;
        }

        lock.lock();
        completedTasks++;
        if (completedTasks == static_cast<int>(numThreads)) {
            cvEnd.notify_one();
        }
    }
}

void Universe::Update(float dt) {
    if (currentActiveCount > 0) {
        {
            std::lock_guard<std::mutex> lock(poolMutex);
            currentDeltaTime = dt;
            completedTasks = 0;
            currentFrameSignal++;
        }
        cvStart.notify_all();

        std::unique_lock<std::mutex> lock(poolMutex);
        cvEnd.wait(lock, [this]() { return completedTasks == static_cast<int>(numThreads); });
    }

    // Recalculate survival statistics inside serial master frame passes
    int survivors = 0;
    for (int i = 0; i < currentActiveCount; ++i) {
        if (asteroids[i].active) survivors++;
    }
    livingAsteroidCount = survivors;

    // Track deterministic trajectory updates of planetary nodes using raymath vectors
    for (auto& planet : planets) {
        Vector3 planetGravity = blackHole.CalculateGravity(planet.position);
        planet.velocity = Vector3Add(planet.velocity, Vector3Scale(planetGravity, dt));
        planet.position = Vector3Add(planet.position, Vector3Scale(planet.velocity, dt));
    }
}

void Universe::Draw3D(Camera3D camera) const {
    rlDisableDepthTest();
    BeginMode3D(camera);

    // Dynamic thread cache feed mechanism
    currentCameraPos = camera.position;

    // Pure dynamic streaming render loop bypassing pipeline calculation blocks entirely
    for (int i = 0; i < currentActiveCount; ++i) {
        if (asteroids[i].active) {
            size_t vIdx = static_cast<size_t>(i) * 3;
            Vector3 cachedApparentPos = { vertexBuffer[vIdx], vertexBuffer[vIdx + 1], vertexBuffer[vIdx + 2] };
            DrawPoint3D(cachedApparentPos, asteroids[i].color);
        }
    }

    // Direct standalone planetary updates
    const Vector3 holePos = blackHole.GetPosition();
    const float holeMass = blackHole.GetMass();
    const float holeHorizon = blackHole.GetEventHorizon();
    for (const auto& planet : planets) {
        Vector3 apparentPos = Relativity::GetApparentPosition(planet.position, camera.position, holePos, holeMass, holeHorizon);
        DrawSphere(apparentPos, planet.radius, planet.color);
    }

    EndMode3D();
    rlEnableDepthTest();
}

void Universe::DrawHUD(Camera3D camera) const {
    const Vector3 holePos = blackHole.GetPosition();
    const float holeMass = blackHole.GetMass();
    const float holeHorizon = blackHole.GetEventHorizon();

    for (const auto& planet : planets) {
        Vector3 apparentPos = Relativity::GetApparentPosition(planet.position, camera.position, holePos, holeMass, holeHorizon);
        Vector2 screenPos = GetWorldToScreen(apparentPos, camera);

        if (screenPos.x > 0 && screenPos.x < GetScreenWidth() && screenPos.y > 0 && screenPos.y < GetScreenHeight()) {
            DrawText(planet.name, (int)screenPos.x + 12, (int)screenPos.y - 6, 14, GREEN);
            DrawCircleLines((int)screenPos.x, (int)screenPos.y, 8, GREEN);
        }
    }
}

void Universe::IncreaseParticleLoad() {
    currentActiveCount += 4000;
    if (currentActiveCount > maxAsteroids) currentActiveCount = maxAsteroids;

    for (int i = 0; i < currentActiveCount; ++i) {
        if (!asteroids[i].active) {
            float radius = 4.0f + (float)(rand() % 160) * 0.1f;
            float angle = (float)(rand() % 360) * DEG2RAD;
            asteroids[i].position = { sinf(angle) * radius, ((float)(rand() % 20) - 10.0f) * 0.03f, cosf(angle) * radius };
            float speed = sqrtf(blackHole.GetMass() / radius) * 1.0f;
            if (i % 3 == 0) speed *= 0.42f;
            asteroids[i].velocity = { -cosf(angle) * speed, 0.0f, sinf(angle) * speed };
            asteroids[i].active = true;
        }
    }
}

void Universe::DecreaseParticleLoad() {
    currentActiveCount -= 4000;
    if (currentActiveCount < 0) currentActiveCount = 0;
}
