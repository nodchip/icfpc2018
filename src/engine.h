#ifndef __ENGINE_H__
#define __ENGINE_H__
#include <functional>
#include <map>

#include "nmms.h"

typedef std::function<Trace(const System& system, const Matrix& problem_matrix)> EngineFunc;

struct RegisterEngine {
    RegisterEngine(std::string name, EngineFunc func) {
        engines[name] = func;
    }

    static std::map<std::string, EngineFunc> engines;
};

#define REGISTER_ENGINE(name, func) \
    RegisterEngine __register_##name(#name, func)

#endif // __ENGINE_H__
// vim: set si et sw=4 ts=4:
