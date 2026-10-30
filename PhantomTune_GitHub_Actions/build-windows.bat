@echo off
setlocal
cmake -S . -B build -G "Visual Studio 17 2022" -A x64
if errorlevel 1 exit /b 1
cmake --build build --config Release --target PhantomTune_VST3
if errorlevel 1 exit /b 1
echo.
echo Build complete. Find Phantom Tune.vst3 under build\PhantomTune_artefacts\Release\VST3\
pause
