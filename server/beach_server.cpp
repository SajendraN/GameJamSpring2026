// move the windows functions to new names
// note that you can't call these functions or structures from your code, but you should not neeed to
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
#include "ixwebsocket/IXWebSocketServer.h"
#include "ixwebsocket/IXWebSocket.h"
#include "ixwebsocket/IXConnectionState.h" // Might be needed for the type

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

#include "raylib.h"

#include <iostream>
#include <string>
#include <map>
#include <mutex>

// --- State ---
Vector2 serverPos = { 400, 225 }; // Server's own circle
std::map<ix::WebSocket*, Vector2> clientPositions;
std::mutex dataMtx;

void HandleClientMessage(std::shared_ptr<ix::WebSocket> socket, const ix::WebSocketMessagePtr& msg) {
    if (msg->type == ix::WebSocketMessageType::Open) {
        std::lock_guard<std::mutex> lock(dataMtx);
        clientPositions[socket.get()] = { -100, -100 }; // Start off-screen
    }
    else if (msg->type == ix::WebSocketMessageType::Close) {
        std::lock_guard<std::mutex> lock(dataMtx);
        clientPositions.erase(socket.get());
    }
    else if (msg->type == ix::WebSocketMessageType::Message) {
        float x, y;
        if (sscanf(msg->str.c_str(), "%f,%f", &x, &y) == 2) {
            std::lock_guard<std::mutex> lock(dataMtx);
            clientPositions[socket.get()] = { x, y };
        }
    }
}

void OnClientConnection(std::weak_ptr<ix::WebSocket> webSocket, std::shared_ptr<ix::ConnectionState> state) {
    auto socket = webSocket.lock();
    if (socket) {
        socket->setOnMessageCallback([socket](const ix::WebSocketMessagePtr& msg) {
            HandleClientMessage(socket, msg);
            });
    }
}


int main() {
    ix::initNetSystem();
    ix::WebSocketServer server(9002, "0.0.0.0");
    server.setOnConnectionCallback(OnClientConnection);

    auto res = server.listen();
    if (!res.first) {
        TraceLog(LOG_ERROR, "Server Listen Failed: %s", res.second.c_str());
        return 1;
    }
    server.start();

    InitWindow(800, 450, "Server - Keys to Move");
    SetTargetFPS(60);

    double lastBroadcast = 0;

    while (!WindowShouldClose()) {
        // 1. Move Server Circle (Keyboard)
        float speed = 200.0f * GetFrameTime();
        if (IsKeyDown(KEY_RIGHT)) serverPos.x += speed;
        if (IsKeyDown(KEY_LEFT))  serverPos.x -= speed;
        if (IsKeyDown(KEY_UP))    serverPos.y -= speed;
        if (IsKeyDown(KEY_DOWN))  serverPos.y += speed;

        // 2. Broadcast Tick (20Hz)
        double now = GetTime();
        if (now - lastBroadcast >= 0.05) {
            // Build one big string of all positions: "ServerX,ServerY|Client1X,Client1Y|..."
            std::string packet = std::to_string(serverPos.x) + "," + std::to_string(serverPos.y);

            dataMtx.lock();
            for (auto const& [socket, pos] : clientPositions) {
                packet += "|" + std::to_string(pos.x) + "," + std::to_string(pos.y);
            }
            dataMtx.unlock();

            for (auto&& client : server.getClients()) {
                client->sendText(packet);
            }
            lastBroadcast = now;
        }

        // 3. Draw Local View
        BeginDrawing();
        ClearBackground(BLACK);
        DrawCircleV(serverPos, 25, RED); // Server is Red
        DrawText("SERVER (YOU)", serverPos.x - 30, serverPos.y + 30, 10, RED);

        dataMtx.lock();
        for (auto const& [socket, pos] : clientPositions) {
            DrawCircleV(pos, 20, BLUE); // Clients are Blue
        }
        dataMtx.unlock();
        EndDrawing();
    }

    server.stop();
    ix::uninitNetSystem();
    return 0;
}