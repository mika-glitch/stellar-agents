#include "universe.h"
#include "relativity.h"
#include "rlgl.h"
#include "raymath.h"
#include <cstdlib>
#include <cmath>

// CIG Style Compliance: Standardized member initialization list layout.
// All initializations strictly match the class member declaration order.
Universe::Universe(const BlackHole& hole, int maxParticles)
    : blackHole(hole)
    , maxAsteroids(maxParticles)
    , currentActiveCount(40000)
    , livingAsteroidCount(40000)
{
    asteroids.reserve(maxAsteroids);
    for (int i = 0; i < maxAsteroids; i++) {
        float radius = 4.0f + (float)(rand() % 160) * 0.1f;
        float angle = (float)(rand() % 360) * DEG2RAD;

        Vector3 pos = { sinf(angle) * radius, ((float)(rand() % 20) - 10.0f) * 0.03f, cosf(angle) * radius };

        // Locked in: Full 100% Keplerian orbital velocity profile
        float speed = sqrtf(blackHole.GetMass() / radius) * 1.0f;
        if (i % 3 == 0) speed *= 0.42f; // Decay spiral trigger for accretion flow

        Vector3 vel = { -cosf(angle) * speed, 0.0f, sinf(angle) * speed };
        vel.y += ((float)(rand() % 20) - 10.0f) * 0.005f;

        int type = rand() % 4;
        Color astColor = GRAY;
        if (type == 0) astColor = LIGHTGRAY;
        if (type == 1) astColor = DARKGRAY;
        if (type == 2) astColor = Color{ 165, 200, 255, 255 }; // High-energy plasma tint

        asteroids.push_back(Asteroid{ pos, vel, astColor, true });
    }

    // Initialize canonical Star Citizen Tamsa system planets with stable orbital velocity setups
    float r1 = 6.5f;
    float speed1 = sqrtf(blackHole.GetMass() / r1) * 1.0f;
    planets.push_back(Planet{ { 0.0f, 0.0f, r1 }, { speed1, 0.0f, 0.0f }, 0.25f, Color{ 200, 240, 255, 255 }, "TAMSA I (Chthonic Diamond Core)" });

    float r2 = 12.0f;
    float speed2 = sqrtf(blackHole.GetMass() / r2) * 1.0f;
    planets.push_back(Planet{ { 0.0f, 0.0f, -r2 }, { -speed2, 0.0f, 0.0f }, 0.75f, Color{ 110, 90, 210, 255 }, "TAMSA II (Gas Giant)" });

    numThreads = std::thread::hardware_concurrency();
    if (numThreads == 0) numThreads = 16;
}

// Thread-safe chunk processing pass executed concurrently across CPU worker boundaries
void Universe::ProcessAsteroidChunk(size_t startIdx, size_t endIdx, float dt) {
    for (size_t i = startIdx; i < endIdx; ++i) {
        auto& ast = asteroids[i];
        if (!ast.active) continue;

        // Eliminate particles instantly when breaching the event horizon boundary
        if (blackHole.HasCrossedPointOfNoReturn(ast.position)) {
            ast.active = false;
            continue;
        }

        // Vectorized relativistic Newtonian gravity integration pass
        Vector3 gravityAcceleration = blackHole.CalculateGravity(ast.position);
        ast.velocity = Vector3Add(ast.velocity, Vector3Scale(gravityAcceleration, dt));
        ast.position = Vector3Add(ast.position, Vector3Scale(ast.velocity, dt));
    }
}

void Universe::Update(float dt) {
    // High-performance CPU job dispatch block
    if (currentActiveCount > 0) {
        std::vector<std::thread> workers;
        workers.reserve(numThreads);
        size_t chunkSize = currentActiveCount / numThreads;

        // Dispatch data chunks across local physical core layout asynchronously
        for (unsigned int t = 1; t <= numThreads; ++t) {
            size_t startIdx = (t - 1) * chunkSize;
            size_t endIdx = (t == numThreads) ? currentActiveCount : startIdx + chunkSize;
            workers.push_back(std::thread(&Universe::ProcessAsteroidChunk, this, startIdx, endIdx, dt));
        }

        // Frame synchronization boundary (Join all dispatched worker instances)
        for (auto& worker : workers) {
            if (worker.joinable()) worker.join();
        }
    }

    // Recalculate survivor counts to refresh HUD statistics in real-time
    int survivors = 0;
    for (int i = 0; i < currentActiveCount; ++i) {
        if (asteroids[i].active) survivors++;
    }
    livingAsteroidCount = survivors;

    // Update planetary paths on the main simulation dispatcher thread
    for (auto& planet : planets) {
        Vector3 gravity = blackHole.CalculateGravity(planet.position);
        planet.velocity = Vector3Add(planet.velocity, Vector3Scale(gravity, dt));
        planet.position = Vector3Add(planet.position, Vector3Scale(planet.velocity, dt));
    }
}

// Const-qualified 3D rendering pass utilizing the fixed Einsteinian lensing wrappers
void Universe::Draw3D(Camera3D camera) const {
    rlDisableDepthTest();
    BeginMode3D(camera);

    // Cache physical parameters locally to maximize CPU register utilization inside hot loops
    const Vector3 holePos = blackHole.GetPosition();
    const float holeMass = blackHole.GetMass();
    const float holeHorizon = blackHole.GetEventHorizon();

    // Render relativistic field of lensed asteroid particles
    for (int i = 0; i < currentActiveCount; ++i) {
        const auto& ast = asteroids[i];
        if (ast.active) {
            Vector3 apparentPos = Relativity::GetApparentPosition(ast.position, camera.position, holePos, holeMass, holeHorizon);
            DrawPoint3D(apparentPos, ast.color);
        }
    }

    // Render relativistic field of lensed celestial bodies
    for (const auto& planet : planets) {
        Vector3 apparentPos = Relativity::GetApparentPosition(planet.position, camera.position, holePos, holeMass, holeHorizon);
        DrawSphere(apparentPos, planet.radius, planet.color);
    }

    EndMode3D();
    rlEnableDepthTest();
}

// Projection pass transforming 3D relativistic apparent data into screen space UI anchors
void Universe::DrawHUD(Camera3D camera) const {
    const Vector3 holePos = blackHole.GetPosition();
    const float holeMass = blackHole.GetMass();
    const float holeHorizon = blackHole.GetEventHorizon();

    for (const auto& planet : planets) {
        Vector3 apparentPos = Relativity::GetApparentPosition(planet.position, camera.position, holePos, holeMass, holeHorizon);
        Vector2 screenPos = GetWorldToScreen(apparentPos, camera);

        // Screen clipping detection boundary checks
        if (screenPos.x > 0 && screenPos.x < GetScreenWidth() && screenPos.y > 0 && screenPos.y < GetScreenHeight()) {
            DrawText(planet.name, (int)screenPos.x + 12, (int)screenPos.y - 6, 14, GREEN);
            DrawCircleLines((int)screenPos.x, (int)screenPos.y, 8, GREEN);
        }
    }
}

void Universe::IncreaseParticleLoad() {
    currentActiveCount += 4000;
    if (currentActiveCount > maxAsteroids) currentActiveCount = maxAsteroids;

    // Respawn reallocated array blocks back into operational tracking bounds
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