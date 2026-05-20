$ErrorActionPreference = "Stop"

function Get-CommandPath($Name) {
    $command = Get-Command $Name -ErrorAction SilentlyContinue
    if ($command) {
        return $command.Source
    }

    return $null
}

function Find-VsDevCmd {
    $vswhere = Join-Path ${env:ProgramFiles(x86)} "Microsoft Visual Studio\Installer\vswhere.exe"
    if (-not (Test-Path $vswhere)) {
        return $null
    }

    $installPath = & $vswhere -latest -products * -requires Microsoft.VisualStudio.Component.VC.Tools.x86.x64 -property installationPath
    if (-not $installPath) {
        return $null
    }

    $candidate = Join-Path $installPath "Common7\Tools\VsDevCmd.bat"
    if (Test-Path $candidate) {
        return $candidate
    }

    return $null
}

$root = Split-Path -Parent $MyInvocation.MyCommand.Path
Set-Location $root

$localTemp = Join-Path $root "build\tmp"
New-Item -ItemType Directory -Force -Path $localTemp | Out-Null
$env:TEMP = $localTemp
$env:TMP = $localTemp

$localCompiler = Get-ChildItem -Path "tools" -Recurse -Filter "x86_64-w64-mingw32-g++.exe" -ErrorAction SilentlyContinue |
    Sort-Object {
        if ($_.FullName -like "*msvcrt*") { 0 } else { 1 }
    }, FullName |
    Select-Object -First 1 -ExpandProperty FullName
if ($localCompiler) {
    New-Item -ItemType Directory -Force -Path "dist" | Out-Null
    & $localCompiler -std=c++17 -O2 -municode -mwindows -static -static-libgcc -static-libstdc++ -Wall -Wextra -Wpedantic "src/main.cpp" -o "dist/crosshair.exe" -luser32 -lgdi32 -lshell32 -lcomctl32 -lwininet
    if ($LASTEXITCODE -ne 0) {
        exit $LASTEXITCODE
    }

    $localX86Compiler = Get-ChildItem -Path (Split-Path -Parent $localCompiler) -Filter "i686-w64-mingw32-g++.exe" -ErrorAction SilentlyContinue |
        Select-Object -First 1 -ExpandProperty FullName
    if ($localX86Compiler) {
        & $localX86Compiler -std=c++17 -O2 -municode -mwindows -static -static-libgcc -static-libstdc++ -Wall -Wextra -Wpedantic "src/main.cpp" -o "dist/crosshair-x86.exe" -luser32 -lgdi32 -lshell32 -lcomctl32 -lwininet
        exit $LASTEXITCODE
    }

    exit 0
}

$cmake = Get-CommandPath "cmake"
if ($cmake) {
    & $cmake -S . -B build
    & $cmake --build build --config Release
    exit $LASTEXITCODE
}

$cl = Get-CommandPath "cl"
if ($cl) {
    New-Item -ItemType Directory -Force -Path "build\msvc" | Out-Null
    & $cl /nologo /std:c++17 /EHsc /W4 /DUNICODE /D_UNICODE /DWIN32 /D_WINDOWS "src\main.cpp" /link /SUBSYSTEM:WINDOWS "user32.lib" "gdi32.lib" "shell32.lib" "comctl32.lib" "wininet.lib" /OUT:"build\msvc\crosshair.exe"
    exit $LASTEXITCODE
}

$vsDevCmd = Find-VsDevCmd
if ($vsDevCmd) {
    New-Item -ItemType Directory -Force -Path "build\msvc" | Out-Null
    $command = "`"$vsDevCmd`" -arch=x64 && cl /nologo /std:c++17 /EHsc /W4 /DUNICODE /D_UNICODE /DWIN32 /D_WINDOWS `"src\main.cpp`" /link /SUBSYSTEM:WINDOWS `"user32.lib`" `"gdi32.lib`" `"shell32.lib`" `"comctl32.lib`" `"wininet.lib`" /OUT:`"build\msvc\crosshair.exe`""
    cmd.exe /s /c $command
    exit $LASTEXITCODE
}

$gpp = Get-CommandPath "g++"
if ($gpp) {
    New-Item -ItemType Directory -Force -Path "build\mingw" | Out-Null
    & $gpp -std=c++17 -municode -mwindows -Wall -Wextra -Wpedantic "src/main.cpp" -o "build/mingw/crosshair.exe" -luser32 -lgdi32 -lshell32 -lcomctl32 -lwininet
    exit $LASTEXITCODE
}

Write-Host "No C++ build tool was found."
Write-Host "Install Visual Studio Build Tools with the 'Desktop development with C++' workload, or install CMake plus a C++ compiler, then run:"
Write-Host "  .\build.ps1"
exit 1
