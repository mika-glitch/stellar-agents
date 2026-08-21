#include "raylib.h"
#include "black_hole.h"
#include "relativity.h" 
#include "rlgl.h" 
#include "raymath.h"
#include <vector>
#include <cstdlib>

// 1. DATA LAYOUT FOR RELATIVISTIC ASTEROID PARTICLES
struct Asteroid {
    Vector3 position;
    Vector3 velocity;
    Vector3 size;       // 3D dimensions for asymmetric, jagged debris shape
    float rotation;     // Individual rotation angle
    float rotSpeed;     // Spinning velocity
    Color color;
    bool active;
};

int main() {
    // Initialize window environment (Full-HD 1080p setup)
    InitWindow(1920, 1080, "Stellar Agents - Tamsa Gravity Sandbox");
    SetTargetFPS(60);

    // Set up the 3D viewing camera
    Camera3D camera = { 0 };
    camera.position = Vector3{ 0.0f, 5.0f, -10.0f }; // High orbital starting position
    camera.target = Vector3{ 0.0f, 0.0f, 0.0f };     // Fixed focus on the singularity
    camera.up = Vector3{ 0.0f, 1.0f, 0.0f };
    camera.fovy = 45.0f;
    camera.projection = CAMERA_PERSPECTIVE;

    // Instantiate the central Schwarzschild Black Hole core
    BlackHole tamsaKharesh(Vector3{ 0.0f, 0.0f, 0.0f }, 500.0f, 1.5f);

    // Inject 250 high-density debris fragments into the system
    const int numAsteroids = 250;
    std::vector<Asteroid> belt;

    for (int i = 0; i < numAsteroids; i++) {
        // Define orbit distribution limits (Between 4.5 and 15.0 units distance)
        float radius = 4.5f + (float)(rand() % 105) * 0.1f;
        float angle = (float)(rand() % 360) * DEG2RAD;

        // Flatten vertical dispersion for a realistic thin accretion layer profile
        Vector3 pos = { sinf(angle) * radius, ((float)(rand() % 20) - 10.0f) * 0.05f, cosf(angle) * radius };

        // Calculate stable orbital velocity profile: v = sqrt(M / r)
        float speed = sqrtf(tamsaKharesh.GetMass() / radius);

        // INTERSTELLAR STRIKE PROTOCOL:
        // Every third asteroid (i % 3 == 0) is deliberately slowed down to simulate a decay spiral!
        if (i % 3 == 0) {
            speed *= 0.45f; // Drastic kinetic energy loss forces terminal inward collapse
        }

        Vector3 vel = { -cosf(angle) * speed, 0.0f, sinf(angle) * speed };
        vel.y += ((float)(rand() % 20) - 10.0f) * 0.05f;

        // Generate irregular geometric scales to avoid perfect spheres
        float baseScale = 0.03f + (float)(rand() % 10) * 0.008f;
        Vector3 size = {
            baseScale * (0.7f + (float)(rand() % 6) * 0.1f),
            baseScale * (0.7f + (float)(rand() % 6) * 0.1f),
            baseScale * (0.7f + (float)(rand() % 6) * 0.1f)
        };

        // Assign albedo reflections ranging from pitch black to ice-blue glare
        int type = rand() % 4;
        Color astColor = GRAY;
        if (type == 0) astColor = LIGHTGRAY;
        if (type == 1) astColor = DARKGRAY;
        if (type == 2) astColor = Color{ 180, 210, 255, 255 }; // Reacts to ice-blue accretion glow

        float rot = (float)(rand() % 360);
        float rSpeed = ((float)(rand() % 40) - 20.0f) * 0.5f;

        belt.push_back(Asteroid{ pos, vel, size, rot, rSpeed, astColor, true });
    }

    // Capture mouse cursor for unconstrained sandbox flight navigation
    DisableCursor();

    // Main engine simulation loop
    while (!WindowShouldClose()) {
        float dt = GetFrameTime();

        // Process standard camera flight controls (WASD navigation mechanics)
        UpdateCamera(&camera, CAMERA_FREE);

        // NATIVE CPU CORE PHYSICS SECTOR
        for (auto& ast : belt) {
            if (!ast.active) continue;

            // SCHWARZSCHILD RADIAL BOUNDARY CHECK:
            // Terminate object rendering instantly if it penetrates the point of no return
            if (tamsaKharesh.HasCrossedPointOfNoReturn(ast.position)) {
                ast.active = false; // permanent mass absorption event
                continue;
            }

            // Retrieve Newtonian acceleration forces from the singularity core
            Vector3 gravityAcceleration = tamsaKharesh.CalculateGravity(ast.position);

            // Execute numerical vector integration routines (Euler-Cromer model)
            ast.velocity = Vector3Add(ast.velocity, Vector3Scale(gravityAcceleration, dt));
            ast.position = Vector3Add(ast.position, Vector3Scale(ast.velocity, dt));
            ast.rotation += ast.rotSpeed * dt; // Advance mesh rotation state
        }

        tamsaKharesh.Update(dt);

        // RENDER PIPELINE EXECUTION
        BeginDrawing();
        ClearBackground(BLACK);

        // STEP 1: Render the full-screen GPU raymarching shader backdrop first
        tamsaKharesh.Draw(camera);

        // STEP 2: Render 3D scene elements into the camera matrix space overlay
        // Disable depth buffer verification temporarily to guarantee visibility over billboard plane
        rlDisableDepthTest();
        BeginMode3D(camera);
        for (const auto& ast : belt) {
            if (ast.active) {
                // Apply CPU-side Einsteinian light bending transformation matrix
                Vector3 apparentPos = Relativity::GetApparentPosition(
                    ast.position,
                    camera.position,
                    tamsaKharesh.GetPosition(),
                    tamsaKharesh.GetMass(),
                    tamsaKharesh.GetEventHorizon()
                );

                // Render asymmetric wireframes and solid bodies at transformed positions
                DrawCubeWires(apparentPos, ast.size.x, ast.size.y, ast.size.z, ast.color);
                DrawCube(apparentPos, ast.size.x * 0.8f, ast.size.y * 0.8f, ast.size.z * 0.8f, ast.color);
            }
        }
        EndMode3D();
        rlEnableDepthTest(); // Re-enable regular hardware depth buffer mechanics

        // Draw flight deck telemetry instrumentation
        DrawText("TAMSA GRAVITY SANDBOX: HIGH DENSITY BELT ACTIVE", 10, 10, 20, SKYBLUE);
        DrawText("Controls: WASD to Fly | Mouse to Look | ESC to exit", 10, 40, 16, RAYWHITE);
        DrawFPS(10, 70);

        EndDrawing();
    }

    // Release mouse pointer safely upon exit state trigger
    EnableCursor();
    CloseWindow();
    return 0;
}