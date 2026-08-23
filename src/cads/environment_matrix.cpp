#include "environment_matrix.h"
#include <utility>
#include <algorithm>

stellar_agents::EnvironmentMatrix::EnvironmentMatrix(uint64_t p_max_agents) noexcept
    : total_allocated_agents(p_max_agents)
{
    // Pre-allocate contiguous heap memory blocks to eliminate runtime allocations
    readable_buffer.resize(total_allocated_agents);
    writable_buffer.resize(total_allocated_agents);
}

void stellar_agents::EnvironmentMatrix::swap_evolutionary_buffers() noexcept {
    // Standard-compliant atomic pointer swap bypassing layout reallocation overhead
    std::swap(readable_buffer, writable_buffer);
}

uint64_t stellar_agents::EnvironmentMatrix::count_active_agents() const noexcept {
    uint64_t active_count = 0;

    // Linear high-throughput contiguous array processing sweep
    for (uint64_t i = 0; i < total_allocated_agents; ++i) {
        if (readable_buffer[i].is_active) {
            ++active_count;
        }
    }

    return active_count;
}
