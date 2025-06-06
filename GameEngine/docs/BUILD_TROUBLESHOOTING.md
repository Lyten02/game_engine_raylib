# Build System Troubleshooting

## Issue: "Failed to open config file: ProjectName_config.json"

### Problem
The compiled game looks for `ProjectName_config.json` but the build system creates `game_config.json`.

### Temporary Solution
Create a copy or symlink:
```bash
cd output/YourProject/bin
cp game_config.json YourProject_config.json
# OR
ln -s game_config.json YourProject_config.json
```

Then run the game:
```bash
./YourProject
```

### Permanent Solution
The build system has been updated to create both files automatically. Rebuild your project:
```bash
# In engine console
project.build.sync
```

## Issue: Game doesn't start

### Check working directory
The game expects to find its resources relative to the executable:
```
bin/
├── YourProject         # executable
├── game_config.json    # required
├── assets/            # required if game uses assets
└── scenes/            # required
    └── main.json      # or your main scene
```

### Run from correct directory
Always run the game from its bin directory:
```bash
cd output/YourProject/bin
./YourProject
```

## Issue: Build takes too long

First build downloads dependencies (RayLib, EnTT, etc.) which can take 1-2 minutes. Subsequent builds are much faster.

## Issue: Build fails with CMake errors

1. Check CMake version: `cmake --version` (need 3.20+)
2. Check C++ compiler supports C++20
3. Check internet connection (for downloading dependencies)
4. Try verbose build to see errors:
   ```bash
   cd output/YourProject/build
   cmake --build . --verbose
   ```