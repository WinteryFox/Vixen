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
option(ENABLE_DOCUMENTATION "Enable the Doxygen API documentation target" OFF)

if (ENABLE_DOCUMENTATION)
    find_package(Doxygen REQUIRED)

    set(DOXYGEN_PROJECT_NAME ${ENGINE_NAME})
    set(DOXYGEN_PROJECT_NUMBER "${ENGINE_VERSION_MAJOR}.${ENGINE_VERSION_MINOR}.${ENGINE_VERSION_PATCH}")
    set(DOXYGEN_PROJECT_BRIEF "Vixen API documentation")
    set(DOXYGEN_OUTPUT_DIRECTORY ${CMAKE_BINARY_DIR}/docs)
    set(DOXYGEN_STRIP_FROM_PATH ${CMAKE_SOURCE_DIR})
    set(DOXYGEN_USE_MDFILE_AS_MAINPAGE ${CMAKE_SOURCE_DIR}/README.MD)
    set(DOXYGEN_FILE_PATTERNS "*.h" "*.cpp" "*.md")
    set(
            DOXYGEN_EXCLUDE_PATTERNS
            "*/thirdparty/*"
            "*/cmake-build-*/*"
            "*/.git/*"
    )
    set(
            DOXYGEN_PREDEFINED
            "VULKAN_ENABLED=1"
            "D3D12_ENABLED=1"
            "OPENGL_ENABLED=1"
    )
    set(DOXYGEN_RECURSIVE YES)
    set(DOXYGEN_EXTRACT_PRIVATE NO)
    set(DOXYGEN_EXTRACT_STATIC YES)
    set(DOXYGEN_JAVADOC_AUTOBRIEF YES)
    set(DOXYGEN_MARKDOWN_SUPPORT YES)
    set(DOXYGEN_AUTOLINK_SUPPORT YES)
    set(DOXYGEN_GENERATE_HTML YES)
    set(DOXYGEN_GENERATE_TREEVIEW YES)
    set(DOXYGEN_GENERATE_LATEX NO)
    set(DOXYGEN_WARN_IF_UNDOCUMENTED NO)
    set(DOXYGEN_WARN_IF_DOC_ERROR YES)
    set(DOXYGEN_WARN_LOGFILE ${CMAKE_BINARY_DIR}/docs/doxygen-warnings.log)

    doxygen_add_docs(
            VixenDocs
            ${CMAKE_SOURCE_DIR}/core
            ${CMAKE_SOURCE_DIR}/editor
            ${CMAKE_SOURCE_DIR}/platform/vulkan
            ${CMAKE_SOURCE_DIR}/platform/opengl
            ${CMAKE_SOURCE_DIR}/platform/d3d12
            ${CMAKE_SOURCE_DIR}/README.MD
            COMMENT "Generate Vixen API documentation"
    )
endif ()

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
