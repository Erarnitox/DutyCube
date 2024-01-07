cmake_minimum_required(VERSION 3.5)
project(DropEngine)

# Build options.
option(DROP_ENGINE_BUILD_CLIENT "Build the game client" ON)
option(DROP_ENGINE_BUILD_SERVER "Build the game server" OFF)

# Generate compile commands (compile_commands.json) for clang tooling etc.
set(CMAKE_EXPORT_COMPILE_COMMANDS ON)

add_subdirectory(src)
