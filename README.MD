# Vixen

Vixen is a 3D physically-based game engine. This project is my own attempt at making a game engine
in my effort to master all 3 major graphics APIs; OpenGL, Vulkan and DirectX12. All of these APIs
are to be supported by Vixen, and they are to be switched between with a simple restart.

## Goals

- Implement the 3 major APIs commonly used in modern rendering systems.
    1. Vulkan 1.3
        * Full Vulkan 1.0 compatability
        * MoltenVK
    2. Direct3D 12
    3. OpenGL 4.5
        * WebGL
- Implement a simple editor application, allowing users to create their own scenes and compile to a standalone game
  executable.
- Implement a scripting engine.
- Implement a physics engine, using a library.
- Implement a sound engine, using a library.
- Create at least one game with Vixen with a minimum gameplay loop of 1 minutes, before the end of my Master.
    - This game will focus on graphical effects, rather than gameplay, hence the short runtime.

## Modules

The engine consists of several modules, primarily the runtime core abstraction and its distinct rendering API
implementations.

### Runtime

The runtime abstraction for Vixen lives under the [core](core) directory. Their implementations are in their
respective [platform](platform) directories.

- [Vulkan implementation](platform/vulkan)
- [OpenGL implementation](platform/opengl)
- [Direct3D implementation](platform/d3d12)

### Editor

The editor application lives under the [editor](editor) directory.

# Contributing

Clone the repository and its submodules.

```bash
git clone --recurse-submodules https://github.com/WinteryFox/Vixen.git
```

## Dependencies

### Windows

Download and install the [Vulkan SDK](https://vulkan.lunarg.com/), [CMake](https://cmake.org/), and a compiler toolchain
of your choice,
like [MSYS2](https://www.msys2.org/).

### Linux

Below is a list of one-liners for the different distributions to install all the required dependencies to build Vixen.

#### Debian & Ubuntu

```
sudo apt update
sudo apt install -y \
    build-essential \
    cmake \
    libxkbcommon-dev \
    libx11-dev \
    libxcursor-dev \
    libxinerama-dev \
    libgl1-mesa-dev \
    libxi-dev \
    libxrandr-dev \
    libwayland-dev \
    libpulse-dev \
    libvulkan-dev \
    vulkan-tools \
    vulkan-utility-libraries-dev \
    vulkan-validationlayers
```
