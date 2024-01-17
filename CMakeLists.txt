cmake_minimum_required(VERSION 3.27)

#include(cmake/utility.cmake)

set(CPP_STANDARD 23)
set(CMAKE_CXX_STANDARD ${CPP_STANDARD}) #TODO: remove -> is set on a per target basis

set(BUILD_SHARED_LIBS OFF)

project(DropEngine
    VERSION 0.0.1
    LANGUAGES C CXX
)

# Build options.
option(DROP_ENGINE_BUILD_CLIENT "Build the game client" ON)
option(DROP_ENGINE_BUILD_SERVER "Build the game server" ON)

# Generate compile commands (compile_commands.json) for clang tooling etc.
set(CMAKE_EXPORT_COMPILE_COMMANDS ON)

add_subdirectory(src bin)
