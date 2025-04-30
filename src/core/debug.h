#pragma once
#include <cstdio>

#if defined(_WIN32)
#include <windows.h>
#define DebugMessage(fmt, ...)                           \
        do {                                               \
            char buf[512];                                 \
            snprintf(buf, sizeof(buf), fmt, ##__VA_ARGS__); \
            OutputDebugStringA(buf);                      \
            OutputDebugStringA("\n");                     \
            printf("%s\n", buf);                          \
        } while (0)
#elif defined(__EMSCRIPTEN__)
#include <emscripten/emscripten.h>
#define DebugMessage(fmt, ...) printf(fmt "\n", ##__VA_ARGS__)
#else
#define DebugMessage(fmt, ...) printf(fmt "\n", ##__VA_ARGS__)
#endif