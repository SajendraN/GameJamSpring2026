add_executable(beach_client beach_client.cpp)

target_link_libraries(beach_client PRIVATE raylib)

if(EMSCRIPTEN)
	set(PLATFORM "Web" CACHE STRING "Raylib platform")

    # Wasm Specific Linker Flags
    target_link_options(beach_client PRIVATE 
        "-lwebsocket.js" 
        "-sUSE_GLFW=3"
        "-sASYNCIFY" # Optional: helps with long-running loops
    )
    set_target_properties(beach_client PROPERTIES SUFFIX ".html")
else()
    # Native client (for testing) needs the network headers
    target_link_libraries(beach_client PRIVATE ixwebsocket)
endif()
