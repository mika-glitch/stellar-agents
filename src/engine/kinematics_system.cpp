// ============================================================================
// [Core/Input] FLIGHT KINEMATICS SYSTEM IMPLEMENTATION
// Description: Processes observer telemetry, executing trigonometric vector 
//              projection and linear thrust integration for 6-DOF movement.
// Standard: ISO C++20
// ============================================================================
#define NOMINMAX
#define WIN32_LEAN_AND_MEAN

#include "engine/kinematics_system.h"
#include "engine/window_manager.h"
#include <GLFW/glfw3.h>
#include <cmath>
#include <algorithm>

namespace engine {

    void KinematicsSystem::process_flight_inputs(engine::EngineContext& context, float delta_time) {
        // Retrieve the active window context safely via the abstracted WindowManager
        GLFWwindow* current_window = static_cast<GLFWwindow*>(engine::WindowManager::get_window_handle());
        if (!current_window) return;

        engine::ObserverTelemetry& flight = context.flight;

        // --------------------------------------------------------------------
        // [1] Euler to Cartesian Vector Projection
        // --------------------------------------------------------------------

        /**
         * Angular Conversion
         * Formula: rad = deg * (pi / 180)
         */
        const float rad_yaw = flight.yaw * 3.14159265f / 180.0f;
        const float rad_pitch = flight.pitch * 3.14159265f / 180.0f;

        /**
         * Spherical to Cartesian Coordinate Transformation
         * Derives the localized forward vector from rotational telemetry:
         * fx = cos(yaw) * cos(pitch)
         * fy = sin(pitch)
         * fz = sin(yaw) * cos(pitch)
         */
        flight.forward[0] = std::cos(rad_yaw) * std::cos(rad_pitch);
        flight.forward[1] = std::sin(rad_pitch);
        flight.forward[2] = std::sin(rad_yaw) * std::cos(rad_pitch);

        /**
         * Forward Vector Normalization
         */
        const float magnitude_fwd = std::sqrt(flight.forward[0] * flight.forward[0] +
            flight.forward[1] * flight.forward[1] +
            flight.forward[2] * flight.forward[2]);
        if (magnitude_fwd > 0.0001f) {
            flight.forward[0] /= magnitude_fwd;
            flight.forward[1] /= magnitude_fwd;
            flight.forward[2] /= magnitude_fwd;
        }

        /**
         * Orthogonal Right Vector Derivation
         */
        flight.right[0] = -flight.forward[2];
        flight.right[2] = flight.forward[0];

        // Right vector normalization
        const float magnitude_right = std::sqrt(flight.right[0] * flight.right[0] +
            flight.right[2] * flight.right[2]);
        if (magnitude_right > 0.0001f) {
            flight.right[0] /= magnitude_right;
            flight.right[2] /= magnitude_right;
        }

        // --------------------------------------------------------------------
        // [2] Propulsion Actuator Logic
        // --------------------------------------------------------------------

        /**
         * Linear Throttle Modulation
         * Clamps the propulsion output strictly within the interval [0.0, 1.0].
         */
        if (glfwGetKey(current_window, GLFW_KEY_R) == GLFW_PRESS) {
            flight.throttle = std::min(1.0f, flight.throttle + 0.5f * delta_time);
        }
        if (glfwGetKey(current_window, GLFW_KEY_F) == GLFW_PRESS) {
            flight.throttle = std::max(0.0f, flight.throttle - 0.5f * delta_time);
        }

        // Base velocity calculation derived from delta time and current throttle saturation
        float velocity_scalar = 0.4f * flight.throttle * delta_time;

        // Afterburner override applies a fixed kinetic multiplier
        if (glfwGetKey(current_window, GLFW_KEY_LEFT_SHIFT) == GLFW_PRESS) {
            velocity_scalar *= 2.5f;
        }

        // --------------------------------------------------------------------
        // [3] Spatial Translation Integration
        // --------------------------------------------------------------------

         // Longitudinal axis (Forward/Backward translation)
        if (glfwGetKey(current_window, GLFW_KEY_W) == GLFW_PRESS) {
            flight.pos[0] += flight.forward[0] * velocity_scalar;
            flight.pos[1] += flight.forward[1] * velocity_scalar;
            flight.pos[2] += flight.forward[2] * velocity_scalar;
        }
        if (glfwGetKey(current_window, GLFW_KEY_S) == GLFW_PRESS) {
            flight.pos[0] -= flight.forward[0] * velocity_scalar;
            flight.pos[1] -= flight.forward[1] * velocity_scalar;
            flight.pos[2] -= flight.forward[2] * velocity_scalar;
        }

        // Lateral axis (Port/Starboard translation) - inverted A/D mapping
        if (glfwGetKey(current_window, GLFW_KEY_A) == GLFW_PRESS) {
            flight.pos[0] -= flight.right[0] * velocity_scalar;
            flight.pos[2] -= flight.right[2] * velocity_scalar;
        }
        if (glfwGetKey(current_window, GLFW_KEY_D) == GLFW_PRESS) {
            flight.pos[0] += flight.right[0] * velocity_scalar;
            flight.pos[2] += flight.right[2] * velocity_scalar;
        }

        // Vertical axis (Ascend/Descend translation relative to global Y)
        if (glfwGetKey(current_window, GLFW_KEY_LEFT_CONTROL) == GLFW_PRESS) {
            flight.pos[1] += velocity_scalar;
        }
        if (glfwGetKey(current_window, GLFW_KEY_SPACE) == GLFW_PRESS) {
            flight.pos[1] -= velocity_scalar;
        }

        // --------------------------------------------------------------------
        // [4] Telemetry Registration
        // --------------------------------------------------------------------

        if (delta_time > 0.0f) {
            flight.current_speed = velocity_scalar * (1.0f / delta_time);
        }
    }

} // namespace engine