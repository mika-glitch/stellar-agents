#include "raylib.h"
#include "black_hole.h"
#include "universe.h"

int main() {
    // 1. WINDOW INFRASTRUCTURE SETUP
    // Using explicit resolution constants rather than hardcoded magic numbers
    constexpr int windowWidth = 1920;
    constexpr int windowHeight = 1080;
    InitWindow(windowWidth, windowHeight, "Stellar Agents - Tamsa Sandbox Engine v3.0");

    // CIG Profiling Pass: Toggle between SetTargetFPS(60) or SetTargetFPS(0) 
    // to audit raw background execution capabilities of the newly optimized worker pool.
    SetTargetFPS(60);

    // 2. CAM MATRIX INITIALIZATION
    // Dynamic camera initialized close to capture the Einstein Ring setup
    Camera3D camera = { 0 };
    camera.position = Vector3{ 0.0f, 5.0f, -10.0f };
    camera.target = Vector3{ 0.0f, 0.0f, 0.0f };
    camera.up = Vector3{ 0.0f, 1.0f, 0.0f };
    camera.fovy = 45.0f;
    camera.projection = CAMERA_PERSPECTIVE;

    // 3. CORE COMPONENT RESOURCE RESERVATIONS
    // Stack-allocated lifecycle ordering ensures that the 'tamsaSystem' thread pool 
    // winds down safely via its destructor BEFORE 'tamsaKharesh' memory is unallocated.
    BlackHole tamsaKharesh(Vector3{ 0.0f, 0.0f, 0.0f }, 600.0f, 1.5f);
    Universe tamsaSystem(tamsaKharesh, 1500000);

    DisableCursor();

    // 4. MAIN ASYNCHRONOUS UPDATE LOOP
    while (!WindowShouldClose()) {
        const float dt = GetFrameTime();

        UpdateCamera(&camera, CAMERA_FREE);

        // Core interactive regulator tracking via keys '1' and '2'
        if (IsKeyDown(KEY_ONE)) tamsaSystem.IncreaseParticleLoad();
        if (IsKeyDown(KEY_TWO)) tamsaSystem.DecreaseParticleLoad();

        // Dispatch parallel updates down the sub-system matrix pipeline
        tamsaSystem.Update(dt);
        tamsaKharesh.Update(dt);

        // 5. LOW-LEVEL RENDER DISPATCH PIPELINE
        BeginDrawing();
        ClearBackground(BLACK);

        // Layer 1: Run uncompromised GPU Raymarching pipeline
        tamsaKharesh.Draw(camera);

        // Layer 2: Render asynchronous CPU particle cluster mechanics
        tamsaSystem.Draw3D(camera);

        // Layer 3: HUD Instruments & UI Telemetry
        DrawText("TAMSA INTERACTIVE COCKPIT - FLIGHT CORE v3.0", 10, 10, 22, ORANGE);
        DrawText("FLIGHT SYSTEMS : [W][A][S][D] to Fly Forward/Left/Backward/Right", 10, 42, 16, RAYWHITE);
        DrawText("THRUSTER ENGINE: [SPACE] to Ascend Vertically | [L-CTRL] to Descend Vertically", 10, 62, 16, RAYWHITE);
        DrawText("HUD NAVIGATION : [MOUSE] to Look Around | [ESC] to Disengage Simulation", 10, 82, 16, RAYWHITE);
        DrawText("ENGINE REGULATOR: Press [1] to Add Asteroids | [2] to Remove Asteroids", 10, 102, 16, SKYBLUE);

        // Draw interactive slider visualization using standardized explicit float conversions
        DrawRectangle(10, 132, 400, 18, DARKGRAY);
        const float activeParticles = static_cast<float>(tamsaSystem.GetActiveParticleCount());
        const float maxParticles = static_cast<float>(tamsaSystem.GetMaxParticleCount());
        const float sliderWidth = (activeParticles / maxParticles) * 400.0f;

        DrawRectangle(10, 132, static_cast<int>(sliderWidth), 18, GREEN);
        DrawRectangleLines(10, 132, 400, 18, WHITE);

        // Telemetry diagnostics projection text
        DrawText(TextFormat("Particle Load: %d / %d Survivors (%d Requested)",
            tamsaSystem.GetLivingParticleCount(),
            tamsaSystem.GetMaxParticleCount(),
            tamsaSystem.GetActiveParticleCount()), 425, 132, 16, GREEN);

        DrawText(TextFormat("Threads: %u Parallel CPU Workers Active", tamsaSystem.GetThreadCount()), 10, 160, 16, SKYBLUE);
        DrawFPS(10, 185);

        // Draw real-time planetary target trackers over the frame
        tamsaSystem.DrawHUD(camera);

        EndDrawing();
    }

    // 6. ORDERLY SYSTEM UNWIND
    EnableCursor();
    CloseWindow();
    return 0;
}
