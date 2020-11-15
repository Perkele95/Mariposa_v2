# Mariposa
 // Mariposa :: v2 //
This is a personal project aimed at creating a voxel engine for game development using Vulkan for hardware acceleration.
*Many thanks to Casey Muratori and his Handmade Hero project, I could not have done it without him.*

## Data-Oriented Design/Programming
The source code follows a Data-Oriented approach, which means making sure there is good cache-coherency, tightly packed data and
few if no unecessary abstractions around what is important. This also means the programming style is very simple. Despite using C++
the code might look more like C, while not a perfect language, doesn't obstruct Data-Oriented Programming.

### Cache-coherency
More and more people are realising packing data into big clusters allows better use of the caches in modern computers. Data which is often
used is better off being clustered together so that most if not all of it can be cached for fastes access. ECS implementations are becoming
popular as people have realised that the usual Object-Oriented Programming way of bunching state and logic together is less efficient with 
a higher number of entities/objects.

### Compression
Typically I employ a simple strategy to programming:
 1. solve the problem
 2. reduce/compress/optimise
 3. abstract
 I like this style personally, as refactoring code is easier when there are fewer layers of abstraction in the way.

 