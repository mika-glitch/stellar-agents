// ============================================================================
// [Core/Graphics] GEOMETRIC PRIMITIVE GENERATOR IMPLEMENTATION
// Description: Implements procedural topological algorithms to generate 
//              lightweight 3D assets for high-density hardware instancing.
// Standard: ISO C++20
// ============================================================================
#include "geometric_primitives.h"
#include <cmath>
#include <array>
#include <vector>

namespace stellar_agents {

    // Evaluates a pseudo-random scalar field for structural displacement
    static float ComputeSpatialNoise(float x, float y, float z) noexcept {
        // High-frequency hash for micro-detail
        const float hash1 = std::abs(std::fmod(x * 12.9898f + y * 78.233f + z * 37.719f, 1.0f));
        // Low-frequency hash for macro-structure
        const float hash2 = std::abs(std::fmod(x * 4.1415f + y * 8.982f + z * 5.765f, 1.0f));
        // Tri-planar blending
        return (hash1 * 0.4f) + (hash2 * 0.6f);
    }

    MeshData GeometricPrimitives::GenerateDebrisManifold() noexcept {
        MeshData mesh;

        // Base scalar constants for procedural volume bounding
        // Set to the requested MAX physical size. 
        // Instance-specific scaling MUST be applied via the Transformation Matrix payload.
        constexpr float maxScale = 0.075f;
        constexpr float craterDepth = 0.45f;

        // Golden ratio formulation: $\phi = \frac{1 + \sqrt{5}}{2}$
        const float phi = (1.0f + std::sqrt(5.0f)) / 2.0f;

        // Foundational un-normalized spatial coordinates for the base icosahedron
        std::vector<std::array<float, 3>> vertices = {
            {-1.0f,  phi,  0.0f}, { 1.0f,  phi,  0.0f}, {-1.0f, -phi,  0.0f}, { 1.0f, -phi,  0.0f},
            { 0.0f, -1.0f,  phi}, { 0.0f,  1.0f,  phi}, { 0.0f, -1.0f, -phi}, { 0.0f,  1.0f, -phi},
            { phi,  0.0f, -1.0f}, { phi,  0.0f,  1.0f}, {-phi,  0.0f, -1.0f}, {-phi,  0.0f,  1.0f}
        };

        // Topological mapping of the 20 base structural faces
        std::vector<std::array<uint16_t, 3>> faces = {
            {0, 11, 5},  {0, 5, 1},   {0, 1, 7},   {0, 7, 10},  {0, 10, 11},
            {1, 5, 9},   {5, 11, 4},  {11, 10, 2}, {10, 7, 6},  {7, 1, 8},
            {3, 9, 4},   {3, 4, 2},   {3, 2, 6},   {3, 6, 8},   {3, 8, 9},
            {4, 9, 5},   {2, 4, 11},  {6, 2, 10},  {8, 6, 7},   {9, 8, 1}
        };

        // ====================================================================
        // GEOMETRIC SUBDIVISION PASS
        // Increases topological density from 20 to 80 faces to allow for 
        // high-fidelity morphological displacement (craters and ridges).
        // ====================================================================
        std::vector<std::array<uint16_t, 3>> subdividedFaces;
        subdividedFaces.reserve(faces.size() * 4);

        for (const auto& face : faces) {
            const auto& v0 = vertices[face[0]];
            const auto& v1 = vertices[face[1]];
            const auto& v2 = vertices[face[2]];

            // Derive structural midpoints
            std::array<float, 3> m01 = { v0[0] + v1[0], v0[1] + v1[1], v0[2] + v1[2] };
            std::array<float, 3> m12 = { v1[0] + v2[0], v1[1] + v2[1], v1[2] + v2[2] };
            std::array<float, 3> m20 = { v2[0] + v0[0], v2[1] + v0[1], v2[2] + v0[2] };

            uint16_t i01 = static_cast<uint16_t>(vertices.size()); vertices.push_back(m01);
            uint16_t i12 = static_cast<uint16_t>(vertices.size()); vertices.push_back(m12);
            uint16_t i20 = static_cast<uint16_t>(vertices.size()); vertices.push_back(m20);

            // Inject 4 new micro-faces per macroscopic base face
            subdividedFaces.push_back({ face[0], i01, i20 });
            subdividedFaces.push_back({ face[1], i12, i01 });
            subdividedFaces.push_back({ face[2], i20, i12 });
            subdividedFaces.push_back({ i01, i12, i20 });
        }
        faces = std::move(subdividedFaces);

        // ====================================================================
        // SPHERICAL NORMALIZATION & VOLUMETRIC DISPLACEMENT
        // ====================================================================
        for (auto& v : vertices) {
            const float len = std::sqrt(v[0] * v[0] + v[1] * v[1] + v[2] * v[2]);

            // Normalize to unit sphere bounds
            const float nx = v[0] / len;
            const float ny = v[1] / len;
            const float nz = v[2] / len;

            // Evaluate procedural spatial noise based on normalized coordinate mapping
            const float noise = ComputeSpatialNoise(nx, ny, nz);

            // Morphological crater generation equation: $r' = r_{max} \cdot (1.0 - amplitude \cdot noise)$
            const float finalRadius = maxScale * (1.0f - (noise * craterDepth));

            v[0] = nx * finalRadius;
            v[1] = ny * finalRadius;
            v[2] = nz * finalRadius;
        }

        // Pre-allocate memory buffers to prevent reallocation overhead
        mesh.vertices.reserve(faces.size() * 3);
        mesh.indices.reserve(faces.size() * 3);

        // Lambda utility for unrolled face injection with explicit flat-shading normal derivation
        auto injectFlatFace = [&](const std::array<float, 3>& p1, const std::array<float, 3>& p2, const std::array<float, 3>& p3) {
            // Compute structural edge vectors: $\mathbf{u} = \mathbf{p2} - \mathbf{p1}$, $\mathbf{w} = \mathbf{p3} - \mathbf{p1}$
            const float u[3] = { p2[0] - p1[0], p2[1] - p1[1], p2[2] - p1[2] };
            const float w[3] = { p3[0] - p1[0], p3[1] - p1[1], p3[2] - p1[2] };

            // Compute orthogonal face normal vector: $\vec{N} = \mathbf{u} \times \mathbf{w}$
            float nx = u[1] * w[2] - u[2] * w[1];
            float ny = u[2] * w[0] - u[0] * w[2];
            float nz = u[0] * w[1] - u[1] * w[0];

            // Normalize vector magnitude: $\|\vec{N}\| = \sqrt{N_x^2 + N_y^2 + N_z^2}$
            const float normalLength = std::sqrt(nx * nx + ny * ny + nz * nz);
            if (normalLength > 1e-6f) {
                nx /= normalLength;
                ny /= normalLength;
                nz /= normalLength;
            }
            else {
                nx = 0.0f; ny = 0.0f; nz = 1.0f; // Degenerate fallback protection
            }

            const uint16_t currentIndex = static_cast<uint16_t>(mesh.vertices.size());

            // Push unrolled vertices with identical flat normals for sharp edge rasterization
            mesh.vertices.push_back({ p1[0], p1[1], p1[2], nx, ny, nz });
            mesh.vertices.push_back({ p2[0], p2[1], p2[2], nx, ny, nz });
            mesh.vertices.push_back({ p3[0], p3[1], p3[2], nx, ny, nz });

            mesh.indices.push_back(currentIndex);
            mesh.indices.push_back(currentIndex + 1);
            mesh.indices.push_back(currentIndex + 2);
            };

        // Execute topological expansion mapping
        for (const auto& face : faces) {
            injectFlatFace(vertices[face[0]], vertices[face[1]], vertices[face[2]]);
        }

        return mesh;
    }

    MeshData GeometricPrimitives::GenerateChassisManifold() noexcept {
        MeshData mesh;

        // Procedural topological manifold generation for high-velocity aerospace vehicle chassis.
        // Coordinate system: +Z defines the longitudinal forward thrust vector.
        constexpr float length = 1.2f;
        constexpr float width = 0.4f;
        constexpr float height = 0.3f;

        // Define foundational vertex positions for the tetrahedral geometry primitive
        const MeshVertex v0 = { 0.0f,   0.0f,   length,          0.0f,  0.0f,  1.0f }; // Apex Nose
        const MeshVertex v1 = { -width, -height, -length * 0.5f, -1.0f, -1.0f, -1.0f }; // Port Nacelle
        const MeshVertex v2 = { width, -height, -length * 0.5f,  1.0f, -1.0f, -1.0f }; // Starboard Nacelle
        const MeshVertex v3 = { 0.0f,   height, -length * 0.5f,  0.0f,  1.0f, -1.0f }; // Dorsal Fin Stabilizer

        // Lambda utility for face injection with explicit flat-shading normal derivation
        auto injectFace = [&](MeshVertex a, MeshVertex b, MeshVertex c) {
            // Compute structural edge vectors: 
            // $\mathbf{u} = \mathbf{b} - \mathbf{a}$
            // $\mathbf{w} = \mathbf{c} - \mathbf{a}$
            const float u[3] = { b.x - a.x, b.y - a.y, b.z - a.z };
            const float w[3] = { c.x - a.x, c.y - a.y, c.z - a.z };

            // Compute cross product for face normal vector: $\vec{N} = \mathbf{u} \times \mathbf{w}$
            float nx = u[1] * w[2] - u[2] * w[1];
            float ny = u[2] * w[0] - u[0] * w[2];
            float nz = u[0] * w[1] - u[1] * w[0];

            // Normalize vector magnitude with singularity protection
            const float len = std::sqrt(nx * nx + ny * ny + nz * nz);
            if (len > 1e-6f) {
                nx /= len;
                ny /= len;
                nz /= len;
            }
            else {
                nx = 0.0f; ny = 0.0f; nz = 1.0f; // Degenerate fallback
            }

            a.nx = b.nx = c.nx = nx;
            a.ny = b.ny = c.ny = ny;
            a.nz = b.nz = c.nz = nz;

            const uint16_t idx = static_cast<uint16_t>(mesh.vertices.size());
            mesh.vertices.push_back(a);
            mesh.vertices.push_back(b);
            mesh.vertices.push_back(c);

            mesh.indices.push_back(idx);
            mesh.indices.push_back(idx + 1);
            mesh.indices.push_back(idx + 2);
            };

        // Construct topological polygon faces for the tetrahedral enclosure
        injectFace(v0, v1, v3); // Port lateral profile
        injectFace(v0, v3, v2); // Starboard lateral profile
        injectFace(v0, v2, v1); // Ventral profile
        injectFace(v3, v1, v2); // Aft propulsion bulkhead plate

        return mesh;
    }

} // namespace stellar_agents