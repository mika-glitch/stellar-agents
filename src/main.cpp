#include "raylib.h"
#include "black_hole.h"
#include "universe.h"

int main() {
    InitWindow(1920, 1080, "Stellar Agents - Tamsa Sandbox Engine v3.0");
    SetTargetFPS(60);

    // Dynamic camera initialized close to capture the Einstein Ring setup
    Camera3D camera = { 0 };
    camera.position = Vector3{ 0.0f, 5.0f, -10.0f };
    camera.target = Vector3{ 0.0f, 0.0f, 0.0f };
    camera.up = Vector3{ 0.0f, 1.0f, 0.0f };
    camera.fovy = 45.0f;
    camera.projection = CAMERA_PERSPECTIVE;

    // Instantiate engine components
    BlackHole tamsaKharesh(Vector3{ 0.0f, 0.0f, 0.0f }, 600.0f, 1.5f);
    Universe tamsaSystem(tamsaKharesh, 150000);

    DisableCursor();

    while (!WindowShouldClose()) {
        float dt = GetFrameTime();

        UpdateCamera(&camera, CAMERA_FREE);

        // Core interactive regulator tracking via keys '1' and '2'
        if (IsKeyDown(KEY_ONE)) tamsaSystem.IncreaseParticleLoad();
        if (IsKeyDown(KEY_TWO)) tamsaSystem.DecreaseParticleLoad();

        // System matrix updates
        tamsaSystem.Update(dt);
        tamsaKharesh.Update(dt);

        BeginDrawing();
        ClearBackground(BLACK);

        // Layer 1: Run your uncompromised GPU Raymarching pipeline
        tamsaKharesh.Draw(camera);

        // Layer 2: Render asynchronous CPU particle cluster mechanics
        tamsaSystem.Draw3D(camera);

        // Layer 3: HUD Instruments & UI Telemetry
        DrawText("TAMSA INTERACTIVE COCKPIT - FLIGHT CORE v3.0", 10, 10, 22, ORANGE);
        DrawText("FLIGHT SYSTEMS : [W][A][S][D] to Fly Forward/Left/Backward/Right", 10, 42, 16, RAYWHITE);
        DrawText("THRUSTER ENGINE: [SPACE] to Ascend Vertically | [L-CTRL] to Descend Vertically", 10, 62, 16, RAYWHITE);
        DrawText("HUD NAVIGATION : [MOUSE] to Look Around | [ESC] to Disengage Simulation", 10, 82, 16, RAYWHITE);
        DrawText("ENGINE REGULATOR: Press [1] to Add Asteroids | [2] to Remove Asteroids", 10, 102, 16, SKYBLUE);

        // Draw interactive slider visualization
        DrawRectangle(10, 132, 400, 18, DARKGRAY);
        float sliderWidth = ((float)tamsaSystem.GetActiveParticleCount() / (float)tamsaSystem.GetMaxParticleCount()) * 400.0f;
        DrawRectangle(10, 132, (int)sliderWidth, 18, GREEN);
        DrawRectangleLines(10, 132, 400, 18, WHITE);

        DrawText(TextFormat("Particle Load: %d / %d Survivors (%d Requested)",
            tamsaSystem.GetLivingParticleCount(),
            tamsaSystem.GetMaxParticleCount(),
            tamsaSystem.GetActiveParticleCount()), 425, 132, 16, GREEN);        
        DrawText(TextFormat("Threads: %d Parallel CPU Workers Active", tamsaSystem.GetThreadCount()), 10, 160, 16, SKYBLUE);
        DrawFPS(10, 185);

        // Draw real-time planetary target trackers over the frame
        tamsaSystem.DrawHUD(camera);

        EndDrawing();
    }

    EnableCursor();
    CloseWindow();
    return 0;
}