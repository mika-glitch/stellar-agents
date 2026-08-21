#stellar-agents
cmake_minimum_required(VERSION 3.16)
project(StellarAgents LANGUAGES CXX)

set(CMAKE_CXX_STANDARD 20)
set(CMAKE_CXX_STANDARD_REQUIRED ON)

# dont compile raylib examples and games
set(BUILD_EXAMPLES OFF CACHE BOOL "Raylib Beispiele nicht bauen" FORCE)
set(BUILD_GAMES    OFF CACHE BOOL "Raylib Spiele nicht bauen" FORCE)

add_subdirectory(ext/raylib)

set(SOURCES
    src/main.cpp
    src/black_hole.cpp
    src/relativity.cpp 
)

add_executable(${PROJECT_NAME} ${SOURCES})
target_link_libraries(${PROJECT_NAME} PRIVATE raylib)