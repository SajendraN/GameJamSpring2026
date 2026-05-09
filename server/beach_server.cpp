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
#define RAYGUI_IMPLEMENTATION //only in this cpp file
#include "raygui.h" // in any cpp file that needs it

#include <iostream>
#include <string>
#include <map>
#include <mutex>

#define PORT 8080
//networking crap to rerwite
//research AI disclosure: https://share.google/aimode/JZ7hl00SIDsgVpG89
// libraries to consider: 
// easywsclient for client
// wsServer (server) ? 
// mongoose.ws (client/server)

// --- State ---
std::mutex dataMtx;
Vector2 serverPos = { 400, 225 }; // Server's own circle
std::map<ix::WebSocket*, Vector2> clientPositions;

void HandleClientMessage(std::shared_ptr<ix::WebSocket> socket, const ix::WebSocketMessagePtr& msg) {
    if (msg->type == ix::WebSocketMessageType::Open) {
        //std::cout << "Client connected - " << msg.get()
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


//reference https://machinezone.github.io/IXWebSocket/usage/
void OnClientConnection(std::weak_ptr<ix::WebSocket> webSocket, std::shared_ptr<ix::ConnectionState> state) {
    auto socket = webSocket.lock();
    if (socket) {
        socket->setOnMessageCallback([socket](const ix::WebSocketMessagePtr& msg) {
            HandleClientMessage(socket, msg);
            });
    }
}



//shit this should be shared client and server somehow, fix folder layout later
enum GameMode {
    GAME_MODE_TITLE_SCREEN = 0,
    GAME_MODE_TUTORIAL,
    GAME_MODE_PLAYING,
    GAME_MODE_VIEWING,
    GAME_MODE_JUDGING,
    GAME_MODE_SCORE,
    GAME_MODE_GAME_OVER,
    GAME_MODE_WIN_SCREEN
};

struct TitleSequence {
    //static const int NUM_FRAMES = 72;
    static const int NUM_FRAMES = 1;
    
    Texture2D frames[NUM_FRAMES];

    int currentFrame = 0;

    float timer = 0.0f;
    float frameTime = 1 / 5.0f;

    bool pingPong = true;
    bool reverse = false;
};


//fuck it, deal with file paths later
#define ASSET_DIR "C:/assets"

//must be called after create window: loads textures (needs GPU context)
void InitTitleScreen(TitleSequence &ts, const char* sub_folder, const char* filenamebase);
void UpdateTitleScreen(TitleSequence& seq);
void DrawTitleSequence(const TitleSequence& ts);
void DeInitTitleScreen(TitleSequence& ts);

//state change
void UpdateGameMode(GameMode& gm);

//ui stuff
TitleSequence titleScreen;
bool showTutorialButton = true;
bool tutorialButtonWasPressed = false;

void DrawCorrectGameMode(const GameMode& gm);
void DrawUI();
void DrawTitleScreen();
void InitTutorial();
void UpdateTutorial();
void DrawTutorial();


const Color SAND_GROUND = { 236, 224, 191, 255 };
const Color SAND_PACKED = { 215, 195, 145, 255 };
const Color SAND_WET = { 180, 150, 110, 255 };
const Color SAND_BRIGHT = { 255, 248, 230, 255 };

const Color SAND_COLORS[] = { SAND_GROUND, SAND_PACKED, SAND_WET, SAND_BRIGHT };
const int numSandColors = 4;
int currentSandColor = 1;

const int GRID_SIZE = 50;
const float GRID_MID = GRID_SIZE / 2;
int sandGrid[GRID_SIZE][GRID_SIZE] = { 0 };


const int MAX_HEIGHT = 100;
char sandGridColors[GRID_SIZE][GRID_SIZE][MAX_HEIGHT] = { 0 };

Vector3 LocationAtGridPosition(int i, int j) {
    return Vector3{ -GRID_MID + i, 0, -GRID_MID + j};
}

void GridPostionAtLocation(Vector3 location, int& i, int &j) {
    i = (int)location.x + GRID_MID;
    j = (int)location.z + GRID_MID;;
}




// Setup player 1 camera and screen
int cameraMode = CAMERA_FIRST_PERSON;
Camera cameraTutorial;


int main() {
    std::fill(&sandGrid[0][0], &sandGrid[0][0] + sizeof(sandGrid) / sizeof(int), 1);

    ix::initNetSystem();
    ix::WebSocketServer server(PORT, "0.0.0.0");
    server.setOnConnectionCallback(OnClientConnection);

    auto res = server.listen();
    if (!res.first) {
        TraceLog(LOG_ERROR, "Server Listen Failed: %s", res.second.c_str());
        return 1;
    }
    server.start();
    double lastBroadcast = 0;

    InitWindow(1280, 720, "Server - Keys to Move");
    SetTargetFPS(60);
    GameMode currentGameMode = GAME_MODE_TITLE_SCREEN;
    
    //draw and load first texture to buy time during loading
    titleScreen.frames[0] = LoadTexture(ASSET_DIR"/title_screen/wcscbg_000.png");
    BeginDrawing(); DrawTexture(titleScreen.frames[0], 0, 0, WHITE); EndDrawing();
    
    //worry about slow loading later
    InitTitleScreen(titleScreen, "title_screen", "wcscbg"); //worry about de-init and unload images later

    InitAudioDevice();
    Music theTrack = LoadMusicStream(ASSET_DIR"/music/complete_soundtrack.mp3");
    SetMusicVolume(theTrack, 1.0);
    PlayMusicStream(theTrack);

    
    

    while( !WindowShouldClose() ) {
        UpdateMusicStream(theTrack);

        //CHECK INPUT
        
        // Move Server Circle (Keyboard)
        float speed = 200.0f * GetFrameTime();
        if (IsKeyDown(KEY_RIGHT)) serverPos.x += speed;
        if (IsKeyDown(KEY_LEFT))  serverPos.x -= speed;
        if (IsKeyDown(KEY_UP))    serverPos.y -= speed;
        if (IsKeyDown(KEY_DOWN))  serverPos.y += speed;


        //UPDATE
        // 
        UpdateGameMode(currentGameMode);
        
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

        //DRAW
        BeginDrawing(); {
            ClearBackground(BLACK);
            DrawCorrectGameMode(currentGameMode);


            DrawCircleV(serverPos, 25, RED); // Server is Red
            DrawText("SERVER (YOU)", serverPos.x - 30, serverPos.y + 30, 10, RED);

            dataMtx.lock();
            for (auto const& [socket, pos] : clientPositions) {
                DrawCircleV(pos, 20, BLUE); // Clients are Blue
            }
            dataMtx.unlock();
        } EndDrawing();
    }

    server.stop();
    ix::uninitNetSystem();

    UnloadMusicStream(theTrack);
    CloseAudioDevice();

    return 0;
}

void InitTitleScreen(TitleSequence &ts, const char* sub_folder, const char* filenamebase) {
    for (int i = 0; i < ts.NUM_FRAMES; i++) {
        const char* path = TextFormat("%s/%s/%s_%03d.png", ASSET_DIR, sub_folder, filenamebase);
        
        Texture2D texture = LoadTexture(path);
        //error checking ignored...

        ts.frames[i] = texture;
    }
}

void UpdateTitleScreen(TitleSequence& seq) {
    seq.timer += GetFrameTime();
    
    if (seq.timer >= seq.frameTime) {
        seq.timer -= seq.frameTime;

        if (!seq.reverse) { //going forward
            seq.currentFrame++;

            if (seq.currentFrame >= seq.NUM_FRAMES) {
                if (seq.pingPong) {
                    seq.currentFrame = seq.NUM_FRAMES - 1;
                    seq.reverse = true;
                } else {
                    seq.currentFrame = 0;
                }
            }
        
        } else { //else in reverse
            seq.currentFrame--;

            if (seq.currentFrame < 0) {
                seq.currentFrame = 0;
                seq.reverse = false;
            }
        } //end else
    } //end timer wait
}//end update title screen

void DrawTitleSequence(const TitleSequence& ts) {
    DrawTexture(ts.frames[ts.currentFrame], 0, 0, WHITE);
    DrawText("Wierd Collaborative Sand Castle Building Game", 125, 500, 45, SAND_GROUND);
}

void DeInitTitleScreen(TitleSequence& ts) {
    for (Texture2D& texture : ts.frames)
        UnloadTexture(texture);
}

void UpdateGameMode(GameMode& gm) {
    switch (gm) {
    case GAME_MODE_TITLE_SCREEN:
        UpdateTitleScreen(titleScreen);
        if (tutorialButtonWasPressed) {
            InitTutorial();
            gm = GAME_MODE_TUTORIAL;
        }
        break;
    case GAME_MODE_TUTORIAL:
        UpdateTutorial();
        break;
    }
}

void DrawCorrectGameMode(const GameMode& gm) {
    switch (gm) {
    case GAME_MODE_TITLE_SCREEN:
        DrawTitleScreen();
        break;
    case GAME_MODE_TUTORIAL:
        DrawTutorial();
        break;
        

    default:
        puts("ERROR UNEXPECTED DRAW MODE ERROR");
    }
}

void DrawTitleScreen() {
    DrawTitleSequence(titleScreen);
    DrawUI();
}

void DrawUI() {
    if (showTutorialButton && GuiButton(Rectangle{ 1100, 100, 120, 24 }, "TUTORIAL")) {
        showTutorialButton = false;
        tutorialButtonWasPressed = true;
    }
}

void InitTutorial() {
    cameraTutorial = { 0 };
    cameraTutorial.fovy = 45.0f;
    cameraTutorial.up.y = 1.0f;
    cameraTutorial.target.y = 1.0f;
    cameraTutorial.position.z = -3.0f;
    cameraTutorial.position.y = 1.0f;

    DisableCursor();
}

void UpdateTutorial() {
    // Update camera computes movement internally depending on the camera mode
    // Some default standard keyboard/mouse inputs are hardcoded to simplify use
    // For advanced camera controls, it's recommended to compute camera movement manually
    UpdateCamera(&cameraTutorial, cameraMode);
    if (IsMouseButtonPressed(MOUSE_BUTTON_LEFT)) {
        int i, j;
        GridPostionAtLocation(cameraTutorial.target, i, j);
        if (i >= GRID_SIZE || j >= GRID_SIZE)
            return;
        sandGrid[i][j]++;
        sandGridColors[i][j][sandGrid[i][j]] = currentSandColor++;
        currentSandColor %= numSandColors;
    }
}

void DrawTutorial() {
    ClearBackground(SKYBLUE);
    //DrawText("This is a Tutorial...!", 20, 20, 30, WHITE);
    BeginMode3D(cameraTutorial); {
        // Draw scene: line of cube trees on a plane to make a "world"
        DrawPlane( Vector3{0, 0, 0}, Vector2{50, 50}, SAND_GROUND); // Simple world plane

        //reference trees
        int count = 5;
        float spacing = 4;
        float backWall = count * spacing;
        for (float x = -count * spacing; x <= count * spacing; x += spacing) {
            DrawCube(Vector3{ x, 1.5f, backWall }, 1, 1, 1, LIME);
            DrawCube(Vector3{ x, 0.5f, backWall }, 0.25f, 1, 0.25f, BROWN);

            DrawCube(Vector3{ x + spacing/2, 1.5f, backWall + spacing / 2 }, 1, 1, 1, LIME);
            DrawCube(Vector3{ x + spacing / 2, 0.5f, backWall + spacing / 2 }, 0.25f, 1, 0.25f, BROWN);
        }

        //player cube
        DrawCube(cameraTutorial.position, 1, 1, 1, RED);

        for(int i = 0; i < GRID_SIZE; i++) {
            for(int j = 0; j < GRID_SIZE; j++) {
                int height = sandGrid[i][j];
                for(int h = 0; h < height; h++) {
                    Vector3 location = LocationAtGridPosition(i, j);
                    location.y = h;
                    Color thisColor = SAND_COLORS[sandGridColors[i][j][h]];
                    DrawCube(location, 1, 1, 1, thisColor);
                }
            }
        }



    }EndMode3D();
}

