cmake_minimum_required(VERSION 4.3)

project(QuakeRoot LANGUAGES NONE)

set(CMAKE_CXX_STANDARD 23)
set(CMAKE_CXX_STANDARD_REQUIRED ON)
set(CMAKE_CXX_EXTENSIONS OFF)

# Make em friends.
set(CMAKE_RUNTIME_OUTPUT_DIRECTORY ${CMAKE_BINARY_DIR}/bin)
set(CMAKE_LIBRARY_OUTPUT_DIRECTORY ${CMAKE_BINARY_DIR}/bin)

add_subdirectory(QuakeLauncher)
add_subdirectory(Quake)

add_dependencies(QuakeLauncher Quake)