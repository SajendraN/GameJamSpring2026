@echo off
SETLOCAL

SET EMSDK_PATH=C:/raylib/emsdk

echo Loading Emscripten environment...
IF EXIST "%EMSDK_PATH%/emsdk_env.bat" (
    call "%EMSDK_PATH%/emsdk_env.bat"
) ELSE (
    echo ERROR: emsdk_env.bat not found at %EMSDK_PATH%
    pause
    exit /b
)



IF NOT EXIST web_build mkdir web_build
cd web_build

echo Configuring and Building...
cmake .. -G "MinGW Makefiles" ^
    -DCMAKE_TOOLCHAIN_FILE="%EMSDK%/upstream/emscripten/cmake/Modules/Platform/Emscripten.cmake" ^
    -DCMAKE_MAKE_PROGRAM="C:/raylib/w64devkit/bin/mingw32-make" ^
    -DPLATFORM=Web ^
    -DCMAKE_BUILD_TYPE=Release
cmake --build .

cd bin

echo.
echo Launching local server...
start "Game Server" python -m http.server 80

echo Opening browser...
start http://localhost:80/beach_client.html

echo.
echo Web build is live at http://localhost:80
echo Close the Python window to stop the server.
pause
