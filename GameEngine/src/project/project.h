#pragma once

#include <string>
#include <vector>
#include <memory>
#include "nlohmann/json.hpp"

namespace GameEngine {

class Project {
private:
    std::string name;
    std::string path;
    std::string version = "1.0.0";
    std::string startScene;
    std::vector<std::string> scenes;
    nlohmann::json metadata;

public:
    bool load(const std::string& projectPath);
    bool save();
    bool createScene(const std::string& sceneName);
    bool deleteScene(const std::string& sceneName);
    std::vector<std::string> getScenes() const;
    const std::string& getName() const;
    const std::string& getPath() const;
    
    // Start scene methods
    bool hasStartScene() const { return !startScene.empty(); }
    const std::string& getStartScene() const { return startScene; }
    void setStartScene(const std::string& sceneName) { startScene = sceneName; }
    
    // Game logic
    std::string getGameLogic() const;
};

} // namespace GameEngine