@echo off
REM -Od = disable optimisation, -O2 = fast code
set CommonCompilerFlags=-MTd -nologo -GR- -EHa- -Od -Oi -WX -W4 -wd4201 -wd4100 -wd4189 -wd4324 -FC -Z7 -I ..\1.2.162.0\Include
set CommonLinkerFlags= -incremental:no -opt:ref user32.lib Gdi32.lib winmm.lib /LIBPATH:..\1.2.162.0\Lib vulkan-1.lib

IF NOT EXIST ..\build mkdir ..\build
pushd ..\build

REM x64 build
set srcPath=..\source\
cl %CommonCompilerFlags% %srcPath%Mariposa.cpp %srcPath%Win32_Mariposa.cpp %srcPath%VulkanLayer.cpp /link %CommonLinkerFlags%
popd