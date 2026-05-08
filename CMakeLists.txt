cmake_minimum_required(VERSION 3.14) 
project(BeachStall)

set(CMAKE_CXX_STANDARD 17)

# --- Redirect all binaries to a single folder ---
set(CMAKE_RUNTIME_OUTPUT_DIRECTORY ${CMAKE_BINARY_DIR}/bin)

# For Wasm builds, Emscripten generates multiple files (.html, .js, .wasm)
# This ensures they all end up in the same /bin folder
set(CMAKE_EXECUTABLE_OUTPUT_PATH ${CMAKE_BINARY_DIR}/bin)


include(FetchContent)

set(RAYLIB_VERSION 5.5)
find_package(raylib ${RAYLIB_VERSION} QUIET) # QUIET or REQUIRED
if (NOT raylib_FOUND) # If there's none, fetch and build raylib
  include(FetchContent)
  FetchContent_Declare(
    raylib
    DOWNLOAD_EXTRACT_TIMESTAMP OFF
    URL https://github.com/raysan5/raylib/archive/refs/tags/${RAYLIB_VERSION}.tar.gz
  )
  FetchContent_GetProperties(raylib)
  if (NOT raylib_POPULATED) # Have we downloaded raylib yet?
    set(FETCHCONTENT_QUIET NO)
    FetchContent_MakeAvailable(raylib)
    set(BUILD_EXAMPLES OFF CACHE BOOL "" FORCE) # don't build the supplied examples
  endif()
endif()

if(NOT EMSCRIPTEN)
    FetchContent_Declare(
        ixwebsocket
        URL https://github.com/machinezone/IXWebSocket/archive/refs/heads/master.zip
    )
    set(USE_WS OFF CACHE BOOL "" FORCE)
    set(USE_TLS OFF CACHE BOOL "" FORCE) # Keep it simple/minimal
    FetchContent_MakeAvailable(ixwebsocket)
endif()


# Add subdirectories
add_subdirectory(client)
if(NOT EMSCRIPTEN)
    add_subdirectory(server)
endif()
