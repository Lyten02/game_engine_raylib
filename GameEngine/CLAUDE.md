# CLAUDE.md

This file provides guidance to Claude Code (claude.ai/code) when working with code in this repository.

## Build Commands

```bash
cd GameEngine
mkdir -p build
cd build
cmake ..
make
./game
```

Requirements:
- CMake 3.20+
- C++20 compiler
- Dependencies are auto-fetched: RayLib 5.0, spdlog v1.14.1, EnTT v3.13.2, GLM 1.0.1

## Architecture Overview

This is an Entity Component System (ECS) game engine using EnTT. Key architectural patterns:

### Core Systems
- **Engine** (src/engine.h): Main class managing initialization, game loop (60 FPS), and shutdown
- **Scene** (src/scene/scene.h): Base class with EnTT registry, lifecycle methods (onCreate, onUpdate, onDestroy)
- **RenderSystem** (src/systems/render_system.h): Processes entities with Transform+Sprite components

### Component Model
- **TransformComponent**: 3D position, rotation, scale using RayLib Vector3
- **Sprite**: 2D rendering with Texture2D, source rectangle, tint color

### Engine Flow
1. Initialize: Create RayLib window → Initialize systems → Create scene
2. Runtime: Update delta time → Update scene → Clear screen → Run render system
3. Shutdown: Destroy scene → Shutdown systems → Close window

### Key Integration Points
- RayLib handles windowing, rendering, and input
- EnTT provides the ECS framework
- spdlog for logging (initialized at debug level)
- MathUtils (src/math/math_utils.h) converts between GLM and RayLib types

When implementing features, follow the ECS pattern: data in components, logic in systems, scene manages entity lifecycle.