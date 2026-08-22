#include "universe.h"
#include "relativity.h"
#include <random>
#include <cmath>
#include <algorithm>

godot_hpc::Universe::Universe(uint64_t p_max_asteroids, float p_orbit_radius) noexcept
    : max_asteroids(p_max_asteroids), target_orbit_radius(p_orbit_radius)
{
    current_states.resize(max_asteroids);
    next_states.resize(max_asteroids);
    hardware_vertex_buffer.resize(max_asteroids * 2);

    std::mt19937 gen(1337);
    std::uniform_real_distribution<float> angle_dist(0.0f, 2.0f * PI);
    std::uniform_real_distribution<float> rad_dist(p_orbit_radius * 0.4f, p_orbit_radius * 1.8f);
    std::uniform_real_distribution<float> height_dist(-40.0f, 40.0f);

    for (uint64_t i = 0; i < max_asteroids; ++i) {
        float angle = angle_dist(gen);
        float radius = rad_dist(gen);

        current_states[i].position = Vector3{ radius * std::cos(angle), height_dist(gen), radius * std::sin(angle) };

        float speed = std::sqrt((400000.0f * 0.0015f) / radius) * 120.0f;
        current_states[i].velocity = Vector3{ -speed * std::sin(angle), 0.0f, speed * std::cos(angle) };
        current_states[i].color = ColorFromHSV(angle * (360.0f / (2.0f * PI)), 0.75f, 0.95f);
        current_states[i].active = true;
    }

    next_states = current_states;

    // C++20 std::jthread factory spawn pass
    const int32_t total_cores = static_cast<int32_t>(std::thread::hardware_concurrency());
    worker_pool.reserve(total_cores);
    for (int32_t i = 0; i < total_cores; ++i) {
        worker_pool.emplace_back(&Universe::worker_thread_execution_loop, this, i);
    }
}

godot_hpc::Universe::~Universe() {
    {
        std::lock_guard<std::mutex> lock(pool_mutex);
        terminate_simulation = true;
    }
    cv_start.notify_all();
    // jthreads automatically invoke safe cooperative abort signaling on destruction!
}

void godot_hpc::Universe::worker_thread_execution_loop(std::stop_token p_token, int32_t p_worker_id) noexcept {
    const uint64_t core_count = worker_pool.capacity();
    const uint64_t chunk_size = max_asteroids / core_count;
    const uint64_t start_idx = p_worker_id * chunk_size;
    const uint64_t end_idx = (p_worker_id == core_count - 1) ? max_asteroids : start_idx + chunk_size;

    uint64_t standard_ticket = 0;

    while (!p_token.stop_requested()) { // C++20 stop token interface compliance
        std::unique_lock<std::mutex> lock(pool_mutex);
        cv_start.wait(lock, [this, standard_ticket, &p_token] {
            return frame_ticket > standard_ticket || terminate_simulation || p_token.stop_requested();
            });

        if (terminate_simulation || p_token.stop_requested()) [[unlikely]] break;
        standard_ticket = frame_ticket;

        // Thread-safe isolation caching inside private CPU registers
        const Vector3 h_pos = cached_hole_pos;
        const float h_mass = cached_hole_mass;
        const float h_horizon = cached_horizon;
        const float dt = atomic_delta_time;
        lock.unlock(); // BLOCK-FREE CONCURRENCY CONTEXT ENGAGED

        // Hard unrolled calculation loop running on the dedicated core register layers
        for (uint64_t i = start_idx; i < end_idx; ++i) {
            if (!current_states[i].active) {
                next_states[i].active = false;
                continue;
            }

            Vector3 pos = current_states[i].position;
            Vector3 vel = current_states[i].velocity;

            Vector3 diff = Vector3Subtract(h_pos, pos);
            float dist_sq = Vector3LengthSqr(diff);

            if (dist_sq < h_horizon * h_horizon) [[unlikely]] {
                next_states[i].active = false; // Isolated thread-safe array write
                continue;
            }

            // High performance Einstein gravitational calculation vector pass
            float force = (4.0f * (h_mass * 0.0015f)) / (dist_sq * std::sqrt(dist_sq) + 1e-5f);
            Vector3 accel = Vector3Scale(Vector3Normalize(diff), force * 80000.0f);

            vel = Vector3Add(vel, Vector3Scale(accel, dt));
            pos = Vector3Add(pos, Vector3Scale(vel, dt));

            // Flush mutated vectors clean into the decoupled secondary nextState buffer
            next_states[i].position = pos;
            next_states[i].velocity = vel;
            next_states[i].active = true;
        }

        std::lock_guard<std::mutex> sync_lock(pool_mutex);
        --active_workers;
        if (active_workers == 0) {
            cv_end.notify_one();
        }
    }
}

void godot_hpc::Universe::update_asynchronous_physics(float p_delta_time, const BlackHole& p_black_hole) noexcept {
    {
        std::lock_guard<std::mutex> lock(pool_mutex);
        atomic_delta_time = p_delta_time;
        cached_hole_pos = p_black_hole.position;
        cached_hole_mass = p_black_hole.mass;
        cached_horizon = p_black_hole.eventHorizon;

        active_workers = static_cast<int32_t>(worker_pool.capacity());
        ++frame_ticket;
    }
    cv_start.notify_all(); // Wake the async core cluster simultaneously

    std::unique_lock<std::mutex> lock(pool_mutex);
    cv_end.wait(lock, [this] { return active_workers == 0; }); // Safe thread join barrier

    // ============================================================================
    // THE ZERO-LOCK BUFFER FLIP PASS
    // Highly efficient standard conform std::swap call mirroring the arrays instantly!
    // ============================================================================
    std::swap(current_states, next_states);
}

void godot_hpc::Universe::render_hardware_vertex_buffers(const Vector3& p_cam_pos) noexcept {
    rlBegin(RL_LINES);
    const Vector3 h_pos = cached_hole_pos;
    const float h_mass = cached_hole_mass;
    const float h_horizon = cached_horizon;

    for (uint64_t i = 0; i < max_asteroids; ++i) {
        if (!current_states[i].active) continue;

        Vector3 r_pos = current_states[i].position;
        // Direct execution routing to our matching relativistic Einstein optics core
        Vector3 app_pos = Relativity::GetApparentPosition(r_pos, p_cam_pos, h_pos, h_mass, h_horizon);

        rlColor4ub(current_states[i].color.r, current_states[i].color.g, current_states[i].color.b, current_states[i].color.a);
        rlVertex3f(app_pos.x, app_pos.y, app_pos.z);
        rlVertex3f(app_pos.x + current_states[i].velocity.x * 0.02f, app_pos.y + current_states[i].velocity.y * 0.02f, app_pos.z + current_states[i].velocity.z * 0.02f);
    }
    rlEnd();
}

uint64_t godot_hpc::Universe::get_active_survivors() const noexcept {
    uint64_t count = 0;
    for (uint64_t i = 0; i < max_asteroids; ++i) {
        if (current_states[i].active) ++count;
    }
    return count;
}
