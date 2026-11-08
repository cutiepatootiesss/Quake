cmake_minimum_required(VERSION 4.3)

project(Quake VERSION 1.0.0 LANGUAGES CXX)

set(CMAKE_WINDOWS_EXPORT_ALL_SYMBOLS ON)

add_library(Quake SHARED Main.cpp
    Renderer/Device.hpp
    Renderer/CommandList.hpp
    Renderer/Pipeline.hpp
    Renderer/Texture.hpp
    Renderer/Buffer.hpp
)

if(MSVC)
    target_compile_options(Quake
        PRIVATE 
            /W4 /permissive-)
else()
    target_compile_options(Quake 
        PRIVATE
            -Wall -Wextra -Wpedantic -Wshadow -Wconversion
    )
endif()