// ============================================================================
// [Core/Memory] ENVIRONMENT MATRIX IMPLEMENTATION (STRUCTURE OF ARRAYS)
// Description: Manages raw memory reservation and structural initialization 
//              for parallel vector data blocks.
// Standard: ISO C++20
// ============================================================================
#include "environment_matrix.h"

namespace stellar_agents {

    // ============================================================================
    // ENVIRONMENT MATRIX CONSTRUCTOR
    // Executes a one-time, strict raw memory reservation loop on application boot.
    // Preserves cache line coherence by packing homogeneous arrays side-by-side.
    // ============================================================================
    EnvironmentMatrix::EnvironmentMatrix(uint64_t p_initial_capacity) noexcept
        : m_capacity(p_initial_capacity)
        , m_active_count(p_initial_capacity)
    {
        // 1. Instantiate flat, contiguous SoA buffer arenas
        m_read_ptr = std::make_unique<EnvironmentBuffersSoA>();
        m_write_ptr = std::make_unique<EnvironmentBuffersSoA>();

        // ========================================================================
        // CRITICAL HARDWARE PASS: HEAP CAPACITY RESERVATION
        // Enforces structural array padding up to the max density perimeter boundaries.
        // Prevents dynamic re-allocations and pointer validation breaks inside the loops.
        // ========================================================================
        m_read_ptr->resize(m_capacity);
        m_write_ptr->resize(m_capacity);

        // Pre-initialize basic fields for structural identity protection and deterministic FSM states
        for (uint64_t i = 0; i < m_capacity; ++i) {
            // Identity and Lifecycle
            m_read_ptr->agent_id[i] = static_cast<uint32_t>(i);
            m_read_ptr->is_active[i] = 0;

            m_write_ptr->agent_id[i] = static_cast<uint32_t>(i);
            m_write_ptr->is_active[i] = 0;

            // Autonomous FSM Navigation Channels (Zero-Initialization)
            m_read_ptr->agent_state[i] = 0;
            m_read_ptr->behavior_timer[i] = 0.0f;
            m_read_ptr->target_pos_x[i] = 0.0f;
            m_read_ptr->target_pos_y[i] = 0.0f;
            m_read_ptr->target_pos_z[i] = 0.0f;

            m_write_ptr->agent_state[i] = 0;
            m_write_ptr->behavior_timer[i] = 0.0f;
            m_write_ptr->target_pos_x[i] = 0.0f;
            m_write_ptr->target_pos_y[i] = 0.0f;
            m_write_ptr->target_pos_z[i] = 0.0f;
        }
    }

} // namespace stellar_agents