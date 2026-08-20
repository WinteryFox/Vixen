cmake_minimum_required(VERSION 3.25)
project("Vixen" LANGUAGES C CXX)

set(ENGINE_NAME "Vixen")
set(ENGINE_VERSION_MAJOR 1)
set(ENGINE_VERSION_MINOR 0)
set(ENGINE_VERSION_PATCH 0)
set(CMAKE_INCLUDE_CURRENT_DIR ON)
set(CMAKE_RUNTIME_OUTPUT_DIRECTORY ${CMAKE_BINARY_DIR}/bin)
set(CMAKE_LIBRARY_OUTPUT_DIRECTORY ${CMAKE_BINARY_DIR}/bin)
set(CMAKE_EXPORT_COMPILE_COMMANDS ON)

function(vixen_configure_target target)
    set_target_properties(
            ${target}
            PROPERTIES
            CXX_STANDARD 23
            CXX_STANDARD_REQUIRED YES
            CXX_EXTENSIONS NO
    )
    target_compile_options(
            ${target}
            PRIVATE
            $<$<COMPILE_LANG_AND_ID:CXX,GNU,Clang,AppleClang>:-Wall;-Wextra;-Wpedantic>
            $<$<COMPILE_LANG_AND_ID:CXX,MSVC>:/W4;/permissive->
    )
    target_compile_definitions(${target} PRIVATE $<$<CONFIG:Debug>:DEBUG_ENABLED>)
endfunction()

if (POLICY CMP0025)
    cmake_policy(SET CMP0025 NEW)
endif ()

set(SKIP_INSTALL_ALL ON)

include(FetchContent)
find_package(PkgConfig REQUIRED)
find_package(Threads REQUIRED)

option(ENABLE_VULKAN "Enable or disable Vulkan support" ON)
option(ENABLE_D3D12 "Enable or disable D3D12 support" OFF)
option(ENABLE_OPENGL "Enable or disable OpenGL support" OFF)
option(ENABLE_TESTS "Enable or disable building of the tests" OFF)
option(ENABLE_EDITOR "Enable or disable the editor" ON)

set(BUILD_SHARED_LIBS OFF)
set(GLFW_BUILD_SHARED_LIBS OFF)
set(GLFW_LIBRARY_TYPE STATIC)
set(GLFW_BUILD_EXAMPLES OFF)
set(GLFW_BUILD_TESTS OFF)
set(GLFW_BUILD_DOCS OFF)
set(GLFW_INSTALL OFF)
add_subdirectory(thirdparty/glfw)
set(ENABLE_OPT "OFF")
add_subdirectory(thirdparty/glslang)
add_subdirectory(thirdparty/SPIRV-Cross)
add_subdirectory(thirdparty/glm)
# TODO: Temporary fix for https://gcc.gnu.org/bugzilla/show_bug.cgi?id=110572
set(SPDLOG_BUILD_SHARED "ON")
add_subdirectory(thirdparty/spdlog)
set(ASSIMP_WARNINGS_AS_ERRORS "OFF")
set(ASSIMP_INSTALL "OFF")
set(ASSIMP_BUILD_TESTS "OFF")
set(BUILD_SHARED_LIBS "OFF")
add_subdirectory(thirdparty/assimp)

add_subdirectory(core)

if (ENABLE_VULKAN)
    add_subdirectory(thirdparty/VulkanMemoryAllocator)
    if (WIN32)
        set(VOLK_STATIC_DEFINES VK_USE_PLATFORM_WIN32_KHR)
    elseif (APPLE)
        set(VOLK_STATIC_DEFINES VK_USE_PLATFORM_METAL_EXT)
    endif ()
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
