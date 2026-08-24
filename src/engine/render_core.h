#ifndef RENDER_CORE_H
#define RENDER_CORE_H

#include "raylib.h"
#include "cads/environment_matrix.h"

namespace stellar_agents {

    // ============================================================================
    // HARDWARE GRAPHICS DISPATCH ENGINE (UNIFIED AGENT SHADER PIPELINE)
    // Standard compliance: Enforces a zero-allocation GPU streaming architecture.
    // Orchestrates the centralized vertex/fragment pipeline to compute relativistic 
    // lensing and agent geometry directly inside the silicon registers.
    // ============================================================================
    class RenderCore final {
    private:
        // Unified shader program driving both volumetric fields and agent projections
        Shader unified_agent_shader{ 0 };

        // Core hardware uniform registry indices
        int32_t uniform_time{ 0 };
        int32_t uniform_resolution{ 0 };
        int32_t uniform_cam_pos{ 0 };
        int32_t uniform_cam_target{ 0 };

        // Singularity metrics to feed the hardware lensing logic inside the vertex pipeline
        int32_t uniform_hole_pos{ 0 };
        int32_t uniform_hole_mass{ 0 };
        int32_t uniform_event_horizon{ 0 };

    public:
        RenderCore() noexcept;
        ~RenderCore();

        // Prevent unsafe compilation copy operations over hardware resource handles
        RenderCore(const RenderCore&) = delete;
        RenderCore& operator=(const RenderCore&) = delete;

        // ============================================================================
        // UNIFIED UNIFIED BLIT OPERATOR
        // Direct hardware pipeline streaming layout. Takes the flat CPU readable state 
        // array, injects it into VRAM, and triggers parallel GPU shader execution.
        // ============================================================================
        void draw_composite_scene(const Camera3D& p_camera, const EnvironmentMatrix& p_matrix) const noexcept;
    };

} // namespace stellar_agents

#endif // RENDER_CORE_H
