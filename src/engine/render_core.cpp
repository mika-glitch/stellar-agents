#include "render_core.h"
#include "raymath.h"
#include "engine_config.h"
#include <vector>

namespace stellar_agents {

    // ============================================================================
    // LOW-LEVEL GRAPHICS INFRASTRUCTURE COUPLING PASS
    // SIMD-aligned shader and descriptor initialization framework.
    // ============================================================================
    RenderCore::RenderCore() noexcept {
        // Vulkan / DX12 API Abstraction Mapping: Loads precompiled shader bytecode
        unified_agent_shader = LoadShader("src/shaders/lensing.vert", "src/shaders/accretion.frag");

        // Uniform descriptor location caches for baseline system telemetry mapping
        uniform_time = GetShaderLocation(unified_agent_shader, "time");
        uniform_resolution = GetShaderLocation(unified_agent_shader, "resolution");
        uniform_cam_pos = GetShaderLocation(unified_agent_shader, "camPos");
        uniform_cam_target = GetShaderLocation(unified_agent_shader, "camTarget");

        uniform_hole_pos = GetShaderLocation(unified_agent_shader, "holePos");
        uniform_hole_mass = GetShaderLocation(unified_agent_shader, "mass");
        uniform_event_horizon = GetShaderLocation(unified_agent_shader, "eventHorizon");
    }

    RenderCore::~RenderCore() {
        UnloadShader(unified_agent_shader);
    }

    // ============================================================================
    // HIGH-PERFORMANCE LOW-LEVEL MULTI-AGENT COMPOSITE RENDER PASS
    // Thread-safe contiguous array packaging minimizing the CPU-to-GPU data overhead.
    // ============================================================================
    void RenderCore::draw_composite_scene(const Camera3D& p_camera, const EnvironmentMatrix& p_matrix) const noexcept {
        const float w = static_cast<float>(GetScreenWidth());
        const float h = static_cast<float>(GetScreenHeight());
        const Vector2 render_res = { w, h };
        const float current_time = static_cast<float>(GetTime());

        const Vector3 cached_c_pos = p_camera.position;
        const Vector3 cached_c_target = p_camera.target;

        constexpr Vector3 singularity_center = { 0.0f, 0.0f, 0.0f };
        const float attractor_mass = config::physics::black_hole_runtime_mass;
        const float horizon_radius = config::physics::rs_horizon;

        // --- STAGE 1: PACK HARDWARE UNIFORM REGISTERS ---
        SetShaderValue(unified_agent_shader, uniform_time, &current_time, SHADER_UNIFORM_FLOAT);
        SetShaderValue(unified_agent_shader, uniform_resolution, &render_res, SHADER_UNIFORM_VEC2);
        SetShaderValue(unified_agent_shader, uniform_cam_pos, &cached_c_pos, SHADER_UNIFORM_VEC3);
        SetShaderValue(unified_agent_shader, uniform_cam_target, &cached_c_target, SHADER_UNIFORM_VEC3);
        SetShaderValue(unified_agent_shader, uniform_hole_pos, &singularity_center, SHADER_UNIFORM_VEC3);
        SetShaderValue(unified_agent_shader, uniform_hole_mass, &attractor_mass, SHADER_UNIFORM_FLOAT);
        SetShaderValue(unified_agent_shader, uniform_event_horizon, &horizon_radius, SHADER_UNIFORM_FLOAT);

        // --- STAGE 2: DRAW 3D AGENT TRAJECTORIES FIRST (BEFORE SHADER) ---
        const auto& population = p_matrix.get_read_buffer();
        const uint64_t entity_max_bounds = p_matrix.get_capacity();

        // 3D-Kontext starten, um Linien im korrekten Raum zu zeichnen
        BeginMode3D(p_camera);
        for (uint64_t i = 0; i < entity_max_bounds; ++i) {
            const AgentState& agent = population[i];

            if (!agent.is_active || agent.type == AgentType::ATTRACTOR) continue;

            Vector3 vStart = agent.position;
            Vector3 vEnd = Vector3Add(agent.position, Vector3Scale(agent.velocity, 0.02f));

            DrawLine3D(vStart, vEnd, agent.color);
        }
        EndMode3D();

        // --- STAGE 3: APPLY POST-PROCESSING ACCRETION SHADER OVER SCENE ---
        BeginShaderMode(unified_agent_shader);
        BeginBlendMode(BLEND_ALPHA);

        // Das Rechteck fängt die Shader-Pixel ein und blendet sie über die Asteroiden
        DrawRectangle(0, 0, GetScreenWidth(), GetScreenHeight(), Color{ 0, 0, 0, 0 });

        EndBlendMode();
        EndShaderMode();
    }

} // namespace stellar_agents

