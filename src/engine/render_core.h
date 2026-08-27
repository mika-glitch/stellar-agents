// ============================================================================
// [Core/Graphics] VULKAN/BGFX RENDERING PIPELINE INTERFACE
// Description: Declares the rendering core subsystem responsible for 
//              volumetric raymarching of gravitational lensing effects 
//              and particle dispatcher pipelines streaming from Structure 
//              of Arrays (SoA) memory arenas.
// Standard: ISO C++20
// ============================================================================
#ifndef RENDER_CORE_H
#define RENDER_CORE_H

#include <bgfx/bgfx.h>
#include "cads/environment_matrix.h"
#include <cstdint>

namespace stellar_agents {

    /**
     * @brief Core Rendering Subsystem.
     * @details Manages shader programs, vertex layouts, uniform handles, and
     *          scene composition passes for both background volumetric effects
     *          and particle rendering.
     */
    class RenderCore final {
    private:
        // Raymarching Pipeline Resources
        bgfx::ProgramHandle uniform_agent_program{ BGFX_INVALID_HANDLE };
        bgfx::VertexBufferHandle m_fullscreenVbh{ BGFX_INVALID_HANDLE };

        // Point-Particle Pipeline Resources for Debris and Fleet Nodes
        bgfx::ProgramHandle m_particle_program{ BGFX_INVALID_HANDLE };
        bgfx::VertexLayout  m_particle_layout;

        // Shader Uniform Handles
        bgfx::UniformHandle uniform_time{ BGFX_INVALID_HANDLE };
        bgfx::UniformHandle uniform_resolution{ BGFX_INVALID_HANDLE };
        bgfx::UniformHandle uniform_cam_pos{ BGFX_INVALID_HANDLE };
        bgfx::UniformHandle uniform_cam_target{ BGFX_INVALID_HANDLE };
        bgfx::UniformHandle uniform_hole_pos{ BGFX_INVALID_HANDLE };

        bgfx::UniformHandle s_uniform_obj1{ BGFX_INVALID_HANDLE };
        bgfx::UniformHandle s_uniform_obj2{ BGFX_INVALID_HANDLE };
        bgfx::UniformHandle s_uniform_obj3{ BGFX_INVALID_HANDLE };

    public:
        RenderCore() noexcept;
        ~RenderCore();

        RenderCore(const RenderCore&) = delete;
        RenderCore& operator=(const RenderCore&) = delete;
        RenderCore(RenderCore&&) noexcept = delete;
        RenderCore& operator=(RenderCore&&) noexcept = delete;

        /**
         * @brief Executes the composite scene rendering passes.
         * @param p_camera_data Pointer to the 6-DOF camera transformation data.
         * @param p_matrix Reference to the environment SoA matrix buffer.
         */
        void draw_composite_scene(const void* p_camera_data, const EnvironmentMatrix& p_matrix) const noexcept;
    };

} // namespace stellar_agents

#endif // RENDER_CORE_H