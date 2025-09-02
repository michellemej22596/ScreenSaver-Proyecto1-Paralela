#pragma once
#include <chrono>

// Devuelve el tiempo actual en segundos como double
inline double now_seconds() {
    using namespace std::chrono;
    return duration<double>(high_resolution_clock::now().time_since_epoch()).count();
}
