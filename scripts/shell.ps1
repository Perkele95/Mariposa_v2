& "C:\Program Files (x86)\Microsoft Visual Studio\2019\Community\VC\Auxiliary\Build\vcvarsall.bat" x64

Set-Location "C:\Users\Arle\Desktop\Mariposa_v2\scripts"
$buildFolderExists = Test-Path -path build
if($buildFolderExists -eq $false)
{
    New-Item -Name "build" -ItemType "directory"
}
Set-Location "..\build"
& "C:\Program Files (x86)\Microsoft Visual Studio\2019\Community\Common7\IDE\devenv.exe" Mariposa.exe
Set-Location "..\scripts"