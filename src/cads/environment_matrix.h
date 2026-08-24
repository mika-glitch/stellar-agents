#ifndef ENVIRONMENT_MATRIX_H
#define ENVIRONMENT_MATRIX_H

#include "agent_state.h"
#include <vector>
#include <memory>
#include <cstdint>

namespace stellar_agents {

    // ============================================================================
    // CADS ENVIRONMENT MATRIX CONTROLLER (DOUBLE-BUFFERING ARCHITECTURE)
    // Manages dual contiguous memory blocks to structurally eliminate data races
    // between asynchronous physics jthread pools and the VRAM streaming core.
    // ============================================================================
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
        // Read buffer access: Enforced const thread-safety for RenderCore streaming
        [[nodiscard]] const std::vector<AgentState>& get_read_buffer() const noexcept {
            return *m_read_ptr;
        }

        // Write buffer access: Provides raw target access for physics thread workers
        [[nodiscard]] std::vector<AgentState>& get_write_buffer() noexcept {
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

        // Dual isolated memory slices managed via lightweight smart pointers
        std::unique_ptr<std::vector<AgentState>> m_read_ptr;
        std::unique_ptr<std::vector<AgentState>> m_write_ptr;
    };

} // namespace stellar_agents

#endif // ENVIRONMENT_MATRIX_H
