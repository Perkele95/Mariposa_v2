@echo off

pushd ..\assets\Shaders
set compilerpath=..\..\1.2.162.0\Bin32

%compilerpath%\glslc.exe shader.vert -o vert.spv
%compilerpath%\glslc.exe shader.frag -o frag.spv

popd