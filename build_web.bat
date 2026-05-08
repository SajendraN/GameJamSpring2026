@echo off
SETLOCAL

:: --- 1. SETUP EMSCRIPTEN ENVIRONMENT ---
SET EMSDK_PATH=C:/raylib/emsdk

echo Loading Emscripten environment...
IF EXIST "%EMSDK_PATH%/emsdk_env.bat" (
    call "%EMSDK_PATH%/emsdk_env.bat"
) ELSE (
    echo ERROR: emsdk_env.bat not found at %EMSDK_PATH%
    pause
    exit /b
)

::SET CMAKE_MAKE_PROGRAM=C:/raylib/w64devkit/bin/mingw32-make


:: --- 2. BUILD ---
IF NOT EXIST build_web mkdir build_web
cd build_web

echo Configuring and Building...
::echo "%EMSDK%/upstream/emscripten/cmake/Modules/Platform/Emscripten.cmake"
cmake .. -G "MinGW Makefiles" ^
    -DCMAKE_TOOLCHAIN_FILE="%EMSDK%/upstream/emscripten/cmake/Modules/Platform/Emscripten.cmake" ^
    -DCMAKE_MAKE_PROGRAM="C:/raylib/w64devkit/bin/mingw32-make" ^
    -DPLATFORM=Web ^
    -DCMAKE_BUILD_TYPE=Release
cmake --build .

:: --- 3. LAUNCH SERVER AND BROWSER ---

:: Navigate to the new bin folder to start the server
cd bin

echo.
echo Launching local server...
:: 'start' opens a new command prompt for the server
:: We use 'client' folder because CMake puts the html there
start "Game Server" python -m http.server 8000

echo Opening browser...
:: Change 'client.html' to match your executable name if different
start http://localhost:8000/client.html

echo.
echo Web build is live at http://localhost:8000
echo Close the Python window to stop the server.
pause
