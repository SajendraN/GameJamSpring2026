
#ifdef __EMSCRIPTEN__
    #include <emscripten/websocket.h>
    EMSCRIPTEN_WEBSOCKET_T wasmSocket;
#else
    #define CloseWindow CloseWindowWin32
    #define Rectangle RectangleWin32
    #define ShowCursor ShowCursorWin32
    #define LoadImageA LoadImageAWin32
    #define LoadImageW LoadImageWin32
    #define DrawTextA DrawTextAWin32
    #define DrawTextW DrawTextWin32
    #define DrawTextExA DrawTextExAWin32
    #define DrawTextExW DrawTextExWin32
    #define PlaySoundA PlaySoundAWin32

    // include windows
    #define WIN32_LEAN_AND_MEAN 

    #include <ixwebsocket/IXNetSystem.h>
    #include "ixwebsocket/IXWebSocket.h"
    ix::WebSocket nativeSocket;

    // remove all our redfintions so that raylib can define them properly
    #undef CloseWindow
    #undef Rectangle
    #undef ShowCursor
    #undef LoadImage 
    #undef LoadImageA
    #undef LoadImageW
    #undef DrawText 
    #undef DrawTextA
    #undef DrawTextW
    #undef DrawTextEx 
    #undef DrawTextExA
    #undef DrawTextExW
    #undef PlaySoundA

#endif

#include "raylib.h"

#include <string>
#include <vector>
#include <deque>
#include <mutex>
#include <sstream>

// --- Data Structures ---
struct PlayerCircle {
    Vector2 pos;
    Color color;
};

// --- SHARED DATA ---
struct {
    std::deque<std::string> queue;
    std::mutex mtx;
} inbox;

std::vector<PlayerCircle> worldState;

// --- NETWORK CALLBACKS ---
#ifdef __EMSCRIPTEN__
EM_BOOL OnWasmMessage(int eventType, const EmscriptenWebSocketMessageEvent* e, void* userData) {
    if (e->data) {
        std::lock_guard<std::mutex> lock(inbox.mtx);
        inbox.queue.push_back(std::string((char*)e->data, e->numBytes));
    }
    return EM_TRUE;
}
#else
void OnNativeMessage(const ix::WebSocketMessagePtr& msg) {
    if (msg->type == ix::WebSocketMessageType::Message) {
        std::lock_guard<std::mutex> lock(inbox.mtx);
        inbox.queue.push_back(msg->str);
    }
}
#endif

// --- CLEAN INTERFACE FUNCTIONS ---
void NetSend(const std::string& msg) {
#ifdef __EMSCRIPTEN__
    emscripten_websocket_send_utf8_text(wasmSocket, msg.c_str());
#else
    if (nativeSocket.getReadyState() == ix::ReadyState::Open) {
        nativeSocket.sendText(msg);
    }
#endif
}

void SendPos(float x, float y) {
    std::string msg = std::to_string(x) + "," + std::to_string(y);
    NetSend(msg); // Logic passes through the universal sender
}

bool NetConnected() {
#ifdef __EMSCRIPTEN__
    return true; // Simple assumption for Wasm
#else
    return nativeSocket.getReadyState() == ix::ReadyState::Open;
#endif
}

// --- LOGIC ---
void UpdateWorldState(std::string data) {
    worldState.clear();
    std::stringstream ss(data);
    std::string segment;
    bool isServer = true;

    while (std::getline(ss, segment, '|')) {
        float x, y;
        if (sscanf(segment.c_str(), "%f,%f", &x, &y) == 2) {
            worldState.push_back({ {x, y}, isServer ? RED : BLUE });
            isServer = false;
        }
    }
}

//#define URL "wss://tribune-dragonfly-roundworm.ngrok-free.dev"
#define WS_URL "ws://192.168.140.92:8080"
//#define WS_URL "ws://192.168.140.175:8080"
//#define WS_URL "ws://192.168.86.38:8080"

const Color SAND_GROUND = { 236, 224, 191, 255 };


int main() {
    // 1. Networking Init
#ifdef __EMSCRIPTEN__
    EmscriptenWebSocketCreateAttributes attr = { WS_URL, NULL, EM_TRUE };
    wasmSocket = emscripten_websocket_new(&attr);
    emscripten_websocket_set_onmessage_callback(wasmSocket, NULL, OnWasmMessage);
#else
    ix::initNetSystem();
    nativeSocket.setUrl(WS_URL);
    nativeSocket.setOnMessageCallback(OnNativeMessage);
    nativeSocket.start();
#endif

    
    InitWindow(500, 500, "Multiplayer Client");
    SetTargetFPS(60);

    while (!WindowShouldClose()) {
        // --- POLL NETWORK ---
        inbox.mtx.lock();
        while( !inbox.queue.empty() ) {
            UpdateWorldState(inbox.queue.front());
            inbox.queue.pop_front();
        }
        inbox.mtx.unlock();

        // --- INPUT: Send mouse position if moved ---
        static Vector2 lastSent = { 0, 0 };
        bool mouseDown = IsMouseButtonDown(MOUSE_BUTTON_LEFT);
        if (mouseDown) {
            Vector2 m = GetMousePosition();
            if (m.x != lastSent.x || m.y != lastSent.y) {
                SendPos(m.x, m.y);
                lastSent = m;
            }
        }
        // --- DRAW ---
        BeginDrawing();
            ClearBackground(SAND_GROUND);
            //reference water
            DrawRectangle(0, 475, 500, 25, BLUE);
            
            //reference trees
            for (int i = 0; i < 10; i++) {
                DrawRectangle(i * 50 + 25, 20, 10, 30, BROWN);
                DrawPoly(Vector2{ i * 50.0f + 30, 15 },13, 15, -90, GREEN);
            }

            if (mouseDown) {
                DrawCircleV(lastSent, 5, BLUE);
            }


            for (const auto& player : worldState) {
                //DrawCircleV(player.pos, 20, player.color);
            }
            if( !NetConnected() ) 
                DrawText("OFFLINE", 10, 10, 20, RED);
            else 
                DrawText("ONLINE", 10, 10, 20, GREEN);
        EndDrawing();
    }

    // 3. Cleanup
#ifndef __EMSCRIPTEN__
    nativeSocket.stop();
    ix::uninitNetSystem();
#endif
    CloseWindow();
    return 0;
}