cmake_minimum_required(VERSION 4.3)

project(QuakeLauncher VERSION 1.0.0 LANGUAGES CXX)

add_executable(QuakeLauncher)
target_sources(QuakeLauncher
    PRIVATE
        Main.cpp
        DynamicLibrary/Interface.hpp
)

if(WIN32)
    target_sources(QuakeLauncher PRIVATE DynamicLibrary/Win32Implementation.cpp)
elseif(LINUX)
    target_sources(QuakeLauncher PRIVATE DynamicLibrary/LinuxImplementation.cpp)
endif()

if(MSVC)
    target_compile_options(QuakeLauncher
        PRIVATE 
            /W4 /permissive-)
else()
    target_compile_options(QuakeLauncher 
        PRIVATE
            -Wall -Wextra -Wpedantic -Wshadow -Wconversion
    )
endif()
# This is an issue on MinGW for some reason. If std::println is used then without this there will be
# undefined references.
if (MINGW AND CMAKE_CXX_COMPILER_ID STREQUAL "GNU")
    target_link_libraries(QuakeLauncher PRIVATE stdc++exp)
endif()

if(UNIX)
    set_target_properties(QuakeLauncher PROPERTIES
        BUILD_RPATH   "$ORIGIN"
        INSTALL_RPATH "$ORIGIN"
    )
endif()