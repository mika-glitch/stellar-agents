#include "raylib.h"
#include "raymath.h"
#include "agent_state.h"
#include "environment_matrix.h"
#include "physics_evolution.h"
#include "render_core.h"
#include <random>
#include <cstdint>
#include <algorithm>

int main() {
    SetConfigFlags(FLAG_MSAA_4X_HINT);
    InitWindow(1920, 1080, "Stellar Agents - Autonomous CADS Sandbox Engine v4.0");
    SetTargetFPS(60);

    Camera3D camera = { Vector3{ 0.0f, 15.0f, -45.0f }, Vector3{ 0.0f, 0.0f, 0.0f }, Vector3{ 0.0f, 1.0f, 0.0f }, 45.0f, CAMERA_PERSPECTIVE };

    constexpr uint64_t totalAsteroids = 1500000, totalShips = 250, totalAgentCapacity = totalAsteroids + totalShips + 1;
    stellar_agents::EnvironmentMatrix stateMatrix(totalAgentCapacity);
    stellar_agents::PhysicsEvolution  evolutionEngine;
    stellar_agents::RenderCore        graphicsEngine;

    auto& initBuffer = stateMatrix.get_write_buffer();
    std::mt19937 rand(1337);
    std::uniform_real_distribution<float> rad(0.0f, 2.0f * PI), rAst(800.0f, 2400.0f), hPl(-25.0f, 25.0f), rSh(300.0f, 600.0f);

    uint64_t idx = 0;
    initBuffer[idx++] = stellar_agents::AgentState{ .position = {0,0,0}, .velocity = {0,0,0}, .color = BLACK, .agent_id = 0, .type = stellar_agents::AgentType::ATTRACTOR };

    for (uint64_t i = 0; i < totalAsteroids; ++i) {
        float a = rad(rand), r = rAst(rand);
        Vector3 p = { r * std::cos(a), hPl(rand), r * std::sin(a) };
        float vMag = std::sqrt((600.0f * 0.0015f) / r) * 140.0f;
        initBuffer[idx++] = stellar_agents::AgentState{ p, Vector3{ -vMag * std::sin(a), 0.0f, vMag * std::cos(a) }, ColorFromHSV(a * (360.0f / (2.0f * PI)), 0.65f, 0.85f), static_cast<uint32_t>(idx), stellar_agents::AgentType::PASSIVE };
    }
    for (uint64_t i = 0; i < totalShips; ++i) {
        float a = rad(rand), r = rSh(rand);
        Vector3 p = { r * std::cos(a), hPl(rand) * 0.2f, r * std::sin(a) };
        float vMag = std::sqrt((600.0f * 0.0015f) / r) * 110.0f;
        initBuffer[idx++] = stellar_agents::AgentState{ p, Vector3{ -vMag * std::sin(a), 0.0f, vMag * std::cos(a) }, (i % 3 == 1) ? ORANGE : SKYBLUE, static_cast<uint32_t>(idx), stellar_agents::AgentType::ADAPTIVE };
    }

    stateMatrix.swap_evolutionary_buffers();
    float timeScale = 1.0f;
    float pilotVelocityCache = 0.0f;
    DisableCursor();

    while (!WindowShouldClose()) {
        const float dt = GetFrameTime();
        Vector3 oldCamPos = camera.position;
        UpdateCamera(&camera, CAMERA_FREE);

        // FIXED: Renamed 'oldCameraPosition' to 'oldCamPos' to match the declaration above!
        if (dt > 0.0f) [[likely]] pilotVelocityCache = Vector3Length(Vector3Subtract(camera.position, oldCamPos)) / dt;

        if (IsKeyPressed(KEY_THREE)) timeScale = std::max(0.1f, timeScale - 0.1f);
        if (IsKeyPressed(KEY_FOUR))  timeScale = std::min(2.0f, timeScale + 0.1f);

        evolutionEngine.execute_asynchronous_tick(dt * timeScale, stateMatrix);

        BeginDrawing();
        ClearBackground(BLACK);
        graphicsEngine.draw_composite_scene(camera, stateMatrix);

        // --- COMPACT HIGH-PERFORMANCE SCI-FI HUD ---
        const int32_t w = GetScreenWidth(), h = GetScreenHeight(), rX = w - 400;
        const Color cyan = Color{ 0, 210, 255, 140 }, alert = Color{ 255, 140, 0, 230 };

        // Corner Brackets
        DrawLine(25, 25, 75, 25, cyan);   DrawLine(25, 25, 25, 75, cyan);
        DrawLine(w - 25, 25, w - 75, 25, cyan); DrawLine(w - 25, 25, w - 25, 75, cyan);
        DrawLine(25, h - 25, 75, h - 25, cyan); DrawLine(25, h - 25, 25, h - 75, cyan);
        DrawLine(w - 25, h - 25, w - 75, h - 25, cyan); DrawLine(w - 25, h - 25, w - 25, h - 75, cyan);

        // Telemetry Panel
        DrawRectangleLines(40, 40, 360, 200, cyan);
        DrawRectangle(40, 40, 360, 30, ColorAlpha(cyan, 0.15f));
        DrawText("UEE PILOT TACTICAL TELEMETRY", 55, 47, 15, cyan);
        DrawText(TextFormat("VECTOR POS X : %8.2f", camera.position.x), 55, 85, 14, RAYWHITE);
        DrawText(TextFormat("VECTOR POS Y : %8.2f", camera.position.y), 55, 110, 14, RAYWHITE);
        DrawText(TextFormat("VECTOR POS Z : %8.2f", camera.position.z), 55, 135, 14, RAYWHITE);
        DrawText(TextFormat("FLIGHT SPEED : %8.1f m/s", pilotVelocityCache), 55, 170, 15, alert);

        // Performance Panel
        DrawRectangleLines(rX, 40, 360, 200, cyan);
        DrawRectangle(rX, 40, 360, 30, ColorAlpha(cyan, 0.15f));
        DrawText("CADS MULTI-CORE PERFORMANCE", rX + 15, 47, 15, cyan);
        DrawText(TextFormat("FRAME RESOLUTION : %d FPS", GetFPS()), rX + 20, 85, 14, GREEN);
        DrawText(TextFormat("TICK TIME DELTA  : %.4f ms", dt * 1000.0f), rX + 20, 110, 14, RAYWHITE);
        DrawText(TextFormat("CHRONO WARP SCALE: %.1fx", timeScale), rX + 20, 135, 14, GOLD);
        DrawText(TextFormat("ACTIVE CADS ENSEMBLE: %llu", stateMatrix.count_active_agents()), rX + 20, 175, 14, cyan);

        EndDrawing();
    }
    EnableCursor(); CloseWindow(); return 0;
}