// ============================================================================
// [Core/Graphics] GEOMETRIC PRIMITIVE GENERATOR
// Description: Procedural generation of 3D topological manifolds for 
//              hardware-instanced rendering pipelines. Generates optimized
//              vertex and index buffers for mass entity rendering.
// Standard: ISO C++20
// ============================================================================
#ifndef GEOMETRIC_PRIMITIVES_H
#define GEOMETRIC_PRIMITIVES_H

#include <vector>
#include <cstdint>

namespace stellar_agents {

    // Standardized spatial payload for 3D hardware pipelines
    struct MeshVertex {
        float x, y, z;
        float nx, ny, nz;
    };

    // Data container for raw geometry upload
    struct MeshData {
        std::vector<MeshVertex> vertices;
        std::vector<uint16_t> indices;
    };

    class GeometricPrimitives {
    public:
        // Generates an irregular icosahedron manifold for volumetric debris fields
        [[nodiscard]] static MeshData GenerateDebrisManifold() noexcept;

        // Generates a directed tetrahedral wedge for autonomous navigation units
        [[nodiscard]] static MeshData GenerateChassisManifold() noexcept;
    };

} // namespace stellar_agents

#endif // GEOMETRIC_PRIMITIVES_H