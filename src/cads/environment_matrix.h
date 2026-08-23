#pragma once
#ifndef ENVIRONMENT_MATRIX_H
#define ENVIRONMENT_MATRIX_H

#include "agent_state.h"
#include <vector>
#include <cstdint>

namespace stellar_agents {

    // ============================================================================
    // CADS ENVIRONMENTAL STATES MATRIX LAYER
    // Encapsulates the contiguous memory arrays managing dual execution buffers.
    // Guarantees lock-free read/write isolation across concurrent worker threads.
    // ============================================================================
    class EnvironmentMatrix final {
    private:
        uint64_t total_allocated_agents{ 0 };

        // Contiguous memory blocks for explicit double-buffering isolation
        std::vector<AgentState> readable_buffer;
        std::vector<AgentState> writable_buffer;

    public:
        explicit EnvironmentMatrix(uint64_t p_max_agents) noexcept;
        ~EnvironmentMatrix() = default;

        // Disable compiler copy mechanisms to safe contiguous hardware buffers
        EnvironmentMatrix(const EnvironmentMatrix&) = delete;
        EnvironmentMatrix& operator=(const EnvironmentMatrix&) = delete;

        // ============================================================================
        // LOW-OVERHEAD BUFFER FLIP REGISTER
        // Swaps internal buffer allocations within microseconds via zero-copy logic.
        // ============================================================================
        void swap_evolutionary_buffers() noexcept;

        // Thread-safe read access to the active frame state memory sequence
        [[nodiscard]] const std::vector<AgentState>& get_read_buffer() const noexcept { return readable_buffer; }

        // Direct slice pointers providing isolated write fields for thread tasks
        [[nodiscard]] std::vector<AgentState>& get_write_buffer() noexcept { return writable_buffer; }

        [[nodiscard]] uint64_t get_capacity() const noexcept { return total_allocated_agents; }
        [[nodiscard]] uint64_t count_active_agents() const noexcept;
    };

} // namespace stellar_agents

#endif // ENVIRONMENT_MATRIX_H
