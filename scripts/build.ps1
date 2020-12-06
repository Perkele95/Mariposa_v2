$buildFolderExists = Test-Path -path build
if($buildFolderExists -eq $false)
{
    New-Item -Name "build" -ItemType "directory"
}
Set-Location "..\build"

New-Variable -name "sourcePath" -value "..\source\"
New-Variable -name "compilerFlags" -value "-MTd -nologo -GR- -EHa- -Od -Oi -WX -W4 -wd4201 -wd4100 -wd4189 -FC -Z7 -I ..\Vulkan\Include -I ..\stb_image"
New-Variable -name "linkerFlags" -value "-incremental:no -opt:ref user32.lib Gdi32.lib winmm.lib /LIBPATH:..\Vulkan\Lib vulkan-1.lib"


Set-Location "..\scripts"