#ifndef __ENGINE_H__
#define __ENGINE_H__

#include <functional>
#include <map>
#include <string>

#include "system.h"
#include "trace.h"

using EngineFunc = std::function<Trace(const System& system, const Matrix& problem_matrix)>;

struct RegisterEngine {
    RegisterEngine(std::string name, EngineFunc func) {
        Engines()[name] = func;
    }

    // Singleton
    static std::map<std::string, EngineFunc>& Engines() {
        static std::map<std::string, EngineFunc> engines;
        return engines;
    }
};

#define REGISTER_ENGINE(name, func) \
    RegisterEngine __register_##name(#name, func)

#endif // __ENGINE_H__
// vim: set si et sw=4 ts=4:
