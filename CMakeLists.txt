cmake_minimum_required(VERSION 3.22)
project(StellarAgents LANGUAGES CXX C)

# 1. Globale C++20-Barriere
set(CMAKE_CXX_STANDARD 20)

# 2. Hardcore-Compiler-Flags für MSVC (AVX2-Vektorisierung + Exception-Kappung)
add_compile_options(/O2 /Oi /Ot /arch:AVX2 /fp:fast /EHsc /GR-)

add_compile_definitions(GRAPHICS_API_VULKAN)

# 3. Das unzensierte, direkte raylib-Quellprojekt einbinden
add_subdirectory("ext/raylib" "build_raylib_sub")

# 4. Das Executable mit den echten, direkten Pfaden zu deinen Quelldateien füttern
add_executable(StellarAgents
    "src/main.cpp"
    "src/engine/render_core.cpp"
    "src/engine/physics_evolution.cpp"
    "src/cads/environment_matrix.cpp"
    "src/cads/passive_agent.cpp"
    "src/cads/adaptive_agent.cpp"
    "src/cads/attractor_agent.cpp"
    "src/math/relativity.cpp"
)

# 5. Direkte Include-Suchpfade für den Präprozessor festschweißen
target_include_directories(StellarAgents PRIVATE 
    "src"
    "src/engine"
    "src/cads"
    "src/math"
    "ext/raylib/src"
)

# 6. Direktes Linker-Gatter für die statische raylib und Windows-Systemkerne
target_link_libraries(StellarAgents PRIVATE raylib winmm gdi32 opengl32 user32 shell32)
