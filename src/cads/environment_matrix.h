// ============================================================================
// [Core/Memory] ENVIRONMENT MATRIX CONTROLLER (STRUCTURE OF ARRAYS)
// Description: Implements a double-buffered Structure of Arrays (SoA) memory 
//              architecture. Separates homogeneous attributes into parallel 
//              contiguous vectors to optimize L1/L2 cache locality and 
//              enable vector register (SIMD) streaming.
// Standard: ISO C++20
// ============================================================================
#ifndef ENVIRONMENT_MATRIX_H
#define ENVIRONMENT_MATRIX_H

#include "agent_state.h"
#include <vector>
#include <memory>
#include <cstdint>

namespace stellar_agents {

    // ============================================================================
    // STRUCTURE OF ARRAYS (SoA) DATA ARENA CONTAINER
    // Holds parallel contiguous data buffers for all simulation entities.
    // ============================================================================
    struct EnvironmentBuffersSoA {
        // Spatial Kinematic Arrays (Cartesian Coordinates)
        std::vector<float> pos_x;
        std::vector<float> pos_y;
        std::vector<float> pos_z;

        std::vector<float> vel_x;
        std::vector<float> vel_y;
        std::vector<float> vel_z;

        // Visual and Metadata Arrays
        std::vector<uint8_t>  color_r;
        std::vector<uint8_t>  color_g;
        std::vector<uint8_t>  color_b;
        std::vector<uint8_t>  color_a;

        std::vector<uint32_t> agent_id;
        std::vector<uint8_t>  agent_type; // Serialized AgentType enum mapping
        std::vector<uint8_t>  is_active;  // Contiguous byte representation for active states

        // Autonomous Navigation State Channels (FSM)
        std::vector<uint8_t>  agent_state;    // Operational phase identifier (e.g., transit, loiter, return)
        std::vector<float>    behavior_timer; // Temporal accumulator for state transitions
        std::vector<float>    target_pos_x;   // Cached spatial objective X coordinate
        std::vector<float>    target_pos_y;   // Cached spatial objective Y coordinate
        std::vector<float>    target_pos_z;   // Cached spatial objective Z coordinate

        // Resizes all internal parallel arrays to the specified capacity barrier
        void resize(uint64_t capacity) {
            pos_x.resize(capacity);
            pos_y.resize(capacity);
            pos_z.resize(capacity);
            vel_x.resize(capacity);
            vel_y.resize(capacity);
            vel_z.resize(capacity);
            color_r.resize(capacity);
            color_g.resize(capacity);
            color_b.resize(capacity);
            color_a.resize(capacity);
            agent_id.resize(capacity);
            agent_type.resize(capacity);
            is_active.resize(capacity);

            // Autonomous Navigation State Resize Allocation
            agent_state.resize(capacity);
            behavior_timer.resize(capacity);
            target_pos_x.resize(capacity);
            target_pos_y.resize(capacity);
            target_pos_z.resize(capacity);
        }
    };

    class EnvironmentMatrix {
    public:
        // Explicit allocation tracking memory bounds directly on system boot
        explicit EnvironmentMatrix(uint64_t p_initial_capacity) noexcept;

        ~EnvironmentMatrix() = default;

        // Delete copy semantics to prevent accidental massive memory duplicates inside the pipeline
        EnvironmentMatrix(const EnvironmentMatrix&) = delete;
        EnvironmentMatrix& operator=(const EnvironmentMatrix&) = delete;

        // Allow move operations for seamless system setup transport
        EnvironmentMatrix(EnvironmentMatrix&&) noexcept = default;
        EnvironmentMatrix& operator=(EnvironmentMatrix&&) noexcept = default;

        // --- HIGH-PERFORMANCE LOCK-FREE ACCESS INTERFACES ---
        // Read buffer access: Enforced const thread-safety for renderer streaming
        [[nodiscard]] const EnvironmentBuffersSoA& get_read_buffer() const noexcept {
            return *m_read_ptr;
        }

        // Write buffer access: Provides raw target access for physics thread workers
        [[nodiscard]] EnvironmentBuffersSoA& get_write_buffer() noexcept {
            return *m_write_ptr;
        }

        // Infrastructure state telemetry metrics
        [[nodiscard]] uint64_t get_capacity() const noexcept {
            return m_capacity;
        }

        [[nodiscard]] uint64_t get_active_count() const noexcept {
            return m_active_count;
        }

        // Mutator mapping for the Swap-and-Pop atomic frame lifecycle counter
        void set_active_count(uint64_t p_new_count) noexcept {
            m_active_count = p_new_count;
        }

        // ========================================================================
        // ATOMIC AT-BARRIER POINTER EXCHANGE GATE
        // Executes a zero-allocation buffer flip once all threads complete cycles.
        // ========================================================================
        void FlipBuffers() noexcept {
            m_read_ptr.swap(m_write_ptr);
        }

    private:
        uint64_t m_capacity;
        uint64_t m_active_count;

        // Dual isolated Structure of Arrays memory arenas managed via smart pointers
        std::unique_ptr<EnvironmentBuffersSoA> m_read_ptr;
        std::unique_ptr<EnvironmentBuffersSoA> m_write_ptr;
    };

} // namespace stellar_agents

#endif // ENVIRONMENT_MATRIX_H