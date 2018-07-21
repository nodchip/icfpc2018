#include "engine.h"

#include <functional>
#include <map>
#include <string>

RegisterEngine::RegisterEngine(std::string name, EngineFunc func) {
    Engines()[name] = func;
}

// singleton
std::map<std::string, EngineFunc>& RegisterEngine::Engines() {
    static std::map<std::string, EngineFunc> engines;
    return engines;
}
