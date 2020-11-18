#pragma once

#include <stdint.h>
#include <stdio.h>
#include <Windows.h>

struct profile_result
{
    char* ID;
    uint64_t CycleCount;
    uint32_t HitCount;
};

static profile_result ProfileResults[30] = {0};
static uint32_t ProfilerCount = 0;

#ifdef PROFILER_ENABLE
static void _ProfilerPushBackResult(char* ID, uint64_t cycleCount)
{
    for(uint32_t i = 0; i < ProfilerCount; i++)
    {
        if(!strcmp(ProfileResults[i].ID, ID))
        {
            ProfileResults[i].CycleCount = cycleCount;
            ProfileResults[i].HitCount++;
            return;
        }
    }
    ProfileResults[ProfilerCount] = {ID, cycleCount, 1};
    ProfilerCount++;
}
#endif

void PrintProfileResults()
{
#ifdef PROFILER_ENABLE
    OutputDebugStringA("Profiler:\n");
    for(uint32_t i = 0; i < ProfilerCount; i++)
    {
        profile_result res = ProfileResults[i];
        char buffer[256];
        _snprintf_s(buffer, sizeof(buffer), "  %s: CycleCount = %zu, HitCount = %d\n", res.ID, res.CycleCount, res.HitCount);
        OutputDebugStringA(buffer);
        
        ProfileResults[i].CycleCount = 0;
        ProfileResults[i].HitCount = 0;
    }
#endif
}

#ifdef PROFILER_ENABLE
    #define PROFILE_SCOPE_START(ID) uint64_t _cycle_counter##ID = __rdtsc();
    #define PROFILE_SCOPE_END(ID) uint64_t _cycle_counter##ID##_result = __rdtsc() - _cycle_counter##ID; _ProfilerPushBackResult(#ID, _cycle_counter##ID##_result);
#else
    #define PROFILE_SCOPE_START(ID)
    #define PROFILE_SCOPE_END(ID)
    #define PROFILE_FUNCTION_START
    #define PROFILE_FUNCTION_END
#endif