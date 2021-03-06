@echo off
echo ---------------------------------------------------------------
echo BUILD
echo ---------------------------------------------------------------

set Includes=-I ..\1.2.162.0\Include -I ..\stb_image

REM -Od = disable optimisation, -O2 = fast code
set CommonCompilerFlags=-MTd -nologo -GR- -EHa- -Od -Oi -WX -W4 -wd4201 -wd4100 -wd4189 -wd4324 -FC -Z7 %Includes%
set CommonLinkerFlags= -incremental:no -opt:ref user32.lib Gdi32.lib winmm.lib /LIBPATH:..\1.2.162.0\Lib vulkan-1.lib

IF NOT EXIST ..\bin mkdir ..\bin
pushd ..\bin

REM x64 build
set srcPath=..\source\
set compilationUnits=%srcPath%Mariposa.cpp %srcPath%Win32_Mariposa.cpp %srcPath%\renderer\renderer.cpp
cl %CommonCompilerFlags% %compilationUnits% /link %CommonLinkerFlags%
popd