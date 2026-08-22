#include "relativity.h"
#include "raymath.h"
#include <cmath>

// ============================================================================
// EINSTEINIAN RELATIVISTIC LENSING PROJECTION ENGINE
// Standard compliance: Enforced strict zero-exception guarantees and precise 
// global namespace mapping to protect high-performance concurrent physics tasks.
// ============================================================================
Vector3 godot_hpc::Relativity::GetApparentPosition(
    const Vector3 realPos,
    const Vector3 camPos,
    const Vector3 holePos,
    const float mass,
    const float eventHorizon) noexcept
{
    // 1. Calculate relative spatial displacement vectors from the gravitational center
    const Vector3 relCam = Vector3Subtract(camPos, holePos);
    const Vector3 relObj = Vector3Subtract(realPos, holePos);

    // Register-caching the geometric scalar distance of the object to block redundant calculations
    const float distObj = Vector3Length(relObj);

    // Inside the absolute black abyss beyond the photon sphere, there is no optical escape
    if (distObj < eventHorizon) [[unlikely]] {
        return realPos;
    }

    // 2. Compute normalized line-of-sight vector tracking the undeflected ray trajectory
    const Vector3 viewDir = Vector3Normalize(Vector3Subtract(realPos, camPos));

    // Vector projection pass tracking the closest point of approach along the camera forward axis
    const float t = Vector3DotProduct(Vector3Negate(relCam), viewDir);
    if (t < 0.0f) [[unlikely]] {
        return realPos;
    }

    // 3. Resolve the closest spatial coordinates (periapsis proxy) to calculate the impact parameter
    const Vector3 closestPoint = Vector3Add(camPos, Vector3Scale(viewDir, t));
    const float b = Vector3Distance(closestPoint, holePos);

    // Critical hardware shield: Artificially damp the impact parameter near the event horizon boundary 
    // to suppress geometric infinity division artifacts (NaN protection gate).
    const float effectiveImpactParameter = (b < eventHorizon * 1.05f) ? (eventHorizon * 1.05f) : b;

    // ============================================================================
    // SCHWARZSCHILD DEFLECTION MATH APPROXIMATION
    // Implements the linearized Einstein deflection calculation framework: alpha = (4 * G * M) / (c^2 * b)
    // Scaled with a high-performance custom spatial tuning multiplier (0.0015f).
    // ============================================================================
    const float deflectionAngle = (4.0f * (mass * 0.0015f)) / effectiveImpactParameter;

    // 4. Construct the relativistic apparent lensing vector shift
    // Extract the radial vector pointing outward from the singularity core toward the periapsis point.
    const Vector3 radialVector = Vector3Normalize(Vector3Subtract(closestPoint, holePos));

    // Displace the closest point of approach outward along the lens deflection axis gradient
    const Vector3 apparentClosestPoint = Vector3Add(holePos, Vector3Scale(radialVector, effectiveImpactParameter * (1.0f + deflectionAngle)));

    // 5. Zero-Copy vector blending pass: Extrapolate the shifted position back into absolute coordinates
    // Re-projects the deflected visual path along the original line-of-sight vector field.
    return Vector3Add(apparentClosestPoint, Vector3Scale(viewDir, distObj - effectiveImpactParameter));
}
