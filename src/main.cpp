#include "raylib.h"
#include "raymath.h"
#include "agent_state.h"
#include "engine_config.h"
#include "environment_matrix.h"
#include "physics_evolution.h"
#include "render_core.h"
#include <random>
#include <cstdint>
#include <algorithm>
#include <cmath>

int main() {
    // Hardware Context Initialization
    SetConfigFlags(FLAG_MSAA_4X_HINT);
    InitWindow(1920, 1080, "Stellar Agents - Autonomous CADS Sandbox Engine v4.0 (Schwarzschild Edition)");
    SetTargetFPS(60);

    // Viewport Pipeline Parameters
    Camera3D camera = { Vector3{ 0.0f, 15.0f, -45.0f }, Vector3{ 0.0f, 0.0f, 0.0f }, Vector3{ 0.0f, 1.0f, 0.0f }, 45.0f, CAMERA_PERSPECTIVE };

    // Strict Memory Boundaries: Total allocation for 150k passive particles and 250 adaptive cruisers
    constexpr uint64_t totalAsteroids = 150000;
    constexpr uint64_t totalShips = 250;
    constexpr uint64_t totalAgentCapacity = totalAsteroids + totalShips + 4; // Pads slots for the system nodes

    stellar_agents::EnvironmentMatrix stateMatrix(totalAgentCapacity);
    stellar_agents::PhysicsEvolution  evolutionEngine;
    stellar_agents::RenderCore        graphicsEngine;

    // Fetch the active write allocation block for cold startup seeding
    auto& initBuffer = stateMatrix.get_write_buffer();
    std::mt19937 rand(1337);
    std::uniform_real_distribution<float> rad(0.0f, 2.0f * PI);
    std::uniform_real_distribution<float> rAst(800.0f, 2400.0f);
    std::uniform_real_distribution<float> hPl(-25.0f, 25.0f);
    std::uniform_real_distribution<float> rSh(300.0f, 600.0f);

    uint64_t idx = 0;

    // ========================================================================
    // DATA SEEDING: ANONYMIZED SYSTEM ARCHETYPE CONFIGURATION
    // Pulling physical parameters directly from the compile-time register
    // ========================================================================

    // NODE 0: Central Stationary Black Hole Attractor
    initBuffer[idx++] = stellar_agents::AgentState{
        .position = { 0.0f, 0.0f, 0.0f },
        .velocity = { 0.0f, 0.0f, 0.0f },
        .color = BLACK,
        .agent_id = static_cast<uint32_t>(idx - 1),
        .type = stellar_agents::AgentType::ATTRACTOR
    };

    // NODE 1: Inner Diamond Core Planet (Dynamic Reference Path Location)
    initBuffer[idx++] = stellar_agents::AgentState{
        .position = { 650.0f, 0.0f, 0.0f },
        .velocity = { 0.0f, 0.0f, 0.0f },
        .color = GREEN,
        .agent_id = static_cast<uint32_t>(idx - 1),
        .type = stellar_agents::AgentType::ATTRACTOR
    };

    // NODE 2: Outer Gas Giant Planet (Dynamic Reference Path Location)
    initBuffer[idx++] = stellar_agents::AgentState{
        .position = { -1414.2f, 0.0f, 1414.2f },
        .velocity = { 0.0f, 0.0f, 0.0f },
        .color = PURPLE,
        .agent_id = static_cast<uint32_t>(idx - 1),
        .type = stellar_agents::AgentType::ATTRACTOR
    };

    // NODE 3: Adaptive Spawner Gateway Node
    initBuffer[idx++] = stellar_agents::AgentState{
        .position = { 1767.7f, 0.0f, -1767.7f },
        .velocity = { 0.0f, 0.0f, 0.0f },
        .color = RED,
        .agent_id = static_cast<uint32_t>(idx - 1),
        .type = stellar_agents::AgentType::ATTRACTOR
    };

    // Seed Massless Passive Fragment Swarms (Asteroids Field)
    for (uint64_t i = 0; i < totalAsteroids; ++i) {
        float a = rad(rand);
        float r = rAst(rand);
        Vector3 p = { r * std::cos(a), hPl(rand), r * std::sin(a) };

        // Velocity magnitude calibrated purely against the pre-scaled central black hole runtime mass
        float vMag = std::sqrt(config::physics::black_hole_runtime_mass / r) * 5.5f;

        initBuffer[idx++] = stellar_agents::AgentState{
            p,
            Vector3{ -vMag * std::sin(a), 0.0f, vMag * std::cos(a) },
            ColorFromHSV(a * (360.0f / (2.0f * PI)), 0.65f, 0.85f),
            static_cast<uint32_t>(idx - 1),
            stellar_agents::AgentType::PASSIVE
        };
    }

    // Seed Homeostatic Adaptive Flotilla (UEE Capital Vessels)
    for (uint64_t i = 0; i < totalShips; ++i) {
        float a = rad(rand);
        float r = rSh(rand);
        Vector3 p = { r * std::cos(a), hPl(rand) * 0.2f, r * std::sin(a) };

        float vMag = std::sqrt(config::physics::black_hole_runtime_mass / r) * 4.2f;

        initBuffer[idx++] = stellar_agents::AgentState{
            p,
            Vector3{ -vMag * std::sin(a), 0.0f, vMag * std::cos(a) },
            (i % 3 == 1) ? ORANGE : SKYBLUE,
            static_cast<uint32_t>(idx - 1),
            stellar_agents::AgentType::ADAPTIVE
        };
    }

    // Lock memory configuration and execute the initial hardware allocation flip
    stateMatrix.FlipBuffers();

    float timeScale = 1.0f;
    float pilotVelocityCache = 0.0f;
    DisableCursor();

    // ============================================================================
    // PRINCIPAL CHRONOLOGICAL EXECUTION ENGINE LOOP
    // ============================================================================
    while (!WindowShouldClose()) {
        const float dt = GetFrameTime();
        Vector3 oldCamPos = camera.position;
        UpdateCamera(&camera, CAMERA_FREE);

        if (dt > 0.0f) [[likely]] {
            pilotVelocityCache = Vector3Length(Vector3Subtract(camera.position, oldCamPos)) / dt;
        }

        // Chrono Warp Scale Interceptors
        if (IsKeyPressed(KEY_THREE)) timeScale = std::max(0.1f, timeScale - 0.1f);
        if (IsKeyPressed(KEY_FOUR))  timeScale = std::min(2.0f, timeScale + 0.1f);

        // Fire background multi-core processing jthreads block-free
        evolutionEngine.execute_asynchronous_tick(dt * timeScale, stateMatrix);

        // Render pass dispatch execution mapping
        BeginDrawing();
        ClearBackground(BLACK);
        graphicsEngine.draw_composite_scene(camera, stateMatrix);

        // --- COMPACT HIGH-PERFORMANCE SCI-FI HUD ---
        const int32_t w = GetScreenWidth();
        const int32_t h = GetScreenHeight();
        const int32_t rX = w - 400;
        const Color cyan = Color{ 0, 210, 255, 140 };
        const Color alert = Color{ 255, 140, 0, 230 };

        // Static Corner Brackets
        DrawLine(25, 25, 75, 25, cyan);   DrawLine(25, 25, 25, 75, cyan);
        DrawLine(w - 25, 25, w - 75, 25, cyan); DrawLine(w - 25, 25, w - 25, 75, cyan);
        DrawLine(25, h - 25, 75, h - 25, cyan); DrawLine(25, h - 25, 25, h - 75, cyan);
        DrawLine(w - 25, h - 25, w - 75, h - 25, cyan); DrawLine(w - 25, h - 25, w - 25, h - 75, cyan);

        // Telemetry Panel Display Pass
        DrawRectangleLines(40, 40, 360, 200, cyan);
        DrawRectangle(40, 40, 360, 30, ColorAlpha(cyan, 0.15f));
        DrawText("PILOT TACTICAL TELEMETRY", 55, 47, 15, cyan);
        DrawText(TextFormat("VECTOR POS X : %8.2f", camera.position.x), 55, 85, 14, RAYWHITE);
        DrawText(TextFormat("VECTOR POS Y : %8.2f", camera.position.y), 55, 110, 14, RAYWHITE);
        DrawText(TextFormat("VECTOR POS Z : %8.2f", camera.position.z), 55, 135, 14, RAYWHITE);
        DrawText(TextFormat("FLIGHT SPEED : %8.1f m/s", pilotVelocityCache), 55, 170, 15, alert);

        // Performance Profiler Metrics Display Pass
        DrawRectangleLines(rX, 40, 360, 200, cyan);
        DrawRectangle(rX, 40, 360, 30, ColorAlpha(cyan, 0.15f));
        DrawText("CADS MULTI-CORE PERFORMANCE", rX + 15, 47, 15, cyan);
        DrawText(TextFormat("FRAME RESOLUTION : %d FPS", GetFPS()), rX + 20, 85, 14, GREEN);
        DrawText(TextFormat("TICK TIME DELTA  : %.4f ms", dt * 1000.0f), rX + 20, 110, 14, RAYWHITE);
        DrawText(TextFormat("CHRONO WARP SCALE: %.1fx", timeScale), rX + 20, 135, 14, GOLD);
        DrawText(TextFormat("ACTIVE CADS ENSEMBLE: %llu", stateMatrix.get_active_count()), rX + 20, 175, 14, cyan);

        EndDrawing();
    }

    // Context Deconstruction
    EnableCursor();
    CloseWindow();
    return 0;
}
