# Mariposa
 // Mariposa :: v2 //
This is a personal project aimed at creating a voxel engine for game development using Vulkan for hardware acceleration.
*Many thanks to Casey Muratori and his Handmade Hero project, I could not have done it without him.*

## Data-Oriented Design (DOD)
The project follows a Data-Oriented approach, where I aim for cache-locality, restructured struct to avoid extra unecessary size
due to struct padding and few if no unecessary abstractions around what is important: _data_. I code in C-style C++,
which means I use classes/structs as only POD (Plain Old Data). An important factor is how cachelines only cover so much, and
cache memory is limited. Separating state from logic makes it trivial to create SoA (Struct of Arrays). SoA can keep "hot"
and "cold" data separate, keeping the hot data cached as much as possible. Cold data is not read/written to very often. It is a
waste to hoist an entire class into the cache just to access one member.

More and more people are realising both the benefits of DOD as well as the fact that software is getting slow.
Software is allowed to get slow because hardware keeps getting better. But that is _not_ an excuse.

## Memory Pool
As most people quickly realise when working with malloc/calloc/realloc is that keeping track of what memory has been freed
or what part of the program will free what is confusing. The Standard Template Library offers safe wrappers for automatic memory
management, but little in the department of pre-allocating memory and reuse of addresses. I created my own memory pool system
which allows me to pre-allocate memory, recycle memory as well as get more precise debug messages relating to memory errors.

### Using the memory system
**mpCreateMemoryPool** creates and allocates a block of memory and prepares it for further use.
**mpGetMemoryRegion** fetches a sub-block of the given memory pool.
**mpAllocateIntoRegion** reserves a sized part of a memory region, and returns a void*.
**mpResetMemoryRegion** nulls the data in a memory region and allows one to write new data into it.
**mpFreeMemoryRegion** releases a given region back to the memory pool and allows another part of the program
to fetch it with mpGetMemoryRegion.

## Compression
Typically I employ a simple strategy to programming (thanks Casey):
 1. solve the problem
 2. reduce/compress/optimise
 3. abstract

I try my best to stick to a certain style of coding throughout the codebase. Although constructors are widely used in C++ programming
today, I find them pointless due to my use of POD structs. By inlining initialisation of struct fields/members to named declarations I
find it easier to see what values correspond to what field. Consider the following two styles:

    mpCamera camera = {};
    camera.speed = 10.0f;
    camera.sensitivity = 2.0f;
    camera.fov = PI32 / 3.0f;
    camera.model = Mat4x4Identity();
    camera.position = vec3{12.0f, 12.0f, 12.0f};

    mpCamera camera = {
        10.0f,
        2.0f,
        PI32 / 3.0f,
        Mat4x4Identity(),
        vec3{12.0f, 12.0f, 12.0f}
    };

In order to understand what fields the values in the second style correspond to, one needs to look up the struct definition. The first
style not only solves that, but can also define fields out of order. C has even better solution, but it's not available in C++.