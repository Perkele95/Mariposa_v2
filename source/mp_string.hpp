#pragma once

#include <stdint.h>
#include <stdlib.h>

struct mpString
{
    mpString() : buffer(nullptr), size(0){
        // Default constructor
    }

    mpString(const char *c_str){
        size = strlen(c_str) + 1;   // +1 for null-terminator
        buffer = static_cast<char*>(malloc(size));
        memset(buffer, 0, size);
        memcpy(buffer, c_str, size);
    }

    const char *c_str(){
        return buffer;
    }

private:
    char *buffer;
    size_t size;
};