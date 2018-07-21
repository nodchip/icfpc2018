#pragma once
#include <cstdio>

#define ASSERT_ERROR(expr) \
	do { if (!expr) { std::fprintf(stderr, "[ERROR] " #expr); } } while (false)
#define LOG_ERROR(msg, ...) \
	do { std::fprintf(stderr, "[ERROR]" msg, ##__VA_ARGS__); std::fprintf(stderr, "\n"); } while (false)

// vim: set si et sw=4 ts=4:
