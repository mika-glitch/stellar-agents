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


========================================================================================================
[ FRONTEND DECK: RAYLIB HOST APPLICATION ] (Main Thread / Cheap Input & System Unwind Registers)
========================================================================================================
  [ CameraController ] (Free Flight Math) ───► Updates Camera LookAt & POS Vectors ──┐
                                                                                     ▼
  [ Tactical HUD ]     (Telemetry UI)    ◄─── Real-Time Throttled Data Stream ──► [ MAIN ENGINE LOOP ]
                                                                                     │
               ┌─────────────────────────────────────────────────────────────────────┘
               │ (1) Dispatch Frame Physics Tick
               ▼
========================================================================================================
[ LOW-LEVEL ENGINE BACKEND: NATIVE C++20 CORE ] (Pure Brute-Force Asynchronous Processing Stack)
========================================================================================================
  
  [ BlackHole (Singularity) ] ───► Calculates Multi-Body Gravity Acceleration Field Vectors
               │
               ├─► [ Relativistic Raymarching Pipeline ] ──► [ Direct GPU Draw (Einstein Ring Layer) ]
               │
               ▼ (2) Dispatches Spatial Parameters & Mass Coordinates
  
  [ Universe (Job Manager) ]  ───► Spawns Autonomous Worker Pool Topology (Contiguous Cache Buffers)
               │
               ▼ Instantiates Asynchronous C++20 Execution Jobs Across All Available Hardware Cores
  ┌──────────────────────────────┬──────────────────────────────┬──────────────────────────────┐
  │ (Spawn Worker Thread 0)      │ (Spawn Worker Thread 1)      │ (Spawn Worker Thread n)      │
  ▼                              ▼                              ▼                              ▼
========================================================================================================
[ WORKER-POOL MATRIX: DETACHED TASK EXECUTION LAYERS ] (Lock-Free Thread Barriers)
========================================================================================================
  ║    std::jthread Task 0     ║ ║    std::jthread Task 1     ║ ║    std::jthread Task n     ║
  ============================== ============================== ==============================
  │ (Computes Orbit Chunk 0)     │ (Computes Orbit Chunk 1)     │ (Computes Orbit Chunk n)     
  ▼                              ▼                              ▼                              
  [ Asteroid Data Block 0 ]      [ Asteroid Data Block 1 ]      [ Asteroid Data Block n ]      
  (Reads current_states / Writes exclusively to separate parallel registers inside next_states)
  │                              │                              │                              
  └──────────────────────────────┼──────────────────────────────┘                              
                                 │
                                 ▼ [ C++20 Synchronization Join Barrier / Pointer Swap ]
                                 │ (All parallel workers report ready back to the Host Thread)
                                 ▼
========================================================================================================
[ HARDWARE GRAPHICS BLIT PASS ] (Zero-Allocation Render Execution Queue)
========================================================================================================
                                 │
                                 ▼ rlBegin(RL_LINES) 
                      [ Relativity::GetApparentPosition() ] ◄─── (Evaluates Einstein Lens Shifts)
                                 │
                                 ▼ Streams Vertex Buffers Straight to the Bus Interface
                    ====================================
                    ║                GPU               ║ ◄─── High-FPS Bulk Vector Array Flush
                    ====================================
