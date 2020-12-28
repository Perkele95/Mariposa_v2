# Mariposa
 // Mariposa :: v2 //
This is a personal project aimed at creating a voxel engine for game development using Vulkan for hardware acceleration.
*Many thanks to Casey Muratori and his Handmade Hero project, I could not have done it without him.*

## Data-Oriented Design (DOD)
The project follows a Data-Oriented approach, where I aim for cache-locality, restructured struct to avoid extra unecessary size
due to struct padding and few if no unecessary abstractions around what is important: _data_. This involves seprating state and 
logic into different parts of the system, keeping data as POD (Plain Old Data) structs. By going for SoA (Struct of Arrays) rather 
than AoS (Array of Structs) one can separate "hot" and "cold" data. "Hot" data is fetched very often and therefore it is beneficial 
to fill the CPU caches with it. SoA gives "hot" data a better cache-locality as more of it fits into the cache-line when the "cold" 
data is separate.

A popular term in game development these days is ECS (Entity-Component-System). ECS has shown to be very efficient for large amounts 
of similar data components for game entities/objects. It uses the same approach of grouping data into SoA since the components are 
looped through at least once every frame.

## Memory Pool
As most people quickly realise when working with malloc/calloc/realloc is that keeping track of what memory has been freed
or what part of the program will free what is confusing. The Standard Template Library offers safe wrappers for automatic memory
management, but little in the department of pre-allocating memory and reuse of addresses. I created my own memory pool system
which allows me to pre-allocate memory, recycle memory as well as get more precise debug messages relating to memory errors.

### Using the memory system
* **mpCreateMemoryPool** creates and allocates a block of memory and prepares it for further use.
* **mpGetMemoryRegion** fetches a sub-block of the given memory pool.
* **mpAllocateIntoRegion** reserves a sized part of a memory region, and returns a void*.
* **mpResetMemoryRegion** nulls the data in a memory region and allows one to write new data into it.
* **mpFreeMemoryRegion** releases a given region back to the memory pool and allows another part of the program
to fetch it with mpGetMemoryRegion.

## Voxel Mesh Creation
Mariposa constructs individual meshes out of chunk data. Chunk data consists of voxel info for N^3 voxels, where N is the 
width/height/depth of the chunk. A single draw call is issued per mesh. The engine always detects changes in chunk data 
to recreate the given chunks. New draw calls are only issued for chunks that have been updated.

At the moment Mariposa generates meshes dynamically by iterating through chunk data. Only visible triangle faces are 
generated and rendered. In the future I plan on using a permutation table with all 64 unique variations of a voxel.
