#pragma once

#include "plugin_api.h"
#include "game_logic_interface.h"
#include <unordered_map>
#include <unordered_set>
#include <string>
#include <memory>
#include <functional>
#include <vector>

namespace GameEngine {
class GameLogicPluginManager : public IPluginManager {
private:
    std::unordered_map<std::string, void*> loadedLibraries;
    std::unordered_map<std::string, std::function<std::unique_ptr<IGameLogic>() >> gameLogicFactories;
    std::unordered_set<std::string> allowedPaths;
    bool securityEnabled = true;

public:
    GameLogicPluginManager();
    ~GameLogicPluginManager();

    // IPluginManager implementation
    void registerGameLogicFactory(const std::string& name, std::function<std::unique_ptr<IGameLogic>()> factory) override;

    // Plugin management
    bool loadPlugin(const std::string& path, const std::string& name);
    bool unloadPlugin(const std::string& name);
    bool loadProjectPlugins(const std::string& projectPath);

    // Game logic creation
    std::unique_ptr<IGameLogic> createGameLogic(const std::string& name);

    // Security and lifecycle
    void disableSecurity();
    std::vector<std::string> getLoadedPlugins() const;
    void clearAll();

private:
    bool isPathAllowed(const std::string& path);
    bool loadPackageFromProject(const std::string& projectPath, const std::string& packageName);
};

} // namespace GameEngine
