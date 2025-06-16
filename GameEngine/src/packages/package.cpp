#include "package.h"

Package::Package(const std::string& name, const std::string& version)
    : name(name), version(version) {
}

void Package::addDependency(const std::string& depName, const std::string& depVersion) {
    dependencies.push_back({depName, depVersion});
}