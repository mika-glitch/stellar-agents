#include "relativity.h"
#include <cmath>

namespace stellar_agents {

    // ============================================================================
    // LOCAL VECTOR MATH HELPERS (STANDALONE REPLACEMENT FOR RAYMATH)
    // ============================================================================
    [[nodiscard]] static inline Vector3 VectorAdd(const Vector3& a, const Vector3& b) noexcept {
        return { a.x + b.x, a.y + b.y, a.z + b.z };
    }

    [[nodiscard]] static inline Vector3 VectorSubtract(const Vector3& a, const Vector3& b) noexcept {
        return { a.x - b.x, a.y - b.y, a.z - b.z };
    }

    [[nodiscard]] static inline Vector3 VectorScale(const Vector3& v, float scale) noexcept {
        return { v.x * scale, v.y * scale, v.z * scale };
    }

    [[nodiscard]] static inline float VectorLengthSqr(const Vector3& v) noexcept {
        return (v.x * v.x) + (v.y * v.y) + (v.z * v.z);
    }

    [[nodiscard]] static inline float VectorLength(const Vector3& v) noexcept {
        return std::sqrt(VectorLengthSqr(v));
    }

    [[nodiscard]] static inline float VectorDistance(const Vector3& v1, const Vector3& v2) noexcept {
        return VectorLength(VectorSubtract(v1, v2));
    }

    [[nodiscard]] static inline Vector3 VectorNormalize(const Vector3& v) noexcept {
        float len = VectorLength(v);
        if (len > 0.00001f) {
            return { v.x / len, v.y / len, v.z / len };
        }
        return { 0.0f, 0.0f, 0.0f };
    }

    [[nodiscard]] static inline float VectorDotProduct(const Vector3& a, const Vector3& b) noexcept {
        return (a.x * b.x) + (a.y * b.y) + (a.z * b.z);
    }

    [[nodiscard]] static inline Vector3 VectorNegate(const Vector3& v) noexcept {
        return { -v.x, -v.y, -v.z };
    }

    // ============================================================================
    // MATHEMATICAL SCHWARZSCHILD MATRIX RESOLUTION
    // Standard compliance: Fully scoped within the unified project namespace.
    // ============================================================================
    Vector3 Relativity::GetApparentPosition(
        const Vector3 realPos,
        const Vector3 camPos,
        const Vector3 holePos,
        const float mass,
        const float eventHorizon) noexcept
    {
        // Calculate relative spatial displacement vectors from the gravitational center (corrected without '3' suffix)
        const Vector3 relCam = VectorSubtract(camPos, holePos);
        const Vector3 relObj = VectorSubtract(realPos, holePos);

        // Register-caching the geometric scalar distance of the object to suppress redundant instructions
        const float distObj = VectorLength(relObj);

        // Core boundary shield: Inside the black hole horizon, light path collapses completely
        if (distObj < eventHorizon) [[unlikely]] {
            return realPos;
        }

        // Compute normalized line-of-sight vector tracking the undeflected ray trajectory
        const Vector3 viewDir = VectorNormalize(VectorSubtract(realPos, camPos));

        // Vector projection pass tracking the closest point of approach along the camera forward axis
        const float t = VectorDotProduct(VectorNegate(relCam), viewDir);
        if (t < 0.0f) [[unlikely]] {
            return realPos;
        }

        // Resolve the closest spatial coordinates (periapsis proxy) to calculate the impact parameter
        const Vector3 closestPoint = VectorAdd(camPos, VectorScale(viewDir, t));
        const float b = VectorDistance(closestPoint, holePos);

        // Anti-infinity shield: Artificially damp the impact parameter near the event horizon 
        // boundary to suppress geometric floating-point NaN violations.
        const float effectiveImpactParameter = (b < eventHorizon * 1.05f) ? (eventHorizon * 1.05f) : b;

        // Linearized Schwarzschild deflection calculation framework: alpha = (4 * G * M) / (c^2 * b)
        // Scaled with a custom spatial tuning multiplier (0.0015f) for visual parity.
        const float deflectionAngle = (4.0f * (mass * 0.0015f)) / effectiveImpactParameter;

        // Construct the relativistic apparent lensing vector shift
        // Extract the radial vector pointing outward from the singularity core toward the periapsis point.
        const Vector3 radialVector = VectorNormalize(VectorSubtract(closestPoint, holePos));

        // Displace the closest point of approach outward along the lens deflection axis gradient
        const Vector3 apparentClosestPoint = VectorAdd(holePos, VectorScale(radialVector, effectiveImpactParameter * (1.0f + deflectionAngle)));

        // ========================================================================
        // GEOMETRIC SHEAR PATCH: Camera-relative Z-Depth Extrapolation
        // Zero-Copy vector blending pass: Extrapolate the shifted position back into absolute space
        // Re-projects the deflected visual path along the original line-of-sight vector field.
        // ========================================================================
        const float camToObjDist = VectorDistance(realPos, camPos);
        return VectorAdd(apparentClosestPoint, VectorScale(viewDir, camToObjDist - t));
    }

} // namespace stellar_agents