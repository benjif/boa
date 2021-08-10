# Boa

Boa is my toy game engine.

## Directory Structure

* boa
    * ctl - control (keyboard, mouse, etc.)
    * ecs - entity component system (naive implementation at this point)
    * gfx - graphics (vulkan renderer and more)
    * ngn - engine (engine ui, main loop, movement, and more)
    * phy - physics (physics-related stuff using bullet3)
    * utl - utility (misc.)
    * (future) lua - move scripting stuff out of ngn
    * (future) gme - for running world files in game-mode
    * (future) afx - audio

## Dependencies

* ImGui
* ImGuizmo
* ImGuiColorTextEdit
* LuaJIT
* fmt
* GLFW
* GLM
* TinyGLTF
* VulkanMemoryAllocator
* Bullet3
* (Future) SoLoud

## Building

Note that I use `FetchContent` for all dependencies except Vulkan, which requires a system-wide installation.

```
mkdir cmake
cd cmake && cmake ..
ninja all
```
