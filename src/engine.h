#ifndef __ENGINE_H__
#define __ENGINE_H__

#include <functional>
#include <map>
#include <string>

struct Matrix;
struct Trace;

using EngineFunc = std::function<Trace(const Matrix& problem_matrix)>;

struct RegisterEngine {
    RegisterEngine(std::string name, EngineFunc func);
    static std::map<std::string, EngineFunc>& Engines();
};

#define REGISTER_ENGINE(name, func) \
    RegisterEngine __register_##name(#name, func)

#endif // __ENGINE_H__
// vim: set si et sw=4 ts=4:
