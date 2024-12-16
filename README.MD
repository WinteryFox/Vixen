# Vixen

Vixen is a 3D physically-based game engine. This project is my own attempt at making a game engine
in my effort to master all 3 major graphics APIs; OpenGL, Vulkan and DirectX12. All of these APIs
are to be supported by Vixen, and they are to be switched between with a simple restart.

## Modules

The engine consists of several modules, primarily the runtime core abstraction and its platform dependant
implementations.

- [Runtime core](/src/engine/core)
  - [Vulkan implementation](/src/engine/core/platform/vulkan)

The editor application consists of only its own module.

- [Editor](/src/editor)
