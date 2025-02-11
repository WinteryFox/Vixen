cmake_minimum_required(VERSION 3.25)
project("Vixen")

set(ENGINE_NAME "Vixen")
set(ENGINE_VERSION_MAJOR 1)
set(ENGINE_VERSION_MINOR 0)
set(ENGINE_VERSION_PATCH 0)
if (CMAKE_BUILD_TYPE MATCHES "Release")
    set(ENGINE_VERSION_STATUS "stable")
else ()
    set(ENGINE_VERSION_STATUS "dev")
endif ()
set(ENGINE_VERSION "${ENGINE_VERSION_MAJOR}.${ENGINE_VERSION_MINOR}.${ENGINE_VERSION_PATCH}.${ENGINE_VERSION_STATUS}")

add_compile_definitions(
        ENGINE_NAME="${ENGINE_NAME}"
        ENGINE_VERSION_MAJOR=${ENGINE_VERSION_MAJOR}
        ENGINE_VERSION_MINOR=${ENGINE_VERSION_MINOR}
        ENGINE_VERSION_PATCH=${ENGINE_VERSION_PATCH}
        ENGINE_VERSION_STATUS="${ENGINE_VERSION_STATUS}"
        ENGINE_VERSION="${ENGINE_VERSION}"
)

set(CMAKE_INCLUDE_CURRENT_DIR ON)
set(CMAKE_CXX_STANDARD 20)
set(CMAKE_CXX_STANDARD_REQUIRED ON)
set(CMAKE_RUNTIME_OUTPUT_DIRECTORY ${CMAKE_BINARY_DIR}/bin)
set(CMAKE_LIBRARY_OUTPUT_DIRECTORY ${CMAKE_BINARY_DIR}/bin)
set(CMAKE_EXPORT_COMPILE_COMMANDS ON)
add_link_options(-static -static-libstdc++ -static-libgcc)
add_compile_options(-Wall -Wextra -Wpedantic)
if (CMAKE_BUILD_TYPE MATCHES "Release")
    add_link_options(-O2)
endif ()

if (POLICY CMP0025)
    cmake_policy(SET CMP0025 NEW)
endif ()

set(SKIP_INSTALL_ALL ON)

include(FetchContent)
find_package(PkgConfig REQUIRED)

option(ENABLE_VULKAN "Enable or disable Vulkan support" ON)
option(ENABLE_D3D12 "Enable or disable D3D12 support" OFF)
option(ENABLE_OPENGL "Enable or disable OpenGL support" OFF)
option(ENABLE_TESTS "Enable or disable building of the tests" ON)
option(ENABLE_EDITOR "Enable or disable the editor" ON)

if (CMAKE_BUILD_TYPE MATCHES "Debug")
    add_compile_definitions(DEBUG_ENABLED)
elseif (CMAKE_BUILD_TYPE MATCHES "Release")
    if (WIN32)
        add_link_options(-mwindows)
    endif ()
endif ()

if (ENABLE_VULKAN)
    add_compile_definitions(VULKAN_ENABLED)
endif ()

if (ENABLE_D3D12)
    add_compile_definitions(D3D12_ENABLED)
endif ()

if (ENABLE_OPENGL)
    add_compile_definitions(OPENGL_ENABLED)
endif ()

add_subdirectory(thirdparty/glfw)
set(ENABLE_OPT "OFF")
add_subdirectory(thirdparty/glslang)
add_subdirectory(thirdparty/SPIRV-Cross)
add_subdirectory(thirdparty/glm)
add_subdirectory(thirdparty/spdlog)
set(ASSIMP_WARNINGS_AS_ERRORS "OFF")
set(ASSIMP_INSTALL "OFF")
set(ASSIMP_BUILD_TESTS "OFF")
set(BUILD_SHARED_LIBS "OFF")
add_subdirectory(thirdparty/assimp)

add_subdirectory(core)

if (ENABLE_VULKAN)
    add_subdirectory(thirdparty/VulkanMemoryAllocator)
    add_subdirectory(thirdparty/volk)
    add_subdirectory(platform/vulkan)
endif ()

if (ENABLE_D3D12)
    add_subdirectory(platform/d3d12)
endif ()

if (ENABLE_OPENGL)
    add_subdirectory(platform/opengl)
endif ()

if (ENABLE_EDITOR)
    add_subdirectory(editor)
endif ()
