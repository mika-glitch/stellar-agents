#include "physics_evolution.h"
#include <cmath>
#include <algorithm>

// External linkages targeting our decoupled CADS functional cores
namespace stellar_agents {
    extern void MutatePassiveAgentState(const AgentState&, AgentState&, const Vector3&, float) noexcept;
    extern void MutateAdaptiveAgentState(const AgentState&, AgentState&, const Vector3&, const Vector3&, const Vector3&, float, float, float) noexcept;
    extern Vector3 CalculateAttractorFieldAcceleration(const AgentState&, const Vector3&, float) noexcept;
    extern bool EvaluateAbsorptionHorizonThreshold(const AgentState&, const Vector3&, float) noexcept;
}

stellar_agents::PhysicsEvolution::PhysicsEvolution() noexcept {
    const int32_t available_hardware_cores = static_cast<int32_t>(std::thread::hardware_concurrency());
    execution_worker_pool.reserve(available_hardware_cores);
}

// Destructor pass guarantees graceful thread pool wind-down before system deallocation
stellar_agents::PhysicsEvolution::~PhysicsEvolution() {
    {
        std::lock_guard<std::mutex> lock(pipeline_mutex);
        termination_signaled = true;
    }
    condition_start.notify_all();
    // jthreads automatically invoke stop tokens and join on destruction context
}

void stellar_agents::PhysicsEvolution::execute_asynchronous_tick(float p_delta_time, EnvironmentMatrix& p_matrix) noexcept {
    // Dynamic bootstrap pass: Delayed initialization linking the matrix memory block on the first frame
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

    // Simultaneously awaken the asleep core worker topology
    condition_start.notify_all();

    // Safe master join barrier: Put the main application thread to sleep while workers compute
    std::unique_lock<std::mutex> lock(pipeline_mutex);
    condition_end.wait(lock, [this] { return counter_active_workers == 0; });

    // Zero-Lock buffer swap: Instantly flip the matured states to the readable register
    p_matrix.swap_evolutionary_buffers();
}

// ============================================================================
// DECOUPLED WORKER TASK PROCESSING LOOP
// Divided into uniform contiguous chunks running with ZERO locking overhead.
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

    // Direct interface references bounding the flat vector arrays
    const auto& readable_source = p_matrix.get_read_buffer();
    auto& writable_target = p_matrix.get_write_buffer();

    while (!p_token.stop_requested()) {
        std::unique_lock<std::mutex> lock(pipeline_mutex);
        condition_start.wait(lock, [this, synchronized_frame_ticket, &p_token] {
            return current_frame_ticket > synchronized_frame_ticket || termination_signaled || p_token.stop_requested();
            });

        if (termination_signaled || p_token.stop_requested()) [[unlikely]] break;
        synchronized_frame_ticket = current_frame_ticket;

        // Register-cache shared system variables into thread-isolated stacks
        const float dt = atomic_delta_time;
        const float runTime = total_execution_time;
        const Vector3 h_pos = cached_attractor_position;
        const float h_mass = cached_attractor_mass;
        const float h_horiz = cached_event_horizon;
        const Vector3 p_pos = cached_target_planet_position;
        lock.unlock(); // BLOCK-FREE CONCURRENCY CONTEXT ENGAGED: MUTEX UNLOCKED

        // Hard unrolled stream sweep traversing the isolated memory chunk
        for (uint64_t i = segment_start_idx; i < segment_end_idx; ++i) {
            const AgentState& current_agent = readable_source[i];
            AgentState& target_agent = writable_target[i];

            if (!current_agent.is_active) {
                target_agent.is_active = false;
                continue;
            }

            // 1. Evaluate environmental boundary checks (Singularity absorption gate)
            if (EvaluateAbsorptionHorizonThreshold(readable_source[0], current_agent.position, h_horiz)) [[unlikely]] {
                target_agent.is_active = false;
                continue;
            }

            // 2. Extract multi-body gravity acceleration field potential vectors
            const Vector3 accumulated_gravity = CalculateAttractorFieldAcceleration(readable_source[0], current_agent.position, h_mass);

            // 3. Functional behavioral core routing based on the flat agent taxonomy token
            if (current_agent.type == AgentType::PASSIVE) {
                MutatePassiveAgentState(current_agent, target_agent, accumulated_gravity, dt);
            }
            else if (current_agent.type == AgentType::ADAPTIVE) {
                MutateAdaptiveAgentState(current_agent, target_agent, accumulated_gravity, h_pos, p_pos, h_horiz, runTime, dt);
            }
            else if (current_agent.type == AgentType::ATTRACTOR) {
                // Stars/Singularities remain deterministic phase-space anchors across frames
                target_agent = current_agent;
            }
        }

        // Thread sync checkpoint: Decrement master execution counter safely
        std::lock_guard<std::mutex> sync_lock(pipeline_mutex);
        --counter_active_workers;
        if (counter_active_workers == 0) {
            condition_end.notify_one();
        }
    }
}
