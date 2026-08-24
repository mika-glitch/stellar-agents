#include "environment_matrix.h"

namespace stellar_agents {

    // ============================================================================
    // CADS ENVIRONMENT MATRIX CONSTRUCTOR
    // Executes a one-time, strict raw memory reservation loop on application boot.
    // Preserves cache line coherence by packing arrays side-by-side in global RAM.
    // ============================================================================
    EnvironmentMatrix::EnvironmentMatrix(uint64_t p_initial_capacity) noexcept
        : m_capacity(p_initial_capacity)
        , m_active_count(p_initial_capacity)
    {
        // 1. Instantiate flat, contiguous array heap vectors
        m_read_ptr = std::make_unique<std::vector<AgentState>>();
        m_write_ptr = std::make_unique<std::vector<AgentState>>();

        // ========================================================================
        // CRITICAL HARDWARE PASS: HEAP CAPACITY RESERVATION
        // Enforces structural array padding up to the max density perimeter boundaries.
        // Prevents dynamic re-allocations and pointer validation breaks inside the loops.
        // ========================================================================
        m_read_ptr->resize(m_capacity);
        m_write_ptr->resize(m_capacity);

        // Pre-initialize basic fields for structural identity protection
        for (uint64_t i = 0; i < m_capacity; ++i) {
            (*m_read_ptr)[i].agent_id = static_cast<uint32_t>(i);
            (*m_read_ptr)[i].is_active = true;

            (*m_write_ptr)[i].agent_id = static_cast<uint32_t>(i);
            (*m_write_ptr)[i].is_active = true;
        }
    }

} // namespace stellar_agents
