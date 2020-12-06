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

inline void logger_set_print_colour(WORD logColour)
{
    HANDLE hConsole = GetStdHandle(STD_OUTPUT_HANDLE);
    SetConsoleTextAttribute(hConsole, logColour);
}

inline void logger_reset_print_colour()
{
    HANDLE hConsole = GetStdHandle(STD_OUTPUT_HANDLE);
    SetConsoleTextAttribute(hConsole, LOGGER_COLOUR_DEFAULT);
}

#define MP_LOG_TRACE logger_set_print_colour(LOGGER_COLOUR_LIGHT_YELLOW);
#define MP_LOG_INFO logger_set_print_colour(LOGGER_COLOUR_GREEN);
#define MP_LOG_WARN logger_set_print_colour(LOGGER_COLOUR_YELLOW);
#define MP_LOG_ERROR logger_set_print_colour(LOGGER_COLOUR_RED);
#define MP_LOG_RESET logger_reset_print_colour();
#else
#define MP_LOG_TRACE
#define MP_LOG_INFO
#define MP_LOG_WARN
#define MP_LOG_ERROR
#define MP_LOG_RESET
#endif