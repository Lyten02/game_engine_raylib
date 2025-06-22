# Game Engine RayLib

[![CI](https://github.com/Lyten02/game_engine_raylib/actions/workflows/ci.yml/badge.svg)](https://github.com/Lyten02/game_engine_raylib/actions/workflows/ci.yml)
[![Release](https://github.com/Lyten02/game_engine_raylib/actions/workflows/release.yml/badge.svg)](https://github.com/Lyten02/game_engine_raylib/actions/workflows/release.yml)

A modern game engine built with ECS architecture, RayLib rendering, and comprehensive CLI support.

## Features
- Entity Component System (ECS) architecture using EnTT
- Cross-platform support (Linux, macOS, Windows)
- Command-line interface for automation
- Integrated project management system
- Hot-reload support for rapid development

## Build Status
Our CI/CD pipeline ensures code quality across all platforms:
- **Build & Test**: Automated builds and tests on every push
- **Code Quality**: Static analysis with clang-tidy and cppcheck
- **Releases**: Automated binary releases for all platforms

## Quick Start
```bash
cd GameEngine
./rebuild_smart.sh
./game_engine
```

For more details, see the [workflow documentation](.github/workflows/README.md).