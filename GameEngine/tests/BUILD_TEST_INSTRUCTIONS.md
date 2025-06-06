# Build System Test Instructions

The build system uses asynchronous compilation which doesn't work well in headless/batch mode.
Therefore, build system testing must be done manually.

## Manual Test Steps

1. **Start the engine in interactive mode:**
   ```bash
   ./game
   ```

2. **Open the console by pressing F1**

3. **Run these commands in sequence:**
   ```
   # Create a new project
   project.create TestGame
   
   # Open the project
   project.open TestGame
   
   # Create a scene
   scene.create main
   
   # Add some entities
   entity.create Player
   entity.create Enemy
   
   # Save the scene
   scene.save main
   
   # Build the project
   project.build
   ```

4. **Wait for build completion**
   - You should see progress messages in the console
   - Build typically takes 30-60 seconds on first run (downloads dependencies)
   - Subsequent builds are much faster

5. **Verify the output:**
   - Check that executable exists: `output/TestGame/bin/TestGame` (or `.exe` on Windows)
   - Check that assets were copied: `output/TestGame/bin/assets/`
   - Check that scenes were copied: `output/TestGame/bin/scenes/main.json`
   - Check that config exists: `output/TestGame/bin/game_config.json`

6. **Run the built game:**
   ```
   project.run
   ```
   Or from terminal:
   ```bash
   ./output/TestGame/bin/TestGame
   ```

## Expected Results

- ✅ Project builds without errors
- ✅ Executable is created and runs
- ✅ Game window opens showing the scene
- ✅ All assets and scenes are properly packaged

## Troubleshooting

- If build fails, check console output for CMake errors
- Ensure CMake 3.20+ is installed
- On first build, internet connection is required to download dependencies
- Check that you have a C++20 compatible compiler