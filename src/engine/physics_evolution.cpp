#include "physics_evolution.h"
#include "engine_config.h"
#include "raymath.h"
#include <cmath>
#include <algorithm>

namespace stellar_agents {

    // Forward declarations targeting external stateless behavioral core compilation blocks
    extern void MutatePassiveAgentState(const AgentState&, AgentState&, const Vector3&, float) noexcept;
    extern void MutateAdaptiveAgentState(const AgentState&, AgentState&, const Vector3&, const Vector3&, const Vector3&, float, float, float) noexcept;
}

// ============================================================================
// SYSTEM CORE INITIALIZATION PASS
// Reserves persistent worker jthreads aligned to the architecture's hardware limits.
// ============================================================================
stellar_agents::PhysicsEvolution::PhysicsEvolution() noexcept {
    const int32_t available_hardware_cores = static_cast<int32_t>(std::thread::hardware_concurrency());
    execution_worker_pool.reserve(available_hardware_cores);
}

stellar_agents::PhysicsEvolution::~PhysicsEvolution() {
    {
        std::lock_guard<std::mutex> lock(pipeline_mutex);
        termination_signaled = true;
    }
    condition_start.notify_all();
}

// ============================================================================
// ASYNCHRONOUS ENGINE STEP DISPATCHER
// Coordinates background tasks and safe post-frame array layout compression.
// ============================================================================
void stellar_agents::PhysicsEvolution::execute_asynchronous_tick(float p_delta_time, EnvironmentMatrix& p_matrix) noexcept {
    // Lazy instantiation pattern for the persistent worker threads mapping native cores
    if (execution_worker_pool.empty()) [[unlikely]] {
        const size_t core_capacity = execution_worker_pool.capacity();
        for (size_t i = 0; i < core_capacity; ++i) {
            execution_worker_pool.emplace_back([this, i, &p_matrix](std::stop_token p_token) {
                this->worker_thread_execution_loop(p_token, static_cast<int32_t>(i), p_matrix);
                });
        }
    }

    {
        std::lock_guard<std::mutex> lock(pipeline_mutex);
        atomic_delta_time = p_delta_time;
        total_execution_time += p_delta_time;
        counter_active_workers = static_cast<int32_t>(execution_worker_pool.capacity());
        ++current_frame_ticket;
    }

    // Wake up the concurrent worker pool asynchronously
    condition_start.notify_all();

    // Fence execution path: Stall main context until all chunks complete calculation blocks
    std::unique_lock<std::mutex> lock(pipeline_mutex);
    condition_end.wait(lock, [this] { return counter_active_workers == 0; });

    // --- STAGE 2: BARE-METAL STORAGE COMPRESSION ---
    // Synchronous memory clean pass executing Swap-and-Pop before flipping buffers
    ExecuteStorageCompression(p_matrix);

    // Release the processed configuration snapshot straight to the render core
    p_matrix.FlipBuffers();
}

// ============================================================================
// CONCURRENT WORKER INTEGRATION LOOP
// Massively parallel state mutations processing unrolled matrix slices.
// ============================================================================
void stellar_agents::PhysicsEvolution::worker_thread_execution_loop(
    std::stop_token p_token,
    int32_t p_worker_id,
    EnvironmentMatrix& p_matrix) noexcept
{
    const uint64_t core_allocation_count = execution_worker_pool.capacity();
    const uint64_t total_entities = p_matrix.get_capacity();
    const uint64_t memory_chunk_size = total_entities / core_allocation_count;

    const uint64_t segment_start_idx = p_worker_id * memory_chunk_size;
    const uint64_t segment_end_idx = (p_worker_id == core_allocation_count - 1) ? total_entities : segment_start_idx + memory_chunk_size;

    uint64_t synchronized_frame_ticket = 0;

    const auto& readable_source = p_matrix.get_read_buffer();
    auto& writable_target = p_matrix.get_write_buffer();

    while (!p_token.stop_requested()) {
        std::unique_lock<std::mutex> lock(pipeline_mutex);
        condition_start.wait(lock, [this, synchronized_frame_ticket, &p_token] {
            return current_frame_ticket > synchronized_frame_ticket || termination_signaled || p_token.stop_requested();
            });

        if (termination_signaled || p_token.stop_requested()) [[unlikely]] break;
        synchronized_frame_ticket = current_frame_ticket;

        const float dt = atomic_delta_time;
        const float runTime = total_execution_time;

        lock.unlock(); // BLOCK-FREE CONCURRENCY CONTEXT ENGAGED

        // Static system positions resolved straight from compile-time configuration namespaces
        constexpr Vector3 black_hole_origin = { 0.0f, 0.0f, 0.0f };
        const Vector3 core_planet_pos = readable_source[1].position;   // Dynamic Object 1 position track
        const Vector3 giant_planet_pos = readable_source[2].position;  // Dynamic Object 2 position track

        // Streamlined linear cache unrolling pass over the assigned chunk segment
        for (uint64_t i = segment_start_idx; i < segment_end_idx; ++i) {
            const AgentState& current_agent = readable_source[i];
            AgentState& target_agent = writable_target[i];

            if (!current_agent.is_active) {
                target_agent.is_active = false;
                continue;
            }

            // Centralized Event Horizon Threshold Evaluation Pass
            Vector3 rel_to_singularity = Vector3Subtract(black_hole_origin, current_agent.position);
            float distance_squared = Vector3LengthSqr(rel_to_singularity);

            if (distance_squared < (config::physics::rs_horizon * config::physics::rs_horizon)) [[unlikely]] {
                target_agent.is_active = false; // Trigger systemic deallocation marker
                continue;
            }

            // O(1) Analytic Schwarzschild Gravitational Field Potential Calculation
            float gravity_magnitude = config::physics::black_hole_runtime_mass / distance_squared;
            Vector3 accumulated_gravity = Vector3Scale(Vector3Normalize(rel_to_singularity), gravity_magnitude);

            // Behavioral multiplexer forwarding data arrays straight down to stateless transformation cores
            if (current_agent.type == AgentType::PASSIVE) {
                MutatePassiveAgentState(current_agent, target_agent, accumulated_gravity, dt);
            }
            else if (current_agent.type == AgentType::ADAPTIVE) {
                MutateAdaptiveAgentState(current_agent, target_agent, accumulated_gravity, black_hole_origin, core_planet_pos, config::physics::rs_horizon, runTime, dt);
            }
            else if (current_agent.type == AgentType::ATTRACTOR) {
                // Fixed background coordinates remain immutable across frames
                target_agent = current_agent;
            }
        }

        // Notify frame completion barrier cleanly using atomic synchronization registers
        std::lock_guard<std::mutex> sync_lock(pipeline_mutex);
        --counter_active_workers;
        if (counter_active_workers == 0) {
            condition_end.notify_one();
        }
    }
}

// ============================================================================
// BARE-METAL CONTIGUOUS STORAGE COMPRESSION (SWAP-AND-POP O(1) PIPELINE)
// Compresses memory layout instantly post-step calculations to maximize SIMD hits.
// ============================================================================
void stellar_agents::PhysicsEvolution::ExecuteStorageCompression(EnvironmentMatrix& p_matrix) noexcept {
    auto& target_buffer = p_matrix.get_write_buffer();
    uint64_t current_active_total = p_matrix.get_active_count();

    // Skip over fixed system attractors resting at head slots [0, 1, 2, 3] to preserve system geometry
    constexpr uint64_t infrastructure_offset = 4;
    if (current_active_total <= infrastructure_offset) return;

    uint64_t scan_idx = infrastructure_offset;
    uint64_t last_active_idx = current_active_total - 1;

    // Direct, branchless pointer array squeeze pass executing completely inside raw memory blocks
    while (scan_idx <= last_active_idx) {
        if (!target_buffer[scan_idx].is_active) {
            // Find the trailing active edge element from the tail of the array block
            while (last_active_idx > scan_idx && !target_buffer[last_active_idx].is_active) {
                --last_active_idx;
            }

            if (last_active_idx > scan_idx) {
                // Squeeze out the vacancy instantly: Overwrite holes using direct bitwise register assignment
                target_buffer[scan_idx] = target_buffer[last_active_idx];
                target_buffer[scan_idx].agent_id = static_cast<uint32_t>(scan_idx); // Realign structural index tracker
                target_buffer[last_active_idx].is_active = false;
                --last_active_idx;
            }
        }
        ++scan_idx;
    }

    // Flush the optimized layout boundaries down to the Environment Matrix telemetry register
    p_matrix.set_active_count(last_active_idx + 1);

} // namespace stellar_agents
