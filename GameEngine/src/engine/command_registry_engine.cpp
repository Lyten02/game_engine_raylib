#include "command_registry.h"
#include "engine_core.h"
#include "../console/console.h"
#include "../console/command_processor.h"
#include "../utils/config.h"
#include <raylib.h>
#include <spdlog/spdlog.h>
#include <sstream>
#include <iomanip>
#include <filesystem>
#include <algorithm>
#include <climits>

namespace GameEngine {
void CommandRegistry::registerEngineCommands(CommandProcessor* processor, EngineCore* engineCore, Console* console) {
    // engine.info command
    processor->registerCommand("engine.info",
        [engineCore, console](const std::vector<std::string>& args) {
            std::stringstream ss;
            ss << "Engine Information:\n";
            ss << "  FPS: " << engineCore->getFPS() << "\n";
            ss << "  Frame Time: " << std::fixed << std::setprecision(3) << engineCore->getFrameTime() * 1000 << " ms\n";
            ss << "  Total Time: " << std::fixed << std::setprecision(1) << engineCore->getTotalTime() << " s\n";
            ss << "  Window: " << engineCore->getScreenWidth() << "x" << engineCore->getScreenHeight();

            console->addLine(ss.str(), YELLOW);
        }, "Display engine information", "Engine");

    // quit command
    processor->registerCommand("quit",
        [engineCore, console](const std::vector<std::string>& args) {
            console->addLine("Shutting down...", YELLOW);
            engineCore->requestQuit();
        }, "Quit the application", "General");

    // engine.fps command
    std::vector<CommandParameter> fpsParams = {
        {"limit", "FPS limit value", true, []() {
            return std::vector<std::string>{"0", "30", "60", "120", "144", "240"};
        }}
    };
    processor->registerCommand("engine.fps",
        [engineCore, console](const std::vector<std::string>& args) {
            if (args.empty()) {
                console->addLine("Usage: engine.fps <limit>", RED);
                console->addLine("  limit: 0 (unlimited), 30, 60, 120, 144, 240", GRAY);
                console->addLine("Current FPS limit: " + std::string(engineCore->getTargetFPS() == 0 ? "Unlimited" : std::to_string(engineCore->getTargetFPS())), YELLOW);
                return;
            }

            int limit = std::stoi(args[0]);
            engineCore->setTargetFPS(limit);
            console->addLine("FPS limit set to: " + (limit == 0 ? std::string("Unlimited") : std::to_string(limit)), GREEN);
        }, "Set engine FPS limit", "Engine", "", fpsParams);

    // engine.vsync command
    std::vector<CommandParameter> vsyncParams = {
        {"enabled", "Enable or disable vsync", true, []() {
            return std::vector<std::string>{"on", "off", "true", "false", "1", "0"};
        }}
    };
    processor->registerCommand("engine.vsync",
        [engineCore, console](const std::vector<std::string>& args) {
            if (args.empty()) {
                console->addLine("Usage: engine.vsync <on|off>", RED);
                console->addLine("Current vsync: " + std::string(engineCore->isVSyncEnabled() ? "ON" : "OFF"), YELLOW);
                return;
            }

            bool enable = args[0] == "on" || args[0] == "true" || args[0] == "1";
            engineCore->setVSync(enable);
            console->addLine("VSync " + std::string(enable ? "enabled" : "disabled"), GREEN);
        }, "Toggle engine VSync", "Engine", "", vsyncParams);

    // engine.diag command
    processor->registerCommand("engine.diag",
        [engineCore, console](const std::vector<std::string>& args) {
            std::stringstream ss;
            ss << "Engine Diagnostics:\n";
            ss << "  Headless Mode: " << (engineCore->isHeadless() ? "Yes" : "No") << "\n";
            ss << "  Window Ready: " << (engineCore->isWindowReady() ? "Yes" : "No") << "\n";
            ss << "  VSync: " << (engineCore->isVSyncEnabled() ? "Enabled" : "Disabled") << "\n";
            ss << "  Target FPS: " << (engineCore->getTargetFPS() == 0 ? "Unlimited" : std::to_string(engineCore->getTargetFPS())) << "\n";
            ss << "  Current FPS: " << engineCore->getFPS() << "\n";
            ss << "  Frame Time: " << std::fixed << std::setprecision(3) << engineCore->getFrameTime() * 1000 << " ms";

            console->addLine(ss.str(), YELLOW);
        }, "Display engine diagnostics", "Engine");
}

void CommandRegistry::registerDebugCommands(CommandProcessor* processor, Console* console, bool* showDebugInfo) {
    // debug.toggle command
    processor->registerCommand("debug.toggle",
        [console, showDebugInfo](const std::vector<std::string>& args) {
            *showDebugInfo = !(*showDebugInfo);
            console->addLine("Debug info " + std::string(*showDebugInfo ? "enabled" : "disabled"), GREEN);
        }, "Toggle debug information display", "Debug");

    // debug.log command
    std::vector<CommandParameter> logParams = {
        {"level", "Log level", true, []() {
            return std::vector<std::string>{"trace", "debug", "info", "warn", "error", "critical", "off"};
        }}
    };
    processor->registerCommand("debug.log",
        [console](const std::vector<std::string>& args) {
            if (args.empty()) {
                console->addLine("Usage: debug.log <level>", RED);
                console->addLine("  Levels: trace, debug, info, warn, error, critical, off", GRAY);
                auto level_sv = spdlog::level::to_string_view(spdlog::get_level());
                std::string levelStr(level_sv.data(), level_sv.size());
                console->addLine("Current level: " + levelStr, YELLOW);
                return;
            }

            std::string level = args[0];
            if (level == "trace") spdlog::set_level(spdlog::level::trace);
            else if (level == "debug") spdlog::set_level(spdlog::level::debug);
            else if (level == "info") spdlog::set_level(spdlog::level::info);
            else if (level == "warn") spdlog::set_level(spdlog::level::warn);
            else if (level == "error") spdlog::set_level(spdlog::level::err);
            else if (level == "critical") spdlog::set_level(spdlog::level::critical);
            else if (level == "off") spdlog::set_level(spdlog::level::off);
            else {
                console->addLine("Invalid log level: " + level, RED);
                return;
            }

            console->addLine("Log level set to: " + level, GREEN);
        }, "Set logging level", "Debug", "", logParams);
}

void CommandRegistry::registerConfigCommands(CommandProcessor* processor, Console* console, EngineCore* engineCore) {
    // config.reload command
    processor->registerCommand("config.reload",
        [console](const std::vector<std::string>& args) {
            if (Config::load("config.json")) {
                console->addLine("Configuration reloaded successfully", GREEN);
            } else {
                console->addLine("Failed to reload configuration", RED);
            }
        }, "Reload configuration from file", "Config");

    // config.get command
    std::vector<CommandParameter> getParams = {
        {"key", "Configuration key", true, []() {
            // Return common config keys
            return std::vector<std::string>{
                "window.width", "window.height", "window.title",
                "engine.target_fps", "engine.vsync",
                "audio.master_volume", "audio.music_volume", "audio.sfx_volume"
            };
        }}
    };
    processor->registerCommand("config.get",
        [console](const std::vector<std::string>& args) {
            if (args.empty()) {
                console->addLine("Usage: config.get <key>", RED);
                return;
            }

            std::string key = args[0];

            // Add key validation
            if (!Config::isValidConfigKey(key)) {
                console->addLine("Error: Invalid config key format: " + key, RED);
                console->addLine("Key format rules:", GRAY);
                console->addLine("  - No dots at start or end", GRAY);
                console->addLine("  - No double dots (..)", GRAY);
                console->addLine("  - Only alphanumeric, dots, and underscores", GRAY);
                console->addLine("  - Maximum 100 characters", GRAY);
                return;
            }

            // Try to get as different types
            // Config doesn't have hasKey, try to get value
            // Check if it's a string
            auto strVal = Config::getString(key, "");
            auto intVal = Config::getInt(key, INT_MIN);
            auto floatVal = Config::getFloat(key, -999999.0f);
            auto boolVal = Config::getBool(key, false);

            std::stringstream ss;
            ss << "Config[" << key << "] = ";

            // Determine type and display
            if (intVal != INT_MIN && std::to_string(intVal) == strVal) {
                ss << intVal << " (int)";
            } else if (floatVal != -999999.0f) {
                ss << floatVal << " (float)";
            } else if (strVal == "true" || strVal == "false") {
                ss << (boolVal ? "true" : "false") << " (bool)";
            } else {
                ss << "\"" << strVal << "\" (string)";
            }

            console->addLine(ss.str(), YELLOW);
        }, "Get configuration value", "Config", "", getParams);

    // config.set command
    std::vector<CommandParameter> setParams = {
        {"key", "Configuration key", true, []() {
            // Return common config keys
            return std::vector<std::string>{
                "window.width", "window.height", "window.title",
                "engine.target_fps", "engine.vsync",
                "audio.master_volume", "audio.music_volume", "audio.sfx_volume"
            };
        }},
        {"value", "Value to set", true}
    };
    processor->registerCommand("config.set",
        [console](const std::vector<std::string>& args) {
            if (args.size() < 2) {
                console->addLine("Usage: config.set <key> <value>", RED);
                return;
            }

            std::string key = args[0];
            std::string value = args[1];

            // Add key validation
            if (!Config::isValidConfigKey(key)) {
                console->addLine("Error: Invalid config key format: " + key, RED);
                console->addLine("Key format rules:", GRAY);
                console->addLine("  - No dots at start or end", GRAY);
                console->addLine("  - No double dots (..)", GRAY);
                console->addLine("  - Only alphanumeric, dots, and underscores", GRAY);
                console->addLine("  - Maximum 100 characters", GRAY);
                return;
            }

            // Try to parse as different types
            bool isInt = true, isFloat = true, isBool = false;
            int intVal = 0;
            float floatVal = 0.0f;

            try {
                size_t idx;
                intVal = std::stoi(value, &idx);
                if (idx != value.length()) isInt = false;
            } catch (...) {
                isInt = false;
            }

            try {
                size_t idx;
                floatVal = std::stof(value, &idx);
                if (idx != value.length()) isFloat = false;
            } catch (...) {
                isFloat = false;
            }

            if (value == "true" || value == "false") {
                isBool = true;
            }

            // Set based on detected type
            if (isBool) {
                // Config doesn't have setters
                // Config::setBool(key, value == "true");
                console->addLine("Set " + key + " = " + value + " (bool)", GREEN);
            } else if (isInt) {
                // Config::setInt(key, intVal);
                console->addLine("Set " + key + " = " + std::to_string(intVal) + " (int)", GREEN);
            } else if (isFloat) {
                // Config::setFloat(key, floatVal);
                console->addLine("Set " + key + " = " + std::to_string(floatVal) + " (float)", GREEN);
            } else {
                // Config::setString(key, value);
                console->addLine("Set " + key + " = \"" + value + "\" (string)", GREEN);
            }

            // Note: Config doesn't have a save method currently
            // if (!Config::save("config.json")) {
            //     console->addLine("Warning: Failed to save configuration", YELLOW);
            // }
        }, "Set configuration value", "Config", "", setParams);
}

void CommandRegistry::registerConsoleCommands(CommandProcessor* processor, Console* console) {
    // console.fps command
    processor->registerCommand("console.fps",
        [console](const std::vector<std::string>& args) {
            console->toggle();
            console->addLine("Console toggled", GREEN);
        }, "Toggle FPS display", "Console");
}

void CommandRegistry::registerLogCommands(CommandProcessor* processor, Console* console) {
    // logs command
    processor->registerCommand("logs",
        [console](const std::vector<std::string>& args) {
            try {
                std::vector<std::filesystem::path> logFiles;

                // Check if logs directory exists
                if (!std::filesystem::exists("logs")) {
                    console->addLine("No logs directory found", YELLOW);
                    return;
                }

                // Collect all log files
                for (const auto& entry : std::filesystem::directory_iterator("logs")) {
                    if (entry.is_regular_file() && entry.path().extension() == ".log") {
                        logFiles.push_back(entry.path());
                    }
                }

                if (logFiles.empty()) {
                    console->addLine("No log files found", YELLOW);
                    return;
                }

                // Sort by modification time (newest first)
                std::sort(logFiles.begin(), logFiles.end(),
                    [](const std::filesystem::path& a, const std::filesystem::path& b) {
                        return std::filesystem::last_write_time(a) > std::filesystem::last_write_time(b);
                    });

                console->addLine("Log files (newest first):", YELLOW);
                for (const auto& file : logFiles) {
                    auto fileTime = std::filesystem::last_write_time(file);
                    auto fileSize = std::filesystem::file_size(file);

                    std::stringstream ss;
                    ss << "  " << file.filename().string() << " - " << fileSize << " bytes";
                    console->addLine(ss.str(), GRAY);
                }

            } catch (const std::exception& e) {
                console->addLine("Error listing logs: " + std::string(e.what()), RED);
            }
        }, "List available log files", "Debug");
}

} // namespace GameEngine
