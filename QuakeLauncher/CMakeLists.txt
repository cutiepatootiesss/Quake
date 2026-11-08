cmake_minimum_required(VERSION 4.3)

project(QuakeLauncher VERSION 1.0.0 LANGUAGES CXX)

include(FetchContent)

# VulkanHeaders
FetchContent_Declare(
    VulkanHeaders
    GIT_REPOSITORY 
        https://github.com/KhronosGroup/Vulkan-Headers.git
    GIT_TAG        
        vulkan-sdk-1.4.363.0
    GIT_SHALLOW    
        TRUE
    EXCLUDE_FROM_ALL
)

# Volk
FetchContent_Declare(
    volk
    GIT_REPOSITORY 
        https://github.com/zeux/volk.git
    GIT_TAG        
        vulkan-sdk-1.4.363.0
    GIT_SHALLOW    
        TRUE
    EXCLUDE_FROM_ALL
)

# VulkanMemoryAllocator
FetchContent_Declare(
    VulkanMemoryAllocator
    GIT_REPOSITORY
        https://github.com/GPUOpen-LibrariesAndSDKs/VulkanMemoryAllocator.git
    GIT_TAG
        v3.1.0
    GIT_SHALLOW
        TRUE
    EXCLUDE_FROM_ALL
    SYSTEM
)
set(VMA_BUILD_SAMPLES OFF CACHE BOOL "" FORCE)

# SDL
FetchContent_Declare(
    SDL3
    GIT_REPOSITORY 
        https://github.com/libsdl-org/SDL.git
    GIT_TAG        
        release-3.4.20
    GIT_SHALLOW    
        TRUE
    EXCLUDE_FROM_ALL
)

# glslang
FetchContent_Declare(
    glslang
    GIT_REPOSITORY
        https://github.com/KhronosGroup/glslang.git
    GIT_TAG
        vulkan-sdk-1.4.363.0
    GIT_SHALLOW
        TRUE
    EXCLUDE_FROM_ALL
    SYSTEM
)

# These require additional dependencies and... not really needed for us here.
set(ENABLE_OPT OFF CACHE BOOL "" FORCE)
set(ENABLE_GLSLANG_BINARIES OFF CACHE BOOL "" FORCE)
 
FetchContent_MakeAvailable(VulkanHeaders)
set(VULKAN_HEADERS_INSTALL_DIR "${vulkanheaders_SOURCE_DIR}" CACHE PATH "" FORCE)
FetchContent_MakeAvailable(volk SDL3 glslang VulkanMemoryAllocator)

add_executable(QuakeLauncher)
target_sources(QuakeLauncher
    PRIVATE
        Main.cpp
        DynamicLibrary/Interface.hpp
        Renderer/VulkanBoilerplate/VulkanInstance.hpp
        Renderer/VulkanBoilerplate/VulkanInstance.cpp
        Renderer/VulkanBoilerplate/VulkanDevice.hpp
        Renderer/VulkanBoilerplate/VulkanDevice.cpp
        Renderer/VulkanBoilerplate/VulkanSurface.hpp
        Renderer/VulkanBoilerplate/VulkanSurface.cpp
        Renderer/VulkanBoilerplate/VulkanSwapchain.hpp
        Renderer/VulkanBoilerplate/VulkanSwapchain.cpp
        Renderer/VulkanBoilerplate/VulkanSync.hpp
        Renderer/VulkanBoilerplate/VulkanSync.cpp
        Renderer/VulkanBoilerplate/VulkanCmd.hpp
        Renderer/VulkanBoilerplate/VulkanCmd.cpp
        Renderer/VulkanBoilerplate/VulkanPipeline.hpp
        Renderer/VulkanBoilerplate/VulkanPipeline.cpp
        Renderer/VulkanBoilerplate/VulkanAllocator.hpp
        Renderer/VulkanBoilerplate/VulkanAllocator.cpp
        Renderer/VulkanBoilerplate/VulkanImage.hpp
        Renderer/VulkanBoilerplate/VulkanImage.cpp
        Renderer/VulkanBoilerplate/VulkanBuffer.hpp
        Renderer/VulkanBoilerplate/VulkanBuffer.cpp
        Renderer/VulkanBoilerplate/VmaImplementation.cpp
        Renderer/VulkanCommandListImpl.hpp
        Renderer/VulkanCommandListImpl.cpp
        Renderer/VulkanDeviceImpl.hpp
        Renderer/VulkanDeviceImpl.cpp
        Renderer/VulkanPipelineImpl.hpp
        Renderer/VulkanTextureImpl.hpp
        Renderer/VulkanBufferImpl.hpp
        Renderer/ShaderCompiler.hpp
        Renderer/ShaderCompiler.cpp
        SDL/SDL.cpp
        SDL/SDL.hpp
)
target_link_libraries(QuakeLauncher PRIVATE SDL3::SDL3 volk glslang SPIRV glslang-default-resource-limits GPUOpen::VulkanMemoryAllocator)
# If this is not set then any file that includes vulkan/vulkan.h without going through volk.h willp produce 
# symbol conflicts
target_compile_definitions(QuakeLauncher PRIVATE VK_NO_PROTOTYPES VMA_STATIC_VULKAN_FUNCTIONS=0 VMA_DYNAMIC_VULKAN_FUNCTIONS=1)

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