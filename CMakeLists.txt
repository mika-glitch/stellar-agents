cmake_minimum_required(VERSION 3.20)
project(StellarAgents VERSION 4.0 LANGUAGES CXX)

# ============================================================================
# C++ COMPILER CONFIGURATION & ADVANCED OPTIMIZATION STANDARDS
# ============================================================================
set(CMAKE_CXX_STANDARD 20)
set(CMAKE_CXX_STANDARD_REQUIRED ON)
set(CMAKE_CXX_EXTENSIONS OFF)

set(CMAKE_RUNTIME_OUTPUT_DIRECTORY ${CMAKE_BINARY_DIR}/bin)

if (MSVC)
    add_compile_options(/O2 /Oi /Ot /W4 /WX /permissive- /volatile:iso /std:c++20)
    add_definitions(-D_CRT_SECURE_NO_WARNINGS)
else()
    add_compile_options(-O3 -Wall -Wextra -Werror -march=native -mtune=native)
endif()

# ============================================================================
# THIRD-PARTY EXTERNAL DEPENDENCIES (STATIC ENGINE SUB-DECK)
# ============================================================================
set(BUILD_EXAMPLES OFF CACHE BOOL "Disable raylib examples compilation" FORCE)
set(BUILD_GAMES    OFF CACHE BOOL "Disable raylib games compilation" FORCE)
set(CUSTOM_RGBA_INTERNALS ON CACHE BOOL "Force internal hardware color profile matches" FORCE)

add_subdirectory(ext/raylib)

# ============================================================================
# MODULAR SYSTEM TRANSLATION UNITS MATRIX
# FIXED: Adjusted relativity path to target the 'src/math/' folder topology.
# ============================================================================
set(ENGINE_SOURCES
    src/main.cpp
    
    # Sector 1: Simulation Infrastructure Modules
    src/engine/render_core.cpp
    src/engine/physics_evolution.cpp
    
    # Sector 2: Complex Adaptive Dynamic Systems (CADS)
    src/cads/environment_matrix.cpp
    src/cads/attractor_agent.cpp
    src/cads/passive_agent.cpp
    src/cads/adaptive_agent.cpp
    
    # Sector 3: Mathematical Resolution Foundation
    src/math/relativity.cpp
)

# ============================================================================
# TARGET LINKAGE & SYSTEM HEADER ABSTRACTION ENFORCEMENT
# ============================================================================
add_executable(${PROJECT_NAME} ${ENGINE_SOURCES})

target_include_directories(${PROJECT_NAME} PRIVATE
    src
    src/engine
    src/cads
    src/math
    ext/raylib/src
)

target_link_libraries(${PROJECT_NAME} PRIVATE raylib)

if (WIN32)
    target_link_libraries(${PROJECT_NAME} PRIVATE winmm gdi32 openGL32)
endif()
