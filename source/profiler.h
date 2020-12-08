#pragma once

/*
 * 
 */

#ifdef PROFILER_ENABLE
#include <stdint.h>
#include <stdio.h>

static int32_t globalDbgCounter = 0;

#define dbgGetID__(varName) ID##varName
#define dbgGetID_(varName) dbgGetID__(varName)
#define dbgGetID dbgGetID_(__LINE__)

#define dbgMakeID static int32_t dbgGetID = globalDbgCounter++

#define PROFILE_SCOPE__(num) dbgMakeID; timed_scope timedScope##num(dbgGetID, __FILE__, __LINE__, __FUNCTION__)
#define PROFILE_SCOPE_(num) PROFILE_SCOPE__(num)
#define PROFILE_SCOPE PROFILE_SCOPE_(__LINE__);

struct dbg_record
{
    char *fileName;
    char *functionName;
    uint32_t line;
    uint32_t hitCount;
    uint64_t cycleCount;
    uint64_t oldCycleCount;
};

static dbg_record dbgRecs[30] = {};

struct timed_scope
{
    dbg_record *record;
    uint64_t newCycleCount;
    
    timed_scope(int32_t index, char *fileName, int32_t lineNumber, char *functionName)
    {
        record = &dbgRecs[index];
        record->fileName = fileName;
        record->functionName = functionName;
        record->line = lineNumber;
        newCycleCount = __rdtsc();
    }
    
    ~timed_scope()
    {
        record->cycleCount = __rdtsc() - newCycleCount;
        record->hitCount++;
    }
};

static void mpDbgProcessSampledRecords(uint32_t samplingLevel)
{
    static uint32_t dbgSamplingCount = samplingLevel;
    
    if(dbgSamplingCount < samplingLevel)
    {
        for(int32_t i = 0; i < globalDbgCounter; i++)
        {
            dbgRecs[i].cycleCount = (dbgRecs[i].cycleCount + dbgRecs[i].oldCycleCount) / 2;
            dbgRecs[i].oldCycleCount = dbgRecs[i].cycleCount;
            dbgRecs[i].hitCount = 0;
        }
        dbgSamplingCount++;
    }
    else
    {
        for(int32_t i = 0; i < globalDbgCounter; i++)
        {
            printf("File: %s, Func: %s, Line: %d, CycleCount: %zu, HitCount: %d\n",
                dbgRecs[i].fileName, dbgRecs[i].functionName, dbgRecs[i].line, dbgRecs[i].cycleCount, dbgRecs[i].hitCount);
            dbgRecs[i].oldCycleCount = dbgRecs[i].cycleCount;
            dbgRecs[i].hitCount = 0;
        }
        dbgSamplingCount = 0;
    }
}

#else
#define PROFILE_SCOPE
#endif