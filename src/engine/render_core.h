#ifndef RENDER_CORE_H
#define RENDER_CORE_H

#include "raylib.h"
#include "environment_matrix.h"

namespace stellar_agents {

    // ============================================================================
    // ZERO-ALLOCATION HARDWARE GRAPHICS DISPATCH
    // Standard compliance: Enforced strict separation of simulation logic from rendering.
    // Responsible for shader pipeline lifecycle management and raw VRAM bulk-blitting.
    // ============================================================================
    class RenderCore final {
    private:
        Shader accretion_shader{ 0 };

        // Shader uniform registration indices
        int32_t uniform_time{ 0 };
        int32_t uniform_resolution{ 0 };
        int32_t uniform_cam_pos{ 0 };
        int32_t uniform_cam_target{ 0 };

    public:
        RenderCore() noexcept;
        ~RenderCore();

        // Suppress copying to secure unmanaged GPU system resource handles
        RenderCore(const RenderCore&) = delete;
        RenderCore& operator=(const RenderCore&) = delete;

        // ============================================================================
        // DUAL-STAGE VISUAL COMPOSITING BLIT
        // Stage 1: Renders the continuous volumetric plasma fields via Fragment Shader.
        // Stage 2: Blits the discrete adaptive agent vector arrays via primitive lines.
        // ============================================================================
        void draw_composite_scene(const Camera3D& p_camera, const EnvironmentMatrix& p_matrix) const noexcept;
    };

} // namespace stellar_agents

#endif // RENDER_CORE_H
#pragma once