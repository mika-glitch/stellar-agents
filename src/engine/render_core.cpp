// ============================================================================
// [Core/Graphics] VULKAN/BGFX RENDERING PIPELINE IMPLEMENTATION
// Description: Implements volumetric raymarching for gravitational lensing 
//              and high-throughput hardware-instanced mesh dispatching from 
//              Structure of Arrays (SoA) memory arenas. Enforces strict 
//              memory alignment protocols for multi-stream vertex buffers.
// Standard: ISO C++20
// ============================================================================

#include "engine/render_core.h"
#include "engine/engine_config.h"
#include "cads/environment_matrix.h"
#include "geometric_primitives.h" // Hardware Geometry Injector

#include <bgfx/bgfx.h>
#include <bx/math.h>
#include <vector>
#include <fstream>
#include <iostream>
#include <chrono>

namespace stellar_agents {

    static bgfx::VertexLayout s_fullscreen_layout;
    static bgfx::VertexLayout s_mesh_layout;
    static bgfx::VertexLayout s_instance_layout;
    static bool s_layouts_initialized = false;

    // Static geometry handles for instantiated hardware dispatch pipelines
    static bgfx::VertexBufferHandle s_asteroid_vbh = BGFX_INVALID_HANDLE;
    static bgfx::IndexBufferHandle  s_asteroid_ibh = BGFX_INVALID_HANDLE;
    static bgfx::VertexBufferHandle s_chassis_vbh = BGFX_INVALID_HANDLE;
    static bgfx::IndexBufferHandle  s_chassis_ibh = BGFX_INVALID_HANDLE;

    struct FullscreenVertex {
        float x, y, z;
        float u, v;
        uint32_t abgr;
    };

    /**
     * @brief Matrix mapping stream payload for hardware instancing.
     * Enforces tight packing via pragmas to prevent compiler-injected padding
     * bytes, guaranteeing exact byte stride alignment ($64\text{ bytes matrix} +
     * 4\text{ bytes color} = 68\text{ bytes total stride}$).
     */
#pragma pack(push, 1)
    struct InstanceData {
        float mtx[16];
        uint32_t abgr;
    };
#pragma pack(pop)

    // Loads compiled binary shader streams from persistent storage with fallback handling.
    static bgfx::ShaderHandle load_shader(const char* filename) {
        std::string fullPath = filename;
        std::ifstream file(fullPath, std::ios::binary | std::ios::ate);

        if (!file.is_open()) {
            fullPath = std::string("build/Release/") + filename;
            file.open(fullPath, std::ios::binary | std::ios::ate);
        }

        if (!file.is_open()) {
            std::cerr << "[RenderCore] CRITICAL: Failed to locate shader binary: " << filename << std::endl;
            return BGFX_INVALID_HANDLE;
        }

        std::streamsize size = file.tellg();
        file.seekg(0, std::ios::beg);

        const auto mem = bgfx::alloc(static_cast<uint32_t>(size) + 1);
        if (file.read(reinterpret_cast<char*>(mem->data), size)) {
            mem->data[size] = '\0';
        }
        file.close();

        return bgfx::createShader(mem);
    }

    RenderCore::RenderCore() noexcept {
        // Initialize immutable vertex stream layouts for multi-pass execution pipelines.
        if (!s_layouts_initialized) {
            s_fullscreen_layout.begin()
                .add(bgfx::Attrib::Position, 3, bgfx::AttribType::Float)
                .add(bgfx::Attrib::TexCoord0, 2, bgfx::AttribType::Float)
                .add(bgfx::Attrib::Color0, 4, bgfx::AttribType::Uint8, true)
                .end();

            s_mesh_layout.begin()
                .add(bgfx::Attrib::Position, 3, bgfx::AttribType::Float)
                .add(bgfx::Attrib::Normal, 3, bgfx::AttribType::Float)
                .end();

            // Hardware Instancing Matrix Format: 4x Vec4 attribute streams mapping 
            // the affine transformation matrix columns ($M_{\text{world}}$) plus RGBA color payload.
            s_instance_layout.begin()
                .add(bgfx::Attrib::TexCoord4, 4, bgfx::AttribType::Float)
                .add(bgfx::Attrib::TexCoord5, 4, bgfx::AttribType::Float)
                .add(bgfx::Attrib::TexCoord6, 4, bgfx::AttribType::Float)
                .add(bgfx::Attrib::TexCoord7, 4, bgfx::AttribType::Float)
                .add(bgfx::Attrib::Color0, 4, bgfx::AttribType::Uint8, true)
                .end();

            s_layouts_initialized = true;

            // ====================================================================
            // HARDWARE GEOMETRY UPLOAD
            // Compile procedural topological manifolds into persistent VRAM buffers.
            // ====================================================================
            auto asteroidMesh = GeometricPrimitives::GenerateDebrisManifold();
            s_asteroid_vbh = bgfx::createVertexBuffer(bgfx::copy(asteroidMesh.vertices.data(), static_cast<uint32_t>(asteroidMesh.vertices.size() * sizeof(MeshVertex))), s_mesh_layout);
            s_asteroid_ibh = bgfx::createIndexBuffer(bgfx::copy(asteroidMesh.indices.data(), static_cast<uint32_t>(asteroidMesh.indices.size() * sizeof(uint16_t))));

            auto chassisMesh = GeometricPrimitives::GenerateChassisManifold();
            s_chassis_vbh = bgfx::createVertexBuffer(bgfx::copy(chassisMesh.vertices.data(), static_cast<uint32_t>(chassisMesh.vertices.size() * sizeof(MeshVertex))), s_mesh_layout);
            s_chassis_ibh = bgfx::createIndexBuffer(bgfx::copy(chassisMesh.indices.data(), static_cast<uint32_t>(chassisMesh.indices.size() * sizeof(uint16_t))));
        }

        // Load volumetric raymarching shader programs for background space curvature.
        bgfx::ShaderHandle fsh = load_shader("accretion_frag.bin");
        bgfx::ShaderHandle vsh = load_shader("lensing_vert.bin");

        if (bgfx::isValid(vsh) && bgfx::isValid(fsh)) {
            uniform_agent_program = bgfx::createProgram(vsh, fsh, true);
        }
        else {
            std::cerr << "[RenderCore] CRITICAL: Failed to link main raymarching pipeline." << std::endl;
        }

        // Load dedicated hardware instancing shader programs for 3D geometric entities.
        bgfx::ShaderHandle inst_vsh = load_shader("instancing_vert.bin");
        bgfx::ShaderHandle inst_fsh = load_shader("instancing_frag.bin");

        if (bgfx::isValid(inst_vsh) && bgfx::isValid(inst_fsh)) {
            m_particle_program = bgfx::createProgram(inst_vsh, inst_fsh, true);
        }
        else {
            std::cerr << "[RenderCore] CRITICAL: Failed to link hardware instancing pipeline." << std::endl;
        }

        // Register pipeline-wide uniform register blocks.
        uniform_time = bgfx::createUniform("u_cameraPositionAndTime", bgfx::UniformType::Vec4);
        uniform_resolution = bgfx::createUniform("u_resolutionAndMass", bgfx::UniformType::Vec4);
        uniform_hole_pos = bgfx::createUniform("u_singularityPosition", bgfx::UniformType::Vec4);
        uniform_cam_target = bgfx::createUniform("u_cameraTarget", bgfx::UniformType::Vec4);

        s_uniform_obj1 = bgfx::createUniform("u_object1", bgfx::UniformType::Vec4);
        s_uniform_obj2 = bgfx::createUniform("u_object2", bgfx::UniformType::Vec4);
        s_uniform_obj3 = bgfx::createUniform("u_object3", bgfx::UniformType::Vec4);

        // Define full-screen quad geometry for screen-space raymarching dispatch.
        FullscreenVertex vertices[] = {
            {-1.0f, -1.0f, 0.0f,  0.0f, 0.0f, 0x00000000},
            { 3.0f, -1.0f, 0.0f,  2.0f, 0.0f, 0x00000000},
            {-1.0f,  3.0f, 0.0f,  0.0f, 2.0f, 0x00000000}
        };

        const bgfx::Memory* mem = bgfx::copy(vertices, sizeof(vertices));
        m_fullscreenVbh = bgfx::createVertexBuffer(mem, s_fullscreen_layout);
    }

    RenderCore::~RenderCore() {
        if (bgfx::isValid(m_fullscreenVbh)) bgfx::destroy(m_fullscreenVbh);
        if (bgfx::isValid(uniform_agent_program)) bgfx::destroy(uniform_agent_program);
        if (bgfx::isValid(m_particle_program)) bgfx::destroy(m_particle_program);

        if (bgfx::isValid(uniform_time)) bgfx::destroy(uniform_time);
        if (bgfx::isValid(uniform_resolution)) bgfx::destroy(uniform_resolution);
        if (bgfx::isValid(uniform_hole_pos)) bgfx::destroy(uniform_hole_pos);
        if (bgfx::isValid(uniform_cam_target)) bgfx::destroy(uniform_cam_target);

        if (bgfx::isValid(s_uniform_obj1)) bgfx::destroy(s_uniform_obj1);
        if (bgfx::isValid(s_uniform_obj2)) bgfx::destroy(s_uniform_obj2);
        if (bgfx::isValid(s_uniform_obj3)) bgfx::destroy(s_uniform_obj3);
    }

    void RenderCore::draw_composite_scene(const void* p_camera_data, const EnvironmentMatrix& p_matrix) const noexcept {
        if (!bgfx::isValid(uniform_agent_program) || !bgfx::isValid(m_fullscreenVbh)) {
            return;
        }

        static auto startTime = std::chrono::steady_clock::now();
        std::chrono::duration<float> elapsed = std::chrono::steady_clock::now() - startTime;
        float current_time = elapsed.count();

        // Default camera parameters
        float cam_pos[3] = { 0.0f, 20.0f, -60.0f };
        float cam_target[3] = { 100.0f, 0.0f, 0.0f };
        float cam_up[3] = { 0.0f, 1.0f, 0.0f };

        if (p_camera_data != nullptr) {
            const float* src = static_cast<const float*>(p_camera_data);
            cam_pos[0] = src[0];    cam_pos[1] = src[1];    cam_pos[2] = src[2];
            cam_target[0] = src[3]; cam_target[1] = src[4]; cam_target[2] = src[5];
        }

        float view_matrix[16];
        float proj_matrix[16];

        // Construct view transformation matrix utilizing absolute spatial target coordinates
        bx::Vec3 eye = { cam_pos[0], cam_pos[1], cam_pos[2] };
        bx::Vec3 at  = { cam_target[0], cam_target[1], cam_target[2] };
        bx::Vec3 up  = { 0.0f, 1.0f, 0.0f };

        bx::mtxLookAt(view_matrix, eye, at, up);

        bx::mtxProj(proj_matrix,
            config::visual::fov_degrees,
            config::visual::aspect_ratio,
            0.1f, 20000.0f,
            bgfx::getCaps()->homogeneousDepth
        );

        bgfx::setViewTransform(0, view_matrix, proj_matrix);
        bgfx::setViewTransform(1, view_matrix, proj_matrix);

        // Bind global simulation telemetry uniforms.
        float cam_time_data[4] = { cam_pos[0], cam_pos[1], cam_pos[2], current_time };
        bgfx::setUniform(uniform_time, cam_time_data);

        float res_mass_data[4] = {
            config::visual::screen_width,
            config::visual::screen_height,
            config::physics::primary_attractor_runtime_mass,
            config::physics::rs_horizon
        };
        bgfx::setUniform(uniform_resolution, res_mass_data);

        float singularity_pos[4] = { 0.0f, 0.0f, 0.0f, 0.0f };
        bgfx::setUniform(uniform_hole_pos, singularity_pos);

        float cam_target_data[4] = { cam_target[0], cam_target[1], cam_target[2], 0.0f };
        bgfx::setUniform(uniform_cam_target, cam_target_data);

        const auto& read_buffer = p_matrix.get_read_buffer();
        const uint64_t capacity = p_matrix.get_capacity();

        Vector3 pos_inner_body = (capacity > 1) ? Vector3{ read_buffer.pos_x[1], read_buffer.pos_y[1], read_buffer.pos_z[1] } : Vector3{ 0.0f, 0.0f, 0.0f };
        Vector3 pos_outer_body = (capacity > 2) ? Vector3{ read_buffer.pos_x[2], read_buffer.pos_y[2], read_buffer.pos_z[2] } : Vector3{ 0.0f, 0.0f, 0.0f };
        Vector3 pos_gateway_node = { config::astrodynamics::gateway_node_pos[0], config::astrodynamics::gateway_node_pos[1], config::astrodynamics::gateway_node_pos[2] };

        float obj1[4] = { pos_inner_body.x, pos_inner_body.y, pos_inner_body.z, config::visual::radius_inner_body };
        float obj2[4] = { pos_outer_body.x, pos_outer_body.y, pos_outer_body.z, config::visual::radius_outer_body };
        float obj3[4] = { pos_gateway_node.x, pos_gateway_node.y, pos_gateway_node.z, config::visual::radius_gateway_node };

        bgfx::setUniform(s_uniform_obj1, obj1);
        bgfx::setUniform(s_uniform_obj2, obj2);
        bgfx::setUniform(s_uniform_obj3, obj3);

        // ====================================================================
        // RENDER PASS 1: Volumetric Raymarching Background & Gravitational Lensing
        // ====================================================================
        float identity_matrix[16];
        bx::mtxIdentity(identity_matrix);
        bgfx::setTransform(identity_matrix);

        bgfx::setVertexBuffer(0, m_fullscreenVbh);
        bgfx::setState(BGFX_STATE_WRITE_RGB | BGFX_STATE_WRITE_A);
        bgfx::submit(0, uniform_agent_program);

        // ====================================================================
        // RENDER PASS 2: Hardware Instanced Dispatch from Structure of Arrays (SoA)
        // Architecture: Stream-compaction over contiguous SoA memory arenas.
        // Utilizes safe sub-batch chunking to prevent ring-buffer exhaustion 
        // under high-density spatial manifold loads ($N > 10000$).
        // ====================================================================
        
        std::vector<uint64_t> asteroid_indices;
        std::vector<uint64_t> ship_indices;
        // PREVENT HEAP REALLOCATION STORM: Reserve maximum possible bounds upfront
        asteroid_indices.reserve(capacity);
        ship_indices.reserve(capacity);
        const uint64_t num_asteroids = config::simulation::total_asteroids;
        const uint64_t ship_start_index = 4 + num_asteroids;

        // 1. Instantaneous Deterministic Filling for Asteroids
        // Bypasses millions of redundant conditional checks since the seeder packs them continuously.
        asteroid_indices.resize(num_asteroids);
        for (uint64_t i = 0; i < num_asteroids; ++i) {
            asteroid_indices[i] = 4 + i;
        }

        // 2. Focused Scan only for Sparse Dynamic Fleet Units
        ship_indices.reserve(config::simulation::total_ships);
        for (uint64_t i = ship_start_index; i < capacity; ++i) {
            if (read_buffer.is_active[i] == 0) continue;
            if (static_cast<AgentType>(read_buffer.agent_type[i]) == AgentType::ADAPTIVE) {
                ship_indices.push_back(i);
            }
        }

        // --- DISPATCH PASSIVE DEBRIS (ASTEROID FIELD MANIFOLDS - BATCHED STREAMING) ---
        constexpr uint32_t MAX_TRANSIENT_BATCH_SIZE = 4000;
        size_t ast_offset = 0;

        while (ast_offset < asteroid_indices.size()) {
            uint32_t batch_size = static_cast<uint32_t>(std::min(size_t(MAX_TRANSIENT_BATCH_SIZE), asteroid_indices.size() - ast_offset));

            if (bgfx::getAvailInstanceDataBuffer(batch_size, s_instance_layout.m_stride) >= batch_size) {
                bgfx::InstanceDataBuffer idb_ast;
                bgfx::allocInstanceDataBuffer(&idb_ast, batch_size, s_instance_layout.m_stride);
                auto* instance_dst = reinterpret_cast<InstanceData*>(idb_ast.data);

                for (uint32_t idx = 0; idx < batch_size; ++idx) {
                    uint64_t i = asteroid_indices[ast_offset + idx];
                    float px = read_buffer.pos_x[i];
                    float py = read_buffer.pos_y[i];
                    float pz = read_buffer.pos_z[i];
                    uint32_t agent_id = read_buffer.agent_id[i];

                    // Procedural tumbling rotation vector calculation based on temporal drift and unique ID hash
                    float rotX = current_time * 0.2f + static_cast<float>(agent_id) * 0.1f;
                    float rotY = current_time * 0.3f + static_cast<float>(agent_id) * 0.2f;
                    float rotZ = current_time * 0.1f + static_cast<float>(agent_id) * 0.3f;

                    float scale = 0.8f + (static_cast<float>(agent_id % 10) * 0.1f);

                    // Construct model transform matrix via Scale-Rotation-Translation composition:
                    // $M_{\text{world}} = T(\mathbf{p}) \cdot R(\boldsymbol{\theta}) \cdot S(\mathbf{s})$
                    bx::mtxSRT(instance_dst[idx].mtx, scale, scale, scale, rotX, rotY, rotZ, px, py, pz);

                    uint8_t alpha = (read_buffer.color_a[i] < 10) ? 255 : read_buffer.color_a[i];
                    instance_dst[idx].abgr = (static_cast<uint32_t>(alpha) << 24) |
                        (static_cast<uint32_t>(read_buffer.color_b[i]) << 16) |
                        (static_cast<uint32_t>(read_buffer.color_g[i]) << 8) |
                        static_cast<uint32_t>(read_buffer.color_r[i]);
                }

                bgfx::setVertexBuffer(0, s_asteroid_vbh);
                bgfx::setIndexBuffer(s_asteroid_ibh);
                bgfx::setInstanceDataBuffer(&idb_ast);
                bgfx::setState(BGFX_STATE_DEFAULT);
                bgfx::submit(1, m_particle_program);
            }
            ast_offset += batch_size;
        }

        // --- DISPATCH ADAPTIVE CHASSIS (AUTONOMOUS NAVIGATION UNITS - BATCHED STREAMING) ---
        size_t shp_offset = 0;

        while (shp_offset < ship_indices.size()) {
            uint32_t batch_size = static_cast<uint32_t>(std::min(size_t(MAX_TRANSIENT_BATCH_SIZE), ship_indices.size() - shp_offset));

            if (bgfx::getAvailInstanceDataBuffer(batch_size, s_instance_layout.m_stride) >= batch_size) {
                bgfx::InstanceDataBuffer idb_shp;
                bgfx::allocInstanceDataBuffer(&idb_shp, batch_size, s_instance_layout.m_stride);
                auto* instance_dst = reinterpret_cast<InstanceData*>(idb_shp.data);

                for (uint32_t idx = 0; idx < batch_size; ++idx) {
                    uint64_t i = ship_indices[shp_offset + idx];
                    float px = read_buffer.pos_x[i];
                    float py = read_buffer.pos_y[i];
                    float pz = read_buffer.pos_z[i];

                    float vx = read_buffer.vel_x[i];
                    float vy = read_buffer.vel_y[i];
                    float vz = read_buffer.vel_z[i];
                    
                    // Velocity magnitude computation: $\|\mathbf{v}\| = \sqrt{v_x^2 + v_y^2 + v_z^2}$
                    float speed = std::sqrt(vx * vx + vy * vy + vz * vz);

                    // Construct orthonormal tangent-space basis aligned to velocity vector trajectory
                    bx::Vec3 forward = { 0.0f, 0.0f, 1.0f };
                    if (speed > 0.001f) {
                        forward = { vx / speed, vy / speed, vz / speed };
                    }

                    bx::Vec3 up = { 0.0f, 1.0f, 0.0f };
                    if (std::abs(forward.y) > 0.99f) {
                        up = { 0.0f, 0.0f, 1.0f };
                    }

                    bx::Vec3 right = bx::normalize(bx::cross(up, forward));
                    up = bx::normalize(bx::cross(forward, right));

                    float scale = 0.6f;

                    // Manually assemble orientation matrix columns to ensure exact alignment with flight heading vectors
                    instance_dst[idx].mtx[0] = right.x * scale;
                    instance_dst[idx].mtx[1] = right.y * scale;
                    instance_dst[idx].mtx[2] = right.z * scale;
                    instance_dst[idx].mtx[3] = 0.0f;

                    instance_dst[idx].mtx[4] = up.x * scale;
                    instance_dst[idx].mtx[5] = up.y * scale;
                    instance_dst[idx].mtx[6] = up.z * scale;
                    instance_dst[idx].mtx[7] = 0.0f;

                    instance_dst[idx].mtx[8] = forward.x * scale;
                    instance_dst[idx].mtx[9] = forward.y * scale;
                    instance_dst[idx].mtx[10] = forward.z * scale;
                    instance_dst[idx].mtx[11] = 0.0f;

                    instance_dst[idx].mtx[12] = px;
                    instance_dst[idx].mtx[13] = py;
                    instance_dst[idx].mtx[14] = pz;
                    instance_dst[idx].mtx[15] = 1.0f;

                    uint8_t alpha = (read_buffer.color_a[i] < 10) ? 255 : read_buffer.color_a[i];
                    instance_dst[idx].abgr = (static_cast<uint32_t>(alpha) << 24) |
                        (static_cast<uint32_t>(read_buffer.color_b[i]) << 16) |
                        (static_cast<uint32_t>(read_buffer.color_g[i]) << 8) |
                        static_cast<uint32_t>(read_buffer.color_r[i]);
                }

                bgfx::setVertexBuffer(0, s_chassis_vbh);
                bgfx::setIndexBuffer(s_chassis_ibh);
                bgfx::setInstanceDataBuffer(&idb_shp);
                bgfx::setState(BGFX_STATE_DEFAULT);
                bgfx::submit(1, m_particle_program);
            }
            shp_offset += batch_size;
        }
    }

} // namespace stellar_agents