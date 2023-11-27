cmake_minimum_required(VERSION 3.25)
project("Vixen")

set(CMAKE_INCLUDE_CURRENT_DIR ON)
set(CMAKE_CXX_STANDARD 23)
set(CMAKE_CXX_STANDARD_REQUIRED ON)
set(CMAKE_RUNTIME_OUTPUT_DIRECTORY ${CMAKE_BINARY_DIR}/bin)
set(CMAKE_LIBRARY_OUTPUT_DIRECTORY ${CMAKE_BINARY_DIR}/bin)
set(CMAKE_EXPORT_COMPILE_COMMANDS ON)
# Statically linking to libstd with C++23 causes a linker bug
# https://gcc.gnu.org/bugzilla/show_bug.cgi?id=110572
# set(CMAKE_EXE_LINKER_FLAGS "${CMAKE_EXE_LINKER_FLAGS} -static -static-libstdc++ -static-libgcc")

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
else ()
    set(CMAKE_EXE_LINKER_FLAGS "${CMAKE_EXE_LINKER_FLAGS} -mwindows")
endif ()

add_subdirectory(src/engine)

if (ENABLE_EDITOR)
    add_subdirectory(src/editor)
endif ()
