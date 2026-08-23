```
      .      *             .       .           *      .
  .       .        .        .      *        .       .
      *        .      Black Hole Engine   .          *
  .       .     .    [TAMSA SINGULARITY]     .       .
 *   .      .        .      *        .       .   *
      .      *             .       .           *      .

A project to make an AI Sandbox for interactive agents moving thru relativistic
conditions around an unstable black hole. Agents include planets, asteroids,
ships, stations, etc..



Core Engineer: Mika Rattay
Project Classification: AI Open-World Simulation Sandbox
Standard Specification: ISO/IEC C++20 Language Compliant Pipeline

Ein paar spielregeln .. denk erstmal nach was wir zuerst machen sollten, und welche dateien du von mir brauchst bevor du code auspuckst. Außerdem muss alles gut kommentiert im CIG stil sein, aber kein rollenspiel und CIG erwähnen. Der Namespace stellar_agents. Und es ist nach c++20 cig standards gerschieben, multi threaded
stellar-agents/
├── CMakeLists.txt                 # Master build orchestration steering native C++20 optimization flags
├── README.md                      # Structural documentation outlining the emergent multi-agent framework
│
└── src/                           # Monolithic containment layers dissolved into decentralized subsystems
    ├── main.cpp                   # Pure application boot node. Instantiates hardware viewport contexts,
    │                              # configures display bounds, and drives the global chronological tick.
    │
    ├── engine/                    # --- SECTOR 1: SIMULATION INFRUSTRUCTURE ---
    │   ├── render_core.h          
    │   ├── render_core.cpp        # Zero-allocation hardware blit gate. Streams processed continuous state 
    │   │                          # vector blocks to VRAM using direct batching. Free of simulation math.
    │   ├── physics_evolution.h    
    │   └── physics_evolution.cpp  # Asynchronous core execution engine. Orchestrates concurrent C++20 jthread pools
    │                              # to compute localized agent state mutations without thread locking barriers.
    │
    ├── cads/                      # --- SECTOR 2: COMPLEX ADAPTIVE DYNAMIC SYSTEMS (CADS) ---
    │   ├── agent_state.h          # Contiguous, cache-aligned struct definitions for flat data-oriented agent states 
    │   │                          # (Position, Velocity, Color, ID, AgentType). Maximum L1/L2 cache local footprint.
    │   ├── environment_matrix.h   # Multi-layered double-buffering controller. Regulates non-interfering read/write 
    │   │                          # permissions across thread workers to structurally eliminate data race conditions.
    │   │
    │   # --- AUTONOMOUS BEHAVIORAL CORES (PURE DECOUPLED MATHEMATICAL TRANSFORMATION FUNCTIONS) ---
    │   ├── attractor_agent.cpp    # Deterministic phase-space attractor logic (Stars/Singularities). Computes 
    │   │                          # non-linear macroscopic force fields mapping global boundary conditions.
    │   ├── passive_agent.cpp      # Non-adaptive particle mechanics (Asteroids/Debris). Processes relativistic orbital 
    │   │                          # trajectories and decay parameters under the influence of neighboring attractors.
    │   └── adaptive_agent.cpp     # Dissipative homeostatic intelligence (UEE Capital Ships). Computes non-conservative 
    │                              # behavioral loops, balancing local swarming thrust vectors with environmental threats.
    │
    └── math/                      # --- SECTOR 3: MATHEMATICAL RESOLUTION FOUNDATION ---
        ├── relativity.h           
        └── relativity.cpp         # Exception-free Schwarzschild lensing solver. Computes non-linear space-time 
                                   # optical path deflections, fully optimized for hardware-near vector registers.
