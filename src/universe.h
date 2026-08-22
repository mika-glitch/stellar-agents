#ifndef UNIVERSE_H
#define UNIVERSE_H

#include <vector>
#include <thread>
#include <mutex>
#include <condition_variable>
#include <future>
#include "raylib.h"
#include "black_hole.h"

namespace godot_hpc { // Clean explicit namespace mapping to block symbol pollution

    struct AsteroidData {
        Vector3 position{ 0.0f, 0.0f, 0.0f };
        Vector3 velocity{ 0.0f, 0.0f, 0.0f };
        Color color{ WHITE };
        bool active{ true };
    };

    class Universe final {
    private:
        uint64_t max_asteroids{ 1000000 };
        float target_orbit_radius{ 2400.0f };

        // ============================================================================
        // THE UNZERSTÖRBAR DOUBLE-BUFFER REGISTRY (ANTI-DATA-RACE OVERRIDE)
        // Threads read from 'current_states' and write ONLY to 'next_states' concurrently.
        // Bypasses the Mutex contention and fully locks down data races!
        // ============================================================================
        std::vector<AsteroidData> current_states;
        std::vector<AsteroidData> next_states;
        std::vector<Vector3> hardware_vertex_buffer;

        // Advanced C++20 Asynchronous Thread Pool Registry
        std::vector<std::jthread> worker_pool; // jthread automatically joins on destruction pass
        std::mutex pool_mutex;
        std::condition_variable cv_start;
        std::condition_variable cv_end;

        int32_t active_workers{ 0 };
        uint64_t frame_ticket{ 0 };
        float atomic_delta_time{ 0.016f };
        bool terminate_simulation{ false };

        // Cached constants to maximize CPU L1/L2 data cache throughput
        Vector3 cached_hole_pos{ 0.0f, 0.0f, 0.0f };
        float cached_hole_mass{ 0.0f };
        float cached_horizon{ 0.0f };

        void worker_thread_execution_loop(std::stop_token p_token, int32_t p_worker_id) noexcept;

    public:
        Universe(uint64_t p_max_asteroids, float p_orbit_radius) noexcept;
        ~Universe();

        // Disallow compiler copying to protect strict multithreaded raw buffers
        Universe(const Universe&) = delete;
        Universe& operator=(const Universe&) = delete;

        void update_asynchronous_physics(float p_delta_time, const BlackHole& p_black_hole) noexcept;
        void render_hardware_vertex_buffers(const Vector3& p_cam_pos) noexcept;

        [[nodiscard]] uint64_t get_active_survivors() const noexcept;
    };

} // namespace godot_hpc

#endif // UNIVERSE_H
