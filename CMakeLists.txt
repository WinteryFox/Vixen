cmake_minimum_required(VERSION 3.24)
project("Vixen")

set(CMAKE_CXX_STANDARD 20)
set(CMAKE_RUNTIME_OUTPUT_DIRECTORY ${CMAKE_BINARY_DIR}/bin)
set(CMAKE_LIBRARY_OUTPUT_DIRECTORY ${CMAKE_BINARY_DIR}/bin)
if (WIN32)
    set(CMAKE_EXE_LINKER_FLAGS "${CMAKE_EXE_LINKER_FLAGS} -static-libstdc++ -static-libgcc")
endif ()

if (POLICY CMP0025)
    cmake_policy(SET CMP0025 NEW)
endif ()

set(SKIP_INSTALL_ALL ON)

find_package(PkgConfig REQUIRED)

option(COMPILE_EDITOR "Enable or disable the compilation of the editor" ON)

add_definitions(-DGLFW_INCLUDE_NONE)
if (CMAKE_BUILD_TYPE MATCHES "Debug")
    add_definitions(-DDEBUG)
endif ()

add_subdirectory(engine)

if (COMPILE_EDITOR)
    add_subdirectory(editor)
endif ()
