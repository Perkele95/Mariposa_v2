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

#define MP_SET_LOG(colour) HANDLE logConsole = GetStdHandle(STD_OUTPUT_HANDLE); SetConsoleTextAttribute(logConsole, colour);

#define MP_LOG_TRACE(format, ...) MP_SET_LOG(LOGGER_COLOUR_LIGHT_YELLOW) printf(format, __VA_ARGS__); SetConsoleTextAttribute(logConsole, LOGGER_COLOUR_DEFAULT);
#define MP_LOG_INFO(format, ...) MP_SET_LOG(LOGGER_COLOUR_GREEN) printf(format, __VA_ARGS__); SetConsoleTextAttribute(logConsole, LOGGER_COLOUR_DEFAULT);
#define MP_LOG_WARN(format, ...) MP_SET_LOG(LOGGER_COLOUR_YELLOW) printf(format, __VA_ARGS__); SetConsoleTextAttribute(logConsole, LOGGER_COLOUR_DEFAULT);
#define MP_LOG_ERROR(format, ...) MP_SET_LOG(LOGGER_COLOUR_RED) printf(format, __VA_ARGS__); SetConsoleTextAttribute(logConsole, LOGGER_COLOUR_DEFAULT);
#else
#define MP_LOG_TRACE(format, ...)
#define MP_LOG_INFO(format, ...)
#define MP_LOG_WARN(format, ...)
#define MP_LOG_ERROR(format, ...)
#endif