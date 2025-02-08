# Vixen

Vixen is a 3D physically-based game engine. This project is my own attempt at making a game engine
in my effort to master all 3 major graphics APIs; OpenGL, Vulkan and DirectX12. All of these APIs
are to be supported by Vixen, and they are to be switched between with a simple restart.

## Modules

The engine consists of several modules, primarily the runtime core abstraction and its platform dependant
implementations.

- [Runtime core](core)
  - [Vulkan implementation](platform/vulkan)
  - [OpenGL implementation](platform/opengl)
  - [Direct3D implementation](platform/d3d12)

The editor application consists of only its own set of module.

- [Editor](editor)

# Dependencies

## Windows

Download and install the [Vulkan SDK](https://vulkan.lunarg.com/).

## Debian & Ubuntu

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
    vulkan-utility-libraries \
    vulkan-validationlayers
```
