#pragma once

#ifdef MP_INTERNAL
#include <Windows.h>
#include <stdint.h>
#include <stdio.h>

#define LOGGER_COLOUR_DEFAULT 7
#define LOGGER_COLOUR_LIGHT_YELLOW 14
#define LOGGER_COLOUR_GREEN 10
#define LOGGER_COLOUR_YELLOW 6
#define LOGGER_COLOUR_RED 4

#define __MP_SET_LOG(colour, cstring) HANDLE logConsole##cstring = GetStdHandle(STD_OUTPUT_HANDLE); SetConsoleTextAttribute(logConsole##cstring, colour);
#define __MP_RESET_LOG(cstring) SetConsoleTextAttribute(logConsole##cstring, LOGGER_COLOUR_DEFAULT);
#define MP_SET_LOG(colour, cstring) __MP_SET_LOG(colour, cstring)
#define MP_RESET_LOG(cstring) __MP_RESET_LOG(cstring)

#define MP_LOG_TRACE(format, ...) MP_SET_LOG(LOGGER_COLOUR_LIGHT_YELLOW, __LINE__) printf(format, __VA_ARGS__); MP_RESET_LOG(__LINE__)
#define MP_LOG_INFO(format, ...) MP_SET_LOG(LOGGER_COLOUR_GREEN, __LINE__) printf(format, __VA_ARGS__); MP_RESET_LOG(__LINE__)
#define MP_LOG_WARN(format, ...) MP_SET_LOG(LOGGER_COLOUR_YELLOW, __LINE__) printf(format, __VA_ARGS__); MP_RESET_LOG(__LINE__)
#define MP_LOG_ERROR(format, ...) MP_SET_LOG(LOGGER_COLOUR_RED, __LINE__) printf(format, __VA_ARGS__); MP_RESET_LOG(__LINE__)
#else
#define MP_LOG_TRACE(format, ...)
#define MP_LOG_INFO(format, ...)
#define MP_LOG_WARN(format, ...)
#define MP_LOG_ERROR(format, ...)
#endif