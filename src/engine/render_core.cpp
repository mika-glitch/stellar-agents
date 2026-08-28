// ============================================================================
// [Core/Graphics] VULKAN/BGFX RENDERING PIPELINE IMPLEMENTATION
// Description: Volumetric raymarching (gravitational lensing) + hardware
//              instanced mesh dispatch from SoA arenas.
// Standard: ISO C++20
// ============================================================================

#include "engine/render_core.h"
#include "engine/engine_config.h"
#include "cads/environment_matrix.h"
#include "geometric_primitives.h"

#include <bgfx/bgfx.h>
#include <bx/math.h>

#include <chrono>
#include <cstdint>
#include <fstream>
#include <iostream>
#include <vector>

#if defined(__GNUC__) || defined(__clang__)
#define RC_RESTRICT __restrict__
#elif defined(_MSC_VER)
#define RC_RESTRICT __restrict
#else
#define RC_RESTRICT
#endif

namespace stellar_agents {

    static bgfx::VertexLayout s_fullscreen_layout;
    static bgfx::VertexLayout s_mesh_layout;
    static bgfx::VertexLayout s_instance_layout;
    static uint16_t s_instance_stride = 0;
    static bool s_layouts_initialized = false;

    static bgfx::VertexBufferHandle s_asteroid_vbh = BGFX_INVALID_HANDLE;
    static bgfx::IndexBufferHandle  s_asteroid_ibh = BGFX_INVALID_HANDLE;
    static bgfx::VertexBufferHandle s_chassis_vbh = BGFX_INVALID_HANDLE;
    static bgfx::IndexBufferHandle  s_chassis_ibh = BGFX_INVALID_HANDLE;

    struct FullscreenVertex {
        float x, y, z;
        float u, v;
        uint32_t abgr;
    };

    // bgfx instance stride MUST be a multiple of 16.
    // The old pack(1) 68-byte layout (64 mtx + 4 color) failed that contract,
    // forced unaligned stores, and trips BX_ASSERT(0 == (stride & 15)) in debug.
    // 80 bytes = 5×16, matching 4×vec4 + rgba8 + 12-byte skip.
    struct alignas(16) InstanceData {
        float    mtx[16];
        uint32_t abgr;
        uint32_t _pad[3];
    };
    static_assert(sizeof(InstanceData) == 80, "instance stride must be 16-byte aligned");
    static_assert(alignof(InstanceData) >= 16);

    namespace {

        constexpr uint32_t kMaxInstanceBatch = 65536;
        constexpr float    kFwdLen2Epsilon = 1.0e-6f;
        constexpr uint64_t kFixedBodyCount = 4; // singularity + 2 bodies + gateway

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

            const std::streamsize size = file.tellg();
            file.seekg(0, std::ios::beg);

            const auto mem = bgfx::alloc(static_cast<uint32_t>(size) + 1);
            if (file.read(reinterpret_cast<char*>(mem->data), size)) {
                mem->data[size] = '\0';
            }
            file.close();

            return bgfx::createShader(mem);
        }

        inline uint32_t pack_abgr(uint8_t r, uint8_t g, uint8_t b, uint8_t a) {
            a = static_cast<uint8_t>(a < 10u ? 255u : a);
            return (uint32_t(a) << 24) | (uint32_t(b) << 16) | (uint32_t(g) << 8) | uint32_t(r);
        }

        // Identical 3x3 to bx::mtxSRT (ZYX / mtxRotateZYX order) with uniform scale.
        // Sin/cos are passed in so the caller can reuse cached id-phase and a
        // single per-frame time sincos via angle addition.
        inline void mtx_srt_uniform_sc(float* RC_RESTRICT m, float scale,
            float sx, float cx,
            float sy, float cy,
            float sz, float cz,
            float tx, float ty, float tz) {
            const float czsy = cz * sy;
            const float szsy = sz * sy;

            m[0] = scale * (cy * cz);
            m[1] = scale * (czsy * sx - cx * sz);
            m[2] = scale * (czsy * cx + sx * sz);
            m[3] = 0.0f;

            m[4] = scale * (cy * sz);
            m[5] = scale * (cx * cz + sx * szsy);
            m[6] = scale * (cx * szsy - cz * sx);
            m[7] = 0.0f;

            m[8] = scale * (-sy);
            m[9] = scale * (cy * sx);
            m[10] = scale * (cx * cy);
            m[11] = 0.0f;

            m[12] = tx;
            m[13] = ty;
            m[14] = tz;
            m[15] = 1.0f;
        }

        inline void angle_add(float st, float ct, float s0, float c0, float& s, float& c) {
            s = st * c0 + ct * s0; // sin(t+id)
            c = ct * c0 - st * s0; // cos(t+id)
        }

        // One rsqrt (not sqrt+div). `up = forward × right` is already unit so
        // the second normalize the old code did is dead work.
        inline void velocity_basis(float vx, float vy, float vz,
            bx::Vec3& forward, bx::Vec3& right, bx::Vec3& up) {
            const float len2 = vx * vx + vy * vy + vz * vz;
            if (len2 > kFwdLen2Epsilon) {
                const float inv = bx::rsqrt(len2);
                forward = { vx * inv, vy * inv, vz * inv };
            }
            else {
                forward = { 0.0f, 0.0f, 1.0f };
            }

            up = { 0.0f, 1.0f, 0.0f };
            if (bx::abs(forward.y) > 0.99f) {
                up = { 0.0f, 0.0f, 1.0f };
            }

            right = bx::normalize(bx::cross(up, forward));
            up = bx::cross(forward, right);
        }

        inline void write_scaled_basis(float* RC_RESTRICT m,
            const bx::Vec3& r, const bx::Vec3& u, const bx::Vec3& f,
            float scale, float px, float py, float pz) {
            m[0] = r.x * scale; m[1] = r.y * scale; m[2] = r.z * scale; m[3] = 0.0f;
            m[4] = u.x * scale; m[5] = u.y * scale; m[6] = u.z * scale; m[7] = 0.0f;
            m[8] = f.x * scale; m[9] = f.y * scale; m[10] = f.z * scale; m[11] = 0.0f;
            m[12] = px;          m[13] = py;          m[14] = pz;          m[15] = 1.0f;
        }

        // Uses whatever the transient ring actually has left (partial batches)
        // instead of dropping a whole 4k chunk when avail < requested.
        template <typename Fill>
        void submit_instance_batches(bgfx::ViewId view,
            bgfx::ProgramHandle program,
            bgfx::VertexBufferHandle vbh,
            bgfx::IndexBufferHandle ibh,
            uint32_t count,
            Fill&& fill) {
            const uint16_t stride = s_instance_stride;
            uint32_t offset = 0;
            while (offset < count) {
                uint32_t want = count - offset;
                if (want > kMaxInstanceBatch) {
                    want = kMaxInstanceBatch;
                }

                const uint32_t avail = bgfx::getAvailInstanceDataBuffer(want, stride);
                if (avail == 0) {
                    break;
                }
                const uint32_t batch = want < avail ? want : avail;

                bgfx::InstanceDataBuffer idb;
                bgfx::allocInstanceDataBuffer(&idb, batch, stride);
                fill(reinterpret_cast<InstanceData*>(idb.data), offset, batch);

                bgfx::setVertexBuffer(0, vbh);
                bgfx::setIndexBuffer(ibh);
                bgfx::setInstanceDataBuffer(&idb);
                bgfx::setState(BGFX_STATE_DEFAULT);
                bgfx::submit(view, program);

                offset += batch;
            }
        }

        struct AsteroidPhase {
            float s0, c0; // sin/cos(id * 0.1)
            float s1, c1; // sin/cos(id * 0.2)
            float s2, c2; // sin/cos(id * 0.3)
            float scale;
        };

        std::vector<AsteroidPhase>& asteroid_phase_cache() {
            thread_local std::vector<AsteroidPhase> cache;
            return cache;
        }

        std::vector<uint64_t>& ship_scratch() {
            thread_local std::vector<uint64_t> ids;
            return ids;
        }

        template <typename ReadBuffer>
        const AsteroidPhase* refresh_asteroid_phases(const ReadBuffer& rb,
            uint64_t begin,
            uint64_t n) {
            auto& cache = asteroid_phase_cache();
            thread_local uint64_t cached_n = ~uint64_t{ 0 };
            thread_local uint32_t cached_id0 = 0;

            const uint32_t id0 = (n > 0) ? rb.agent_id[begin] : 0;
            if (cached_n == n && cached_id0 == id0 && cache.size() == n) {
                return cache.data();
            }

            cache.resize(n);
            for (uint64_t k = 0; k < n; ++k) {
                const uint32_t id = rb.agent_id[begin + k];
                const float    idf = static_cast<float>(id);
                AsteroidPhase& e = cache[k];
                e.s0 = bx::sin(idf * 0.1f); e.c0 = bx::cos(idf * 0.1f);
                e.s1 = bx::sin(idf * 0.2f); e.c1 = bx::cos(idf * 0.2f);
                e.s2 = bx::sin(idf * 0.3f); e.c2 = bx::cos(idf * 0.3f);
                e.scale = 0.8f + static_cast<float>(id % 10u) * 0.1f;
            }
            cached_n = n;
            cached_id0 = id0;
            return cache.data();
        }

    } // namespace

    RenderCore::RenderCore() noexcept {
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

            s_instance_layout.begin()
                .add(bgfx::Attrib::TexCoord4, 4, bgfx::AttribType::Float)
                .add(bgfx::Attrib::TexCoord5, 4, bgfx::AttribType::Float)
                .add(bgfx::Attrib::TexCoord6, 4, bgfx::AttribType::Float)
                .add(bgfx::Attrib::TexCoord7, 4, bgfx::AttribType::Float)
                .add(bgfx::Attrib::Color0, 4, bgfx::AttribType::Uint8, true)
                .skip(12)
                .end();

            s_instance_stride = s_instance_layout.m_stride;
            s_layouts_initialized = true;

            auto asteroidMesh = GeometricPrimitives::GenerateDebrisManifold();
            s_asteroid_vbh = bgfx::createVertexBuffer(
                bgfx::copy(asteroidMesh.vertices.data(),
                    static_cast<uint32_t>(asteroidMesh.vertices.size() * sizeof(MeshVertex))),
                s_mesh_layout);
            s_asteroid_ibh = bgfx::createIndexBuffer(
                bgfx::copy(asteroidMesh.indices.data(),
                    static_cast<uint32_t>(asteroidMesh.indices.size() * sizeof(uint16_t))));

            auto chassisMesh = GeometricPrimitives::GenerateChassisManifold();
            s_chassis_vbh = bgfx::createVertexBuffer(
                bgfx::copy(chassisMesh.vertices.data(),
                    static_cast<uint32_t>(chassisMesh.vertices.size() * sizeof(MeshVertex))),
                s_mesh_layout);
            s_chassis_ibh = bgfx::createIndexBuffer(
                bgfx::copy(chassisMesh.indices.data(),
                    static_cast<uint32_t>(chassisMesh.indices.size() * sizeof(uint16_t))));
        }

        bgfx::ShaderHandle fsh = load_shader("accretion_frag.bin");
        bgfx::ShaderHandle vsh = load_shader("lensing_vert.bin");

        if (bgfx::isValid(vsh) && bgfx::isValid(fsh)) {
            uniform_agent_program = bgfx::createProgram(vsh, fsh, true);
        }
        else {
            std::cerr << "[RenderCore] CRITICAL: Failed to link main raymarching pipeline." << std::endl;
        }

        bgfx::ShaderHandle inst_vsh = load_shader("instancing_vert.bin");
        bgfx::ShaderHandle inst_fsh = load_shader("instancing_frag.bin");

        if (bgfx::isValid(inst_vsh) && bgfx::isValid(inst_fsh)) {
            m_particle_program = bgfx::createProgram(inst_vsh, inst_fsh, true);
        }
        else {
            std::cerr << "[RenderCore] CRITICAL: Failed to link hardware instancing pipeline." << std::endl;
        }

        uniform_time = bgfx::createUniform("u_cameraPositionAndTime", bgfx::UniformType::Vec4);
        uniform_resolution = bgfx::createUniform("u_resolutionAndMass", bgfx::UniformType::Vec4);
        uniform_hole_pos = bgfx::createUniform("u_singularityPosition", bgfx::UniformType::Vec4);
        uniform_cam_target = bgfx::createUniform("u_cameraTarget", bgfx::UniformType::Vec4);

        s_uniform_obj1 = bgfx::createUniform("u_object1", bgfx::UniformType::Vec4);
        s_uniform_obj2 = bgfx::createUniform("u_object2", bgfx::UniformType::Vec4);
        s_uniform_obj3 = bgfx::createUniform("u_object3", bgfx::UniformType::Vec4);

        FullscreenVertex vertices[] = {
            {-1.0f, -1.0f, 0.0f,  0.0f, 0.0f, 0x00000000},
            { 3.0f, -1.0f, 0.0f,  2.0f, 0.0f, 0x00000000},
            {-1.0f,  3.0f, 0.0f,  0.0f, 2.0f, 0x00000000}
        };

        m_fullscreenVbh = bgfx::createVertexBuffer(bgfx::copy(vertices, sizeof(vertices)), s_fullscreen_layout);
    }

    RenderCore::~RenderCore() {
        if (bgfx::isValid(m_fullscreenVbh))       bgfx::destroy(m_fullscreenVbh);
        if (bgfx::isValid(uniform_agent_program)) bgfx::destroy(uniform_agent_program);
        if (bgfx::isValid(m_particle_program))    bgfx::destroy(m_particle_program);

        if (bgfx::isValid(uniform_time))       bgfx::destroy(uniform_time);
        if (bgfx::isValid(uniform_resolution)) bgfx::destroy(uniform_resolution);
        if (bgfx::isValid(uniform_hole_pos))   bgfx::destroy(uniform_hole_pos);
        if (bgfx::isValid(uniform_cam_target)) bgfx::destroy(uniform_cam_target);

        if (bgfx::isValid(s_uniform_obj1)) bgfx::destroy(s_uniform_obj1);
        if (bgfx::isValid(s_uniform_obj2)) bgfx::destroy(s_uniform_obj2);
        if (bgfx::isValid(s_uniform_obj3)) bgfx::destroy(s_uniform_obj3);
    }

    void RenderCore::draw_composite_scene(const void* p_camera_data, const EnvironmentMatrix& p_matrix) const noexcept {
        if (!bgfx::isValid(uniform_agent_program) || !bgfx::isValid(m_fullscreenVbh)) {
            return;
        }

        static const auto startTime = std::chrono::steady_clock::now();
        const float current_time = std::chrono::duration<float>(std::chrono::steady_clock::now() - startTime).count();

        float cam_pos[3] = { 0.0f, 20.0f, -60.0f };
        float cam_target[3] = { 100.0f, 0.0f, 0.0f };

        if (p_camera_data != nullptr) {
            const float* src = static_cast<const float*>(p_camera_data);
            cam_pos[0] = src[0];    cam_pos[1] = src[1];    cam_pos[2] = src[2];
            cam_target[0] = src[3]; cam_target[1] = src[4]; cam_target[2] = src[5];
        }

        float view_matrix[16];
        float proj_matrix[16];

        const bx::Vec3 eye = { cam_pos[0],    cam_pos[1],    cam_pos[2] };
        const bx::Vec3 at = { cam_target[0], cam_target[1], cam_target[2] };
        const bx::Vec3 up = { 0.0f, 1.0f, 0.0f };

        bx::mtxLookAt(view_matrix, eye, at, up);
        bx::mtxProj(proj_matrix,
            config::visual::fov_degrees,
            config::visual::aspect_ratio,
            0.1f, 20000.0f,
            bgfx::getCaps()->homogeneousDepth);

        bgfx::setViewTransform(0, view_matrix, proj_matrix);
        bgfx::setViewTransform(1, view_matrix, proj_matrix);

        const float cam_time_data[4] = { cam_pos[0], cam_pos[1], cam_pos[2], current_time };
        bgfx::setUniform(uniform_time, cam_time_data);

        const float res_mass_data[4] = {
            config::visual::screen_width,
            config::visual::screen_height,
            config::physics::primary_attractor_runtime_mass,
            config::physics::rs_horizon
        };
        bgfx::setUniform(uniform_resolution, res_mass_data);

        const float singularity_pos[4] = { 0.0f, 0.0f, 0.0f, 0.0f };
        bgfx::setUniform(uniform_hole_pos, singularity_pos);

        const float cam_target_data[4] = { cam_target[0], cam_target[1], cam_target[2], 0.0f };
        bgfx::setUniform(uniform_cam_target, cam_target_data);

        const auto& rb = p_matrix.get_read_buffer();
        const uint64_t capacity = p_matrix.get_capacity();

        const Vector3 pos_inner_body = (capacity > 1)
            ? Vector3{ rb.pos_x[1], rb.pos_y[1], rb.pos_z[1] }
        : Vector3{ 0.0f, 0.0f, 0.0f };
        const Vector3 pos_outer_body = (capacity > 2)
            ? Vector3{ rb.pos_x[2], rb.pos_y[2], rb.pos_z[2] }
        : Vector3{ 0.0f, 0.0f, 0.0f };
        const Vector3 pos_gateway_node = {
            config::astrodynamics::gateway_node_pos[0],
            config::astrodynamics::gateway_node_pos[1],
            config::astrodynamics::gateway_node_pos[2]
        };

        const float obj1[4] = { pos_inner_body.x,   pos_inner_body.y,   pos_inner_body.z,   config::visual::radius_inner_body };
        const float obj2[4] = { pos_outer_body.x,   pos_outer_body.y,   pos_outer_body.z,   config::visual::radius_outer_body };
        const float obj3[4] = { pos_gateway_node.x, pos_gateway_node.y, pos_gateway_node.z, config::visual::radius_gateway_node };

        bgfx::setUniform(s_uniform_obj1, obj1);
        bgfx::setUniform(s_uniform_obj2, obj2);
        bgfx::setUniform(s_uniform_obj3, obj3);

        // ------------------------------------------------------------------
        // PASS 1: fullscreen raymarch (lensing / accretion)
        // ------------------------------------------------------------------
        float identity_matrix[16];
        bx::mtxIdentity(identity_matrix);
        bgfx::setTransform(identity_matrix);
        bgfx::setVertexBuffer(0, m_fullscreenVbh);
        bgfx::setState(BGFX_STATE_WRITE_RGB | BGFX_STATE_WRITE_A);
        bgfx::submit(0, uniform_agent_program);

        if (!bgfx::isValid(m_particle_program)
            || !bgfx::isValid(s_asteroid_vbh)
            || !bgfx::isValid(s_chassis_vbh)) {
            return;
        }

        // ------------------------------------------------------------------
        // PASS 2: hardware instancing from SoA
        //
        // Asteroids are a dense packed range [4, 4+N). The old code allocated
        // a uint64 index vector of {4,5,6,...} every frame — a heap hit plus
        // an extra indirection for a contiguous span. Walk the range directly.
        //
        // Ships are sparse: compact active ADAPTIVE units into a thread_local
        // scratch vector (zero heap traffic after the first frame).
        // ------------------------------------------------------------------
        const uint64_t num_asteroids = config::simulation::total_asteroids;
        const uint64_t asteroid_begin = kFixedBodyCount;
        const uint64_t ship_begin = kFixedBodyCount + num_asteroids;

        // 3 sincos per frame, not 3 per asteroid. id-phase is constant and cached.
        const float st0 = bx::sin(current_time * 0.2f), ct0 = bx::cos(current_time * 0.2f);
        const float st1 = bx::sin(current_time * 0.3f), ct1 = bx::cos(current_time * 0.3f);
        const float st2 = bx::sin(current_time * 0.1f), ct2 = bx::cos(current_time * 0.1f);

        const AsteroidPhase* RC_RESTRICT phases =
            refresh_asteroid_phases(rb, asteroid_begin, num_asteroids);

        submit_instance_batches(
            1, m_particle_program, s_asteroid_vbh, s_asteroid_ibh,
            static_cast<uint32_t>(num_asteroids),
            [&](InstanceData* dst, uint32_t offset, uint32_t batch) {
                const uint64_t base = asteroid_begin + offset;
                for (uint32_t k = 0; k < batch; ++k) {
                    const uint64_t        i = base + k;
                    const AsteroidPhase& p = phases[offset + k];

                    float sx, cx, sy, cy, sz, cz;
                    angle_add(st0, ct0, p.s0, p.c0, sx, cx);
                    angle_add(st1, ct1, p.s1, p.c1, sy, cy);
                    angle_add(st2, ct2, p.s2, p.c2, sz, cz);

                    mtx_srt_uniform_sc(dst[k].mtx, p.scale,
                        sx, cx, sy, cy, sz, cz,
                        rb.pos_x[i], rb.pos_y[i], rb.pos_z[i]);
					//visible asteroids are cyan, invisible ones are black (alpha=0)
                    dst[k].abgr = pack_abgr(0u, 220u, 255u, 255u);
                    //dst[k].abgr = pack_abgr(rb.color_r[i], rb.color_g[i], rb.color_b[i], rb.color_a[i]);
                }
            });

        auto& ships = ship_scratch();
        ships.clear();
        if (ships.capacity() < config::simulation::total_ships) {
            ships.reserve(config::simulation::total_ships);
        }

        for (uint64_t i = ship_begin; i < capacity; ++i) {
            if (rb.is_active[i] == 0) {
                continue;
            }
            if (static_cast<AgentType>(rb.agent_type[i]) == AgentType::ADAPTIVE) {
                ships.push_back(i);
            }
        }

        const uint32_t ship_count = static_cast<uint32_t>(ships.size());
        if (ship_count == 0) {
            return;
        }

        const uint64_t* RC_RESTRICT ship_ids = ships.data();
        submit_instance_batches(
            1, m_particle_program, s_chassis_vbh, s_chassis_ibh,
            ship_count,
            [&](InstanceData* dst, uint32_t offset, uint32_t batch) {
                for (uint32_t k = 0; k < batch; ++k) {
                    const uint64_t i = ship_ids[offset + k];

                    bx::Vec3 fwd{ 0.0f, 0.0f, 1.0f };
                    bx::Vec3 right{ 1.0f, 0.0f, 0.0f };
                    bx::Vec3 upv{ 0.0f, 1.0f, 0.0f };

                    velocity_basis(rb.vel_x[i], rb.vel_y[i], rb.vel_z[i], fwd, right, upv);
                    write_scaled_basis(dst[k].mtx, right, upv, fwd, 0.6f,
                        rb.pos_x[i], rb.pos_y[i], rb.pos_z[i]);
                    dst[k].abgr = pack_abgr(rb.color_r[i], rb.color_g[i], rb.color_b[i], rb.color_a[i]);
                }
            });
    }

} // namespace stellar_agents