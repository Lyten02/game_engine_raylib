#include "command_registry.h"
#include "../console/console.h"
#include "../console/command_processor.h"
#include "../packages/package_manager.h"
#include "../packages/package.h"
#include <spdlog/spdlog.h>

namespace GameEngine {

void CommandRegistry::registerPackageCommands(CommandProcessor* processor, Console* console, 
                                            PackageManager* packageManager) {
    // package.list command - List all available packages
    processor->registerCommand("package.list",
        [console, packageManager](const std::vector<std::string>& args) {
            packageManager->scanPackages();
            auto packages = packageManager->getAvailablePackages();
            
            if (packages.empty()) {
                console->addLine("No packages found", YELLOW);
                console->addLine("Ensure packages are installed in the packages directory", GRAY);
            } else {
                console->addLine("Available packages:", YELLOW);
                for (const auto& pkg : packages) {
                    // Check if loaded
                    bool isLoaded = packageManager->getPackage(pkg) != nullptr;
                    std::string status = isLoaded ? " [loaded]" : "";
                    console->addLine("  - " + pkg + status, WHITE);
                }
            }
        }, "List all available packages", "Package");
    
    // package.loaded command - List currently loaded packages
    processor->registerCommand("package.loaded",
        [console, packageManager](const std::vector<std::string>& args) {
            auto packages = packageManager->getLoadedPackages();
            
            if (packages.empty()) {
                console->addLine("No packages loaded", YELLOW);
                console->addLine("Use 'package.load <name>' to load a package", GRAY);
            } else {
                console->addLine("Loaded packages:", YELLOW);
                for (const auto& pkg : packages) {
                    auto package = packageManager->getPackage(pkg);
                    if (package) {
                        console->addLine("  - " + pkg + " v" + package->getVersion(), WHITE);
                    }
                }
            }
        }, "List currently loaded packages", "Package");
    
    // package.info command - Show detailed information about a package
    {
        std::vector<CommandParameter> infoParams = {
            {"name", "Name of the package", true, [packageManager]() { 
                packageManager->scanPackages();
                return packageManager->getAvailablePackages(); 
            }}
        };
        processor->registerCommand("package.info",
        [console, packageManager](const std::vector<std::string>& args) {
            if (args.empty()) {
                console->addLine("Usage: package.info <name>", RED);
                return;
            }
            
            auto package = packageManager->getPackage(args[0]);
            if (!package) {
                // Try to load metadata without fully loading the package
                console->addLine("Package not loaded. Scanning for package info...", GRAY);
                packageManager->scanPackages();
                
                // Check if package exists
                auto available = packageManager->getAvailablePackages();
                if (std::find(available.begin(), available.end(), args[0]) == available.end()) {
                    console->addLine("Package not found: " + args[0], RED);
                    return;
                }
                
                console->addLine("Package found but not loaded: " + args[0], YELLOW);
                console->addLine("Use 'package.load " + args[0] + "' to load it", GRAY);
                return;
            }
            
            // Display package information
            console->addLine("Package Information:", YELLOW);
            console->addLine("  Name: " + package->getName(), WHITE);
            console->addLine("  Version: " + package->getVersion(), WHITE);
            console->addLine("  Description: " + package->getDescription(), WHITE);
            console->addLine("  Author: " + package->getAuthor(), WHITE);
            console->addLine("  License: " + package->getLicense(), WHITE);
            console->addLine("  Engine Version: " + package->getEngineVersion(), WHITE);
            
            // Display dependencies
            auto deps = package->getDependencies();
            if (!deps.empty()) {
                console->addLine("  Dependencies:", WHITE);
                for (const auto& dep : deps) {
                    console->addLine("    - " + dep.name + " " + dep.version, GRAY);
                }
            }
            
            // Display components
            auto components = package->getComponents();
            if (!components.empty()) {
                console->addLine("  Components:", WHITE);
                for (const auto& comp : components) {
                    console->addLine("    - " + comp.name, GRAY);
                }
            }
            
            // Display systems
            auto systems = package->getSystems();
            if (!systems.empty()) {
                console->addLine("  Systems:", WHITE);
                for (const auto& sys : systems) {
                    console->addLine("    - " + sys.name + " (priority: " + std::to_string(sys.priority) + ")", GRAY);
                }
            }
        }, "Show detailed information about a package", "Package",
        "package.info <name>", infoParams);
    }
    
    // package.load command - Load a package
    {
        std::vector<CommandParameter> loadParams = {
            {"name", "Name of the package to load", true, [packageManager]() { 
                packageManager->scanPackages();
                return packageManager->getAvailablePackages(); 
            }}
        };
        processor->registerCommand("package.load",
        [console, packageManager](const std::vector<std::string>& args) {
            if (args.empty()) {
                console->addLine("Usage: package.load <name>", RED);
                return;
            }
            
            // Check if already loaded
            if (packageManager->getPackage(args[0])) {
                console->addLine("Package already loaded: " + args[0], YELLOW);
                return;
            }
            
            console->addLine("Loading package: " + args[0] + "...", GRAY);
            
            if (packageManager->loadPackageWithDependencies(args[0])) {
                auto package = packageManager->getPackage(args[0]);
                if (package) {
                    console->addLine("Package loaded successfully: " + args[0] + " v" + package->getVersion(), GREEN);
                    
                    // Show loaded dependencies
                    auto deps = package->getDependencies();
                    if (!deps.empty()) {
                        console->addLine("Loaded dependencies:", GRAY);
                        for (const auto& dep : deps) {
                            console->addLine("  - " + dep.name, GRAY);
                        }
                    }
                }
            } else {
                console->addLine("Failed to load package: " + args[0], RED);
                
                // Check dependencies
                auto resolution = packageManager->checkDependencies(args[0]);
                if (!resolution.missing.empty()) {
                    console->addLine("Missing dependencies:", YELLOW);
                    for (const auto& missing : resolution.missing) {
                        console->addLine("  - " + missing, RED);
                    }
                }
                if (!resolution.incompatible.empty()) {
                    console->addLine("Incompatible dependencies:", YELLOW);
                    for (const auto& incomp : resolution.incompatible) {
                        console->addLine("  - " + incomp, RED);
                    }
                }
            }
        }, "Load a package with its dependencies", "Package",
        "package.load <name>", loadParams);
    }
    
    // package.unload command - Unload a package
    {
        std::vector<CommandParameter> unloadParams = {
            {"name", "Name of the package to unload", true, [packageManager]() { 
                return packageManager->getLoadedPackages(); 
            }}
        };
        processor->registerCommand("package.unload",
        [console, packageManager](const std::vector<std::string>& args) {
            if (args.empty()) {
                console->addLine("Usage: package.unload <name>", RED);
                return;
            }
            
            if (!packageManager->getPackage(args[0])) {
                console->addLine("Package not loaded: " + args[0], YELLOW);
                return;
            }
            
            // Check if other packages depend on this one
            bool hasDependents = false;
            for (const auto& loadedPkg : packageManager->getLoadedPackages()) {
                if (loadedPkg == args[0]) continue;
                
                auto pkg = packageManager->getPackage(loadedPkg);
                if (pkg) {
                    for (const auto& dep : pkg->getDependencies()) {
                        if (dep.name == args[0]) {
                            console->addLine("Cannot unload: " + loadedPkg + " depends on " + args[0], RED);
                            hasDependents = true;
                            break;
                        }
                    }
                }
            }
            
            if (hasDependents) {
                return;
            }
            
            if (packageManager->unloadPackage(args[0])) {
                console->addLine("Package unloaded: " + args[0], GREEN);
            } else {
                console->addLine("Failed to unload package: " + args[0], RED);
                console->addLine("Note: Package unloading is not yet implemented", YELLOW);
            }
        }, "Unload a package", "Package",
        "package.unload <name>", unloadParams);
    }
    
    // package.deps command - Check package dependencies
    {
        std::vector<CommandParameter> depsParams = {
            {"name", "Name of the package", true, [packageManager]() { 
                return packageManager->getLoadedPackages(); 
            }}
        };
        processor->registerCommand("package.deps",
        [console, packageManager](const std::vector<std::string>& args) {
            if (args.empty()) {
                console->addLine("Usage: package.deps <name>", RED);
                return;
            }
            
            auto package = packageManager->getPackage(args[0]);
            if (!package) {
                console->addLine("Package not loaded: " + args[0], RED);
                return;
            }
            
            // Check dependencies
            auto resolution = packageManager->checkDependencies(args[0]);
            auto deps = package->getDependencies();
            
            if (deps.empty()) {
                console->addLine("Package has no dependencies", YELLOW);
                return;
            }
            
            console->addLine("Dependencies for " + args[0] + ":", YELLOW);
            for (const auto& dep : deps) {
                // Check if dependency is satisfied
                auto depPkg = packageManager->getPackage(dep.name);
                if (depPkg) {
                    bool compatible = dep.version.empty() || 
                                    packageManager->isVersionCompatible(dep.version, depPkg->getVersion());
                    
                    if (compatible) {
                        console->addLine("  ✓ " + dep.name + " " + dep.version + 
                                       " (satisfied by v" + depPkg->getVersion() + ")", GREEN);
                    } else {
                        console->addLine("  ✗ " + dep.name + " " + dep.version + 
                                       " (incompatible: v" + depPkg->getVersion() + ")", RED);
                    }
                } else {
                    console->addLine("  ✗ " + dep.name + " " + dep.version + " (not loaded)", RED);
                }
            }
            
            // Show dependency order
            if (resolution.satisfied) {
                auto order = packageManager->getDependencyOrder(args[0]);
                if (order.size() > 1) {
                    console->addLine("Load order:", GRAY);
                    for (size_t i = 0; i < order.size(); ++i) {
                        console->addLine("  " + std::to_string(i + 1) + ". " + order[i], GRAY);
                    }
                }
            }
            
            // Check for circular dependencies
            if (packageManager->hasCircularDependency(args[0])) {
                console->addLine("WARNING: Circular dependency detected!", RED);
            }
        }, "Check package dependencies", "Package",
        "package.deps <name>", depsParams);
    }
    
    // package.refresh command - Rescan packages directory
    processor->registerCommand("package.refresh",
        [console, packageManager](const std::vector<std::string>& args) {
            console->addLine("Scanning packages directory...", GRAY);
            packageManager->scanPackages();
            
            auto packages = packageManager->getAvailablePackages();
            console->addLine("Found " + std::to_string(packages.size()) + " packages", GREEN);
        }, "Rescan the packages directory", "Package");
}

} // namespace GameEngine