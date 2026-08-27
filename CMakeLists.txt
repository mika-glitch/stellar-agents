cmake_minimum_required(VERSION 3.22)
project(StellarAgents LANGUAGES CXX C)

set(CMAKE_CXX_STANDARD 20)
set(CMAKE_CXX_STANDARD_REQUIRED ON)


# ============================================================================
# BGFX VIA BGFX.CMAKE
# ============================================================================
# Define directories for bx, bimg, and bgfx
set(BGFX_DIR "${CMAKE_CURRENT_SOURCE_DIR}/ext/bgfx" CACHE PATH "")
set(BX_DIR "${CMAKE_CURRENT_SOURCE_DIR}/ext/bx" CACHE PATH "")
set(BIMG_DIR "${CMAKE_CURRENT_SOURCE_DIR}/ext/bimg" CACHE PATH "")

# Disable examples, but ENABLE tools to build the shaderc compiler
set(BGFX_BUILD_EXAMPLES OFF CACHE BOOL "")
set(BGFX_BUILD_TOOLS ON CACHE BOOL "" FORCE)

# Include the bgfx wrapper
add_subdirectory(ext/bgfx.cmake)

# ============================================================================
# SUBMODULE & EXECUTABLE
# ============================================================================
add_subdirectory("ext/glfw")

add_executable(StellarAgents
    "src/main.cpp"
    "src/engine/render_core.cpp"
    "src/engine/physics_evolution.cpp"
    "src/engine/universe_seeder.cpp"
    "src/engine/window_manager.cpp"
    "src/engine/kinematics_system.cpp"
    "src/engine/telemetry_hud.cpp"
    "src/engine/geometric_primitives.cpp"
    "src/cads/environment_matrix.cpp"
    "src/cads/passive_agent.cpp"
    "src/cads/adaptive_agent.cpp"
    "src/cads/attractor_agent.cpp"
    "src/math/relativity.cpp"
)

target_include_directories(StellarAgents PRIVATE 
    "src"
    "src/engine"
    "src/cads"
    "src/math"
    "src/shaders"
    "ext/bgfx/include"
    "ext/bimg/include"
    "ext/bx/include"
    "ext/glfw/include"
    "ext/bgfx.cmake/include"
)

# Link required libraries
target_link_libraries(StellarAgents PRIVATE 
    bgfx
    bx
    bimg
    glfw 
    winmm 
    gdi32 
    user32 
    shell32
)
# ============================================================================
# COMPILER OPTIMIZATIONS (Isolated to StellarAgents)
# ============================================================================
target_compile_options(StellarAgents PRIVATE 
    /O2 /Oi /Ot /arch:AVX2 /fp:fast /EHsc /GR- /Zc:__cplusplus /Zc:preprocessor
)
# ============================================================================
# [Core/Shaders] AUTOMATED SPIR-V SHADER COMPILATION PIPELINE
# ============================================================================
set(SHADER_SRC_DIR "${CMAKE_CURRENT_SOURCE_DIR}/src/shaders")
set(SHADER_BIN_DIR "${CMAKE_CURRENT_BINARY_DIR}/Release")

# Ensure target binary output directories are provisioned prior to invocation
file(MAKE_DIRECTORY ${SHADER_BIN_DIR})

# 1. Compile Screen-Space Raymarching Vertex Stream (lensing.vert)
add_custom_command(
    OUTPUT ${SHADER_BIN_DIR}/lensing_vert.bin
    COMMAND shaderc -f ${SHADER_SRC_DIR}/lensing.vert -o ${SHADER_BIN_DIR}/lensing_vert.bin
            --type vertex --platform windows --profile spirv
            -i "${CMAKE_CURRENT_SOURCE_DIR}/ext/bgfx/src"
    DEPENDS ${SHADER_SRC_DIR}/lensing.vert shaderc
    COMMENT "Compiling Screen-Space Vertex Shader (Vulkan/SPIR-V): lensing.vert"
)

# 2. Compile Volumetric Accretion Fragment Stream (accretion.frag)
add_custom_command(
    OUTPUT ${SHADER_BIN_DIR}/accretion_frag.bin
    COMMAND shaderc -f ${SHADER_SRC_DIR}/accretion.frag -o ${SHADER_BIN_DIR}/accretion_frag.bin
            --type fragment --platform windows --profile spirv
            -i "${CMAKE_CURRENT_SOURCE_DIR}/ext/bgfx/src"
    DEPENDS ${SHADER_SRC_DIR}/accretion.frag shaderc
    COMMENT "Compiling Volumetric Fragment Shader (Vulkan/SPIR-V): accretion.frag"
)

# 3. Compile Hardware Instancing Vertex Stream (instancing_vert.sc)
add_custom_command(
    OUTPUT ${SHADER_BIN_DIR}/instancing_vert.bin
    COMMAND shaderc -f ${SHADER_SRC_DIR}/instancing_vert.sc -o ${SHADER_BIN_DIR}/instancing_vert.bin
            --type vertex --platform windows --profile spirv
            --varyingdef ${SHADER_SRC_DIR}/instancing.varying.def.sc
            -i "${CMAKE_CURRENT_SOURCE_DIR}/ext/bgfx/src"
    DEPENDS ${SHADER_SRC_DIR}/instancing_vert.sc ${SHADER_SRC_DIR}/instancing.varying.def.sc shaderc
    COMMENT "Compiling Instancing Vertex Shader (Vulkan/SPIR-V): instancing_vert.sc"
)

# 4. Compile Hardware Instancing Fragment Stream (instancing_frag.sc)
add_custom_command(
    OUTPUT ${SHADER_BIN_DIR}/instancing_frag.bin
    COMMAND shaderc -f ${SHADER_SRC_DIR}/instancing_frag.sc -o ${SHADER_BIN_DIR}/instancing_frag.bin
            --type fragment --platform windows --profile spirv
            --varyingdef ${SHADER_SRC_DIR}/instancing.varying.def.sc
            -i "${CMAKE_CURRENT_SOURCE_DIR}/ext/bgfx/src"
    DEPENDS ${SHADER_SRC_DIR}/instancing_frag.sc ${SHADER_SRC_DIR}/instancing.varying.def.sc shaderc
    COMMENT "Compiling Instancing Fragment Shader (Vulkan/SPIR-V): instancing_frag.sc"
)

# Aggregate shader targets into a unified compilation dependency block
add_custom_target(CompileShaders ALL DEPENDS 
    ${SHADER_BIN_DIR}/lensing_vert.bin 
    ${SHADER_BIN_DIR}/accretion_frag.bin
    ${SHADER_BIN_DIR}/instancing_vert.bin
    ${SHADER_BIN_DIR}/instancing_frag.bin
)

# Enforce strict topological build order: Assets must be processed before linkage
add_dependencies(StellarAgents CompileShaders)