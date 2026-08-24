#ifndef PHYSICS_EVOLUTION_H
#define PHYSICS_EVOLUTION_H

#include "environment_matrix.h"
#include <vector>
#include <thread>
#include <mutex>
#include <condition_variable>

namespace stellar_agents {

    // ============================================================================
    // CONCURRENT PHYSICS EVOLUTION SUBSYSTEM
    // Orchestrates a low-overhead asynchronous thread pool driving parallel state mutations.
    // Leverages C++20 cooperative lifetimes (std::jthread) to suppress allocation thrashing.
    // Modified to execute contiguous storage compression via direct Swap-and-Pop.
    // ============================================================================
    class PhysicsEvolution final {
    private:
        // Lifecycle metrics and synchronization fences mapping native hardware threads
        std::vector<std::jthread> execution_worker_pool;
        std::mutex pipeline_mutex;
        std::condition_variable condition_start;
        std::condition_variable condition_end;

        // Microscopic atomic state controllers driving the synchronization cycles
        int32_t counter_active_workers{ 0 };
        uint64_t current_frame_ticket{ 0 };
        float atomic_delta_time{ 0.016f };
        float total_execution_time{ 0.0f };
        bool termination_signaled{ false };

        // Core worker loop processing dedicated memory segments concurrently across threads
        void worker_thread_execution_loop(std::stop_token p_token, int32_t p_worker_id, EnvironmentMatrix& p_matrix) noexcept;

        // ========================================================================
        // BARE-METAL CONTIGUOUS STORAGE COMPRESSION (SWAP-AND-POP O(1) GC)
        // Re-aligns flat vector streams post-frame execution to eliminate memory gaps.
        // ========================================================================
        void ExecuteStorageCompression(EnvironmentMatrix& p_matrix) noexcept;

    public:
        explicit PhysicsEvolution() noexcept;
        ~PhysicsEvolution();

        // Prevent unsafe compilation copy operations over concurrent thread boundaries
        PhysicsEvolution(const PhysicsEvolution&) = delete;
        PhysicsEvolution& operator=(const PhysicsEvolution&) = delete;
        PhysicsEvolution(PhysicsEvolution&&) noexcept = delete;
        PhysicsEvolution& operator=(PhysicsEvolution&&) noexcept = delete;

        // ============================================================================
        // CONCURRENT WORKLOAD DISPATCH PASS
        // Synchronizes the thread barriers and wakes the core pool simultaneously.
        // ============================================================================
        void execute_asynchronous_tick(float p_delta_time, EnvironmentMatrix& p_matrix) noexcept;
    };

} // namespace stellar_agents

#endif // PHYSICS_EVOLUTION_H
