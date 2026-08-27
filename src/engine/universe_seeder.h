// ============================================================================
// [AAA/Simulation] UNIVERSE SEEDER INTERFACE
// Description: Handles deterministic initial condition population for 
//              gravitational attractors, debris fields, and navigation agents.
// Standard: ISO C++20
// ============================================================================
#ifndef UNIVERSE_SEEDER_H
#define UNIVERSE_SEEDER_H

#include "cads/environment_matrix.h"

namespace stellar_agents {

    class UniverseSeeder {
    public:
        /**
         * Populates the environment matrix buffer with gravitational attractors,
         * passive asteroid belts, and adaptive navigation entities.
         *
         * @param matrix Reference to the double-buffered environment matrix SoA arena.
         */
        static void seed_initial_conditions(stellar_agents::EnvironmentMatrix& matrix);
    };

} // namespace stellar_agents 

#endif // UNIVERSE_SEEDER_H