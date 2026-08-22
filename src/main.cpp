#include "raylib.h"
#include "black_hole.h"
#include "universe.h"
#include "fleet_system.h"
#include <algorithm>
#include <cstdint>

// Explicitly mapping code layers into our secure high-performance sandbox namespace
using namespace godot_hpc;

int main() {
    // ============================================================================
    // 1. HARDWARE SUB-SYSTEM & WINDOW INITIALIZATION
    // Enforcing explicit constants to suppress magical number register injections.
    // ============================================================================
    constexpr int32_t windowWidth = 1920;
    constexpr int32_t windowHeight = 1080;
    InitWindow(windowWidth, windowHeight, "Stellar Agents - Tamsa Sandbox Engine v3.0");

    // Host Telemetry Pass: Capped at solid 60Hz. Can be set to 0 to unthrottle 
    // the asynchronous multi-threaded worker pools for raw execution diagnostics.
    SetTargetFPS(60);

    // ============================================================================
    // 2. RENDERING MATRIX & CAMERA CONFIGURATION
    // Standard perspective matrix aligned close to the gravitational singularity center.
    // ============================================================================
    Camera3D camera = { 0 };
    camera.position = Vector3{ 0.0f, 5.0f, -10.0f };
    camera.target = Vector3{ 0.0f, 0.0f, 0.0f };
    camera.up = Vector3{ 0.0f, 1.0f, 0.0f };
    camera.fovy = 45.0f;
    camera.projection = CAMERA_PERSPECTIVE;

    // ============================================================================
    // 3. DETERMINISTIC REEP-ALLOCATION RESSOURCING (RAII PATTERN)
    // Ordered sequentially on the stack frame so the 'tamsaSystem' worker threads
    // terminate gracefully via destructors BEFORE 'tamsaKharesh' memory is freed.
    // ============================================================================
    BlackHole tamsaKharesh(Vector3{ 0.0f, 0.0f, 0.0f }, 600.0f, 1.5f);
    Universe  tamsaSystem(1500000, 2400.0f); // Re-aligned initialization matching unified DoD footprint
    FleetSystem fleetSystem(tamsaKharesh, tamsaSystem);

    // Simulation delta throttling registers
    float simulationSpeed = 1.0f;
    constexpr float minSpeed = 0.1f;
    constexpr float maxSpeed = 2.0f;
    constexpr float speedStep = 0.1f;

    DisableCursor();

    // ============================================================================
    // 4. MAIN ASYNCHRONOUS ENGINE SIMULATION DECK
    // Independent host tick loop processing environment updates concurrently.
    // ============================================================================
    while (!WindowShouldClose()) {
        const float baseDt = GetFrameTime();

        // Flush hardware input updates to the active matrix controller
        UpdateCamera(&camera, CAMERA_FREE);

        // Conditional task pooling inputs managing particle payload boundaries
        if (IsKeyDown(KEY_ONE)) tamsaSystem.IncreaseParticleLoad();
        if (IsKeyDown(KEY_TWO)) tamsaSystem.DecreaseParticleLoad();

        // Scale chronological parameters safely using standardized math templates
        if (IsKeyPressed(KEY_THREE)) simulationSpeed = std::max(minSpeed, simulationSpeed - speedStep);
        if (IsKeyPressed(KEY_FOUR))  simulationSpeed = std::min(maxSpeed, simulationSpeed + speedStep);

        // Calculate the speed-scaled dynamic frame interval step
        const float dt = baseDt * simulationSpeed;

        // Dispatch frame updates down the decoupled sub-system matrix pipeline
        tamsaSystem.update_asynchronous_physics(dt, tamsaKharesh); // Synchronized multi-threaded physics pass
        fleetSystem.Update(dt);
        tamsaKharesh.Update(dt);

        // ============================================================================
        // 5. LOW-LEVEL RENDER DISPATCH PIPELINE
        // Sequential hardware blit queues streaming spatial data structures straight to VRAM.
        // ============================================================================
        BeginDrawing();
        ClearBackground(BLACK);

        // Pass 1: Execute uncompromised GPU Raymarching pipeline (The Einstein Ring layer)
        tamsaKharesh.Draw(camera);

        // Pass 2: Blit asynchronous CPU particle cluster arrays (Contiguous buffer streaming)
        tamsaSystem.render_hardware_vertex_buffers(camera.position); // Zero-copy line blit pass

        // Pass 3: Render physical ship hulls inside lensed spacetime coordinates
        fleetSystem.Draw3D(camera);

        // Pass 4: Low-overhead UI telemetry HUD instrumentation projection
        DrawText("TAMSA INTERACTIVE COCKPIT - FLIGHT CORE v3.0", 10, 10, 22, ORANGE);
        DrawText("FLIGHT SYSTEMS : [W][A][S][D] to Fly Forward/Left/Backward/Right", 10, 42, 16, RAYWHITE);
        DrawText("THRUSTER ENGINE: [SPACE] to Ascend Vertically | [L-CTRL] to Descend Vertically", 10, 62, 16, RAYWHITE);
        DrawText("HUD NAVIGATION : [MOUSE] to Look Around | [ESC] to Disengage Simulation", 10, 82, 16, RAYWHITE);
        DrawText("ENGINE REGULATOR: Press [1] to Add Asteroids | [2] to Remove Asteroids", 10, 102, 16, SKYBLUE);
        DrawText(TextFormat("CHRONO CORE    : Speed: %.1fx | [3] Slower | [4] Faster (0.1x - 2.0x)", simulationSpeed), 10, 122, 16, GOLD);

        // Draw interactive load matrix visualization using clean float conversions
        DrawRectangle(10, 152, 400, 18, DARKGRAY);
        const float activeParticles = static_cast<float>(tamsaSystem.get_active_survivors());
        const float maxParticles = 1500000.0f; // Synchronized maximum payload tracker
        const float sliderWidth = (activeParticles / maxParticles) * 400.0f;

        DrawRectangle(10, 152, static_cast<int32_t>(sliderWidth), 18, GREEN);
        DrawRectangleLines(10, 152, 400, 18, WHITE);

        // Output real-time subsystem diagnostics to the text screen
        DrawText(TextFormat("Particle Load: %llu / 1500000 Survivors Active",
            tamsaSystem.get_active_survivors()), 425, 152, 16, GREEN);

        DrawText("Threads: Asynchronous C++20 Thread-Pool Active", 10, 180, 16, SKYBLUE);
        DrawFPS(10, 205);

        // Render target tracking rings over the 2D frame buffer viewport bounds
        tamsaSystem.DrawHUD(camera);
        fleetSystem.DrawHUD(camera);

        EndDrawing();
    }

    // ============================================================================
    // 6. ORDERLY SYSTEM UNWIND
    // Clear display contexts and deallocate host OS interface registers safely.
    // ============================================================================
    EnableCursor();
    CloseWindow();
    return 0;
}
