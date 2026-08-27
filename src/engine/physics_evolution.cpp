// ============================================================================
// [Core/Physics] PHYSICS EVOLUTION IMPLEMENTATION (STRUCTURE OF ARRAYS)
// Description: Multi-threaded asynchronous physics simulation pipeline utilizing 
//              a Structure of Arrays (SoA) memory layout for optimal vector 
//              register streaming and L1/L2 cache locality. Enforces explicit 
//              kinematic integration for orbital trajectory propagation.
// Standard: ISO C++20
// ============================================================================
#include "physics_evolution.h"
#include "engine_config.h"
#include <cmath>
#include <algorithm>

namespace stellar_agents {

    // ============================================================================
    // EXTERNAL BEHAVIORAL CORE MUTATORS (SoA PIPELINE HOOKS)
    // ============================================================================
    extern void MutateAttractorAgentSoA(EnvironmentBuffersSoA& buffer, uint64_t i, float execution_time) noexcept;
    extern void MutatePassiveAgentSoA(EnvironmentBuffersSoA& buffer, uint64_t i, float field_acc_x, float field_acc_y, float field_acc_z, float delta_time) noexcept;
    extern void MutateAdaptiveAgentSoA(EnvironmentBuffersSoA& buffer, uint64_t i, float field_acc_x, float field_acc_y, float field_acc_z, float target_planet_x, float target_planet_y, float target_planet_z, float execution_time, float delta_time) noexcept;

    // ============================================================================
    // SYSTEM CORE INITIALIZATION PASS
    // ============================================================================
    PhysicsEvolution::PhysicsEvolution() noexcept {
        const size_t available_hardware_cores = std::max<size_t>(1, std::thread::hardware_concurrency());
        execution_worker_pool.reserve(available_hardware_cores);
    }

    PhysicsEvolution::~PhysicsEvolution() {
        {
            std::lock_guard<std::mutex> lock(pipeline_mutex);
            termination_signaled = true;
        }
        condition_start.notify_all();
    }

    // ============================================================================
    // ASYNCHRONOUS ENGINE STEP DISPATCHER
    // ============================================================================
    void PhysicsEvolution::execute_asynchronous_tick(float p_delta_time, EnvironmentMatrix& p_matrix) noexcept {
        if (execution_worker_pool.empty()) [[unlikely]] {
            const size_t core_count = std::max<size_t>(1, std::thread::hardware_concurrency());
            execution_worker_pool.clear();
            execution_worker_pool.reserve(core_count);
            for (size_t i = 0; i < core_count; ++i) {
                execution_worker_pool.emplace_back([this, worker_id = static_cast<int32_t>(i), &p_matrix](std::stop_token p_token) {
                    this->worker_thread_execution_loop(p_token, worker_id, p_matrix);
                    });
            }
        }

        {
            std::lock_guard<std::mutex> lock(pipeline_mutex);
            atomic_delta_time = p_delta_time;
            total_execution_time += p_delta_time;
            counter_active_workers = static_cast<int32_t>(execution_worker_pool.size());
            ++current_frame_ticket;
        }

        condition_start.notify_all();

        std::unique_lock<std::mutex> lock(pipeline_mutex);
        condition_end.wait(lock, [this] { return counter_active_workers == 0; });

        // --- STAGE 2: BARE-METAL STORAGE COMPRESSION (DENSE PACKING) ---
        ExecuteStorageCompression(p_matrix);

        p_matrix.FlipBuffers();
    }

    // ============================================================================
    // CONCURRENT WORKER THREAD EXECUTION LOOP (Structure of Arrays Iteration)
    // ============================================================================
    void PhysicsEvolution::worker_thread_execution_loop(
        std::stop_token p_token,
        int32_t p_worker_id,
        EnvironmentMatrix& p_matrix) noexcept
    {
        const uint64_t core_allocation_count = std::max<uint64_t>(1, execution_worker_pool.size());
        const uint64_t total_entities = p_matrix.get_capacity();
        const uint64_t memory_chunk_size = total_entities / core_allocation_count;

        const uint64_t segment_start_idx = p_worker_id * memory_chunk_size;
        const uint64_t segment_end_idx = (p_worker_id == static_cast<int32_t>(core_allocation_count - 1)) ? total_entities : segment_start_idx + memory_chunk_size;

        uint64_t synchronized_frame_ticket = 0;

        while (!p_token.stop_requested()) {
            std::unique_lock<std::mutex> lock(pipeline_mutex);
            condition_start.wait(lock, [this, synchronized_frame_ticket, &p_token] {
                return current_frame_ticket > synchronized_frame_ticket || termination_signaled || p_token.stop_requested();
                });

            if (termination_signaled || p_token.stop_requested()) [[unlikely]] break;
            synchronized_frame_ticket = current_frame_ticket;

            const float dt = atomic_delta_time;
            const float run_time = total_execution_time;

            lock.unlock();

            const auto& readable_source = p_matrix.get_read_buffer();
            auto& writable_target = p_matrix.get_write_buffer();

            // Extract reference position of the inner dense attractor node for spatial AI targeting
            const float inner_attractor_x = total_entities > 1 ? readable_source.pos_x[1] : 0.0f;
            const float inner_attractor_y = total_entities > 1 ? readable_source.pos_y[1] : 0.0f;
            const float inner_attractor_z = total_entities > 1 ? readable_source.pos_z[1] : 0.0f;

            for (uint64_t i = segment_start_idx; i < segment_end_idx; ++i) {
                // Copy baseline attributes from read buffer to write buffer[cite: 8]
                writable_target.pos_x[i] = readable_source.pos_x[i];
                writable_target.pos_y[i] = readable_source.pos_y[i];
                writable_target.pos_z[i] = readable_source.pos_z[i];

                writable_target.vel_x[i] = readable_source.vel_x[i];
                writable_target.vel_y[i] = readable_source.vel_y[i];
                writable_target.vel_z[i] = readable_source.vel_z[i];

                writable_target.color_r[i] = readable_source.color_r[i];
                writable_target.color_g[i] = readable_source.color_g[i];
                writable_target.color_b[i] = readable_source.color_b[i];
                writable_target.color_a[i] = readable_source.color_a[i];

                writable_target.agent_id[i] = readable_source.agent_id[i];
                writable_target.agent_type[i] = readable_source.agent_type[i];
                writable_target.is_active[i] = readable_source.is_active[i];

                // FSM memory channel state synchronization[cite: 8]
                writable_target.agent_state[i] = readable_source.agent_state[i];
                writable_target.behavior_timer[i] = readable_source.behavior_timer[i];
                writable_target.target_pos_x[i] = readable_source.target_pos_x[i];
                writable_target.target_pos_y[i] = readable_source.target_pos_y[i];
                writable_target.target_pos_z[i] = readable_source.target_pos_z[i];

                if (readable_source.is_active[i] == 0) {
                    writable_target.is_active[i] = 0;
                    continue;
                }

                // Handle system attractor nodes (Planetary Keplerian Rails and Central Barycenter)[cite: 8]
                if (static_cast<AgentType>(readable_source.agent_type[i]) == AgentType::ATTRACTOR) {
                    MutateAttractorAgentSoA(writable_target, i, run_time);
                    continue;
                }

                const float px = readable_source.pos_x[i];
                const float py = readable_source.pos_y[i];
                const float pz = readable_source.pos_z[i];

                const float distance_squared = (px * px) + (py * py) + (pz * pz);

                /**
                 * Schwarzschild Event Horizon Culling Trigger[cite: 8]
                 * Formula: $r^2 \le r_s^2$, where $r_s = \frac{2GM}{c^2}$
                 */
                if (distance_squared < (config::physics::rs_horizon * config::physics::rs_horizon)) [[unlikely]] {
                    writable_target.is_active[i] = 0;
                    continue;
                }

                /**
                 * Newtonian Gravitational Field Acceleration Vector[cite: 8]
                 * Formula: $\vec{a} = -\frac{G \cdot M}{|\vec{r}|^3} \vec{r}$
                 */
                const float distance = std::sqrt(distance_squared);
                const float gravity_magnitude = config::physics::primary_attractor_runtime_mass / (distance_squared + 0.0001f);

                const float norm_x = -px / (distance + 0.00001f);
                const float norm_y = -py / (distance + 0.00001f);
                const float norm_z = -pz / (distance + 0.00001f);

                const float accumulated_gravity_x = norm_x * gravity_magnitude;
                const float accumulated_gravity_y = norm_y * gravity_magnitude;
                const float accumulated_gravity_z = norm_z * gravity_magnitude;

                const AgentType current_type = static_cast<AgentType>(readable_source.agent_type[i]);
                if (current_type == AgentType::PASSIVE) {
                    MutatePassiveAgentSoA(writable_target, i, accumulated_gravity_x, accumulated_gravity_y, accumulated_gravity_z, dt);
                }
                else if (current_type == AgentType::ADAPTIVE) {
                    MutateAdaptiveAgentSoA(writable_target, i, accumulated_gravity_x, accumulated_gravity_y, accumulated_gravity_z, inner_attractor_x, inner_attractor_y, inner_attractor_z, run_time, dt);
                }

                // ====================================================================
                // KINEMATIC INTEGRATION STEP (EXPLICIT EULER-CROMER)[cite: 8]
                // Updates velocity vector and spatial position over delta time step.
                // Formula: $\vec{v}_{new} = \vec{v}_old + \vec{a} \cdot dt$
                // Formula: $\vec{p}_{new} = \vec{p}_old + \vec{v}_new \cdot dt$
                // ====================================================================
                writable_target.vel_x[i] += accumulated_gravity_x * dt;
                writable_target.vel_y[i] += accumulated_gravity_y * dt;
                writable_target.vel_z[i] += accumulated_gravity_z * dt;

                writable_target.pos_x[i] += writable_target.vel_x[i] * dt;
                writable_target.pos_y[i] += writable_target.vel_y[i] * dt;
                writable_target.pos_z[i] += writable_target.vel_z[i] * dt;
            }

            std::lock_guard<std::mutex> sync_lock(pipeline_mutex);
            --counter_active_workers;
            if (counter_active_workers == 0) {
                condition_end.notify_one();
            }
        }
    }

    // ============================================================================
    // BARE-METAL CONTIGUOUS STORAGE COMPRESSION (SWAP-AND-POP O(1) PIPELINE)
    // Description: Compacts active records across parallel SoA vector arrays, 
    //              eliminating fragmentation without dynamic reallocations.
    // ============================================================================
    void PhysicsEvolution::ExecuteStorageCompression(EnvironmentMatrix& p_matrix) noexcept {
        auto& target_buffer = p_matrix.get_write_buffer();
        uint64_t current_active_total = p_matrix.get_active_count();

        constexpr uint64_t infrastructure_offset = 4;
        if (current_active_total <= infrastructure_offset) {
            p_matrix.set_active_count(infrastructure_offset);
            return;
        }

        uint64_t scan_idx = infrastructure_offset;
        uint64_t last_active_idx = current_active_total - 1;

        while (scan_idx <= last_active_idx && scan_idx < target_buffer.pos_x.size() && last_active_idx < target_buffer.pos_x.size()) {

            if (scan_idx < infrastructure_offset) {
                scan_idx = infrastructure_offset;
            }

            if (target_buffer.is_active[scan_idx] == 0) {
                while (last_active_idx > scan_idx && last_active_idx >= infrastructure_offset && target_buffer.is_active[last_active_idx] == 0) {
                    --last_active_idx;
                }

                if (last_active_idx < infrastructure_offset || last_active_idx <= scan_idx) {
                    break;
                }

                std::swap(target_buffer.pos_x[scan_idx], target_buffer.pos_x[last_active_idx]);
                std::swap(target_buffer.pos_y[scan_idx], target_buffer.pos_y[last_active_idx]);
                std::swap(target_buffer.pos_z[scan_idx], target_buffer.pos_z[last_active_idx]);

                std::swap(target_buffer.vel_x[scan_idx], target_buffer.vel_x[last_active_idx]);
                std::swap(target_buffer.vel_y[scan_idx], target_buffer.vel_y[last_active_idx]);
                std::swap(target_buffer.vel_z[scan_idx], target_buffer.vel_z[last_active_idx]);

                std::swap(target_buffer.color_r[scan_idx], target_buffer.color_r[last_active_idx]);
                std::swap(target_buffer.color_g[scan_idx], target_buffer.color_g[last_active_idx]);
                std::swap(target_buffer.color_b[scan_idx], target_buffer.color_b[last_active_idx]);
                std::swap(target_buffer.color_a[scan_idx], target_buffer.color_a[last_active_idx]);

                std::swap(target_buffer.agent_id[scan_idx], target_buffer.agent_id[last_active_idx]);
                std::swap(target_buffer.agent_type[scan_idx], target_buffer.agent_type[last_active_idx]);
                std::swap(target_buffer.is_active[scan_idx], target_buffer.is_active[last_active_idx]);

                std::swap(target_buffer.agent_state[scan_idx], target_buffer.agent_state[last_active_idx]);
                std::swap(target_buffer.behavior_timer[scan_idx], target_buffer.behavior_timer[last_active_idx]);
                std::swap(target_buffer.target_pos_x[scan_idx], target_buffer.target_pos_x[last_active_idx]);
                std::swap(target_buffer.target_pos_y[scan_idx], target_buffer.target_pos_y[last_active_idx]);
                std::swap(target_buffer.target_pos_z[scan_idx], target_buffer.target_pos_z[last_active_idx]);

                target_buffer.agent_id[scan_idx] = static_cast<uint32_t>(scan_idx);
                target_buffer.is_active[last_active_idx] = 0;
                --last_active_idx;
            }
            ++scan_idx;
        }

        uint64_t final_count = last_active_idx + 1;
        p_matrix.set_active_count(std::max(final_count, infrastructure_offset));
    }

} // namespace stellar_agents