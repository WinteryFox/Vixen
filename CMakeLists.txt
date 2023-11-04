cmake_minimum_required(VERSION 3.25)
project("Vixen")

set(CMAKE_INCLUDE_CURRENT_DIR ON)
set(CMAKE_CXX_STANDARD 20)
set(CMAKE_CXX_STANDARD_REQUIRED ON)
set(CMAKE_RUNTIME_OUTPUT_DIRECTORY ${CMAKE_BINARY_DIR}/bin)
set(CMAKE_LIBRARY_OUTPUT_DIRECTORY ${CMAKE_BINARY_DIR}/bin)
if (WIN32)
    set(CMAKE_EXE_LINKER_FLAGS "${CMAKE_EXE_LINKER_FLAGS} -static-libstdc++ -static-libgcc")
endif ()

if (POLICY CMP0025)
    cmake_policy(SET CMP0025 NEW)
endif ()

set(SKIP_INSTALL_ALL ON)

include(FetchContent)
find_package(PkgConfig REQUIRED)

option(ENABLE_VULKAN "Enable or disable Vulkan support" ON)
option(ENABLE_OPENGL "Enable or disable OpenGL support" ON)
option(ENABLE_TESTS "Enable or disable building of the tests" ON)
option(ENABLE_EDITOR "Enable or disable the compilation of the editor" ON)

if (CMAKE_BUILD_TYPE MATCHES "Debug")
    add_definitions(-DDEBUG)
endif ()

add_subdirectory(engine)

if (ENABLE_EDITOR)
    add_subdirectory(editor)
endif ()
