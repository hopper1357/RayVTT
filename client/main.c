#include "raylib.h"
#include <emscripten/emscripten.h>
#include <stdio.h>
#include <string.h>
#include <stdlib.h>

#include <time.h>

#include "network.h"

#define MAX_TOKENS 5
#define MAX_DICE_MESSAGES 5
#define MAX_ROOM_LOG_MESSAGES 10

// Define a simple Token structure
typedef struct Token {
    int id;
    float x;
    float y;
    float width;
    float height;
} Token;

Token tokens[MAX_TOKENS];
int numTokens = 0;

Token* draggedToken = NULL; // Pointer to the token currently being dragged

char diceRollMessages[MAX_DICE_MESSAGES][64];
volatile int numDiceRollMessages = 0;

char my_client_id[64]; // Store our own client ID
// char current_room_id[64]; // Store the current room ID - Defined in network.c

char roomLogMessages[MAX_ROOM_LOG_MESSAGES][128];
volatile int numRoomLogMessages = 0;

bool isNetworkReady = false; // Flag to indicate if network is ready to send messages

// Function to add a message to the room log
void addRoomLogMessage(const char* message) {
    if (numRoomLogMessages < MAX_ROOM_LOG_MESSAGES) {
        strncpy(roomLogMessages[numRoomLogMessages], message, 127);
        roomLogMessages[numRoomLogMessages][127] = '\0';
        numRoomLogMessages++;
    } else {
        // Shift messages up
        for (int i = 0; i < MAX_ROOM_LOG_MESSAGES - 1; i++) {
            strncpy(roomLogMessages[i], roomLogMessages[i+1], 127);
        }
        strncpy(roomLogMessages[MAX_ROOM_LOG_MESSAGES - 1], message, 127);
        roomLogMessages[MAX_ROOM_LOG_MESSAGES - 1][127] = '\0';
    }
}


// Callback functions for network events
void network_on_update_token(const char* sender_id, int id, float x, float y) {
    char sender_id_str[64];
    if (sender_id) {
        strncpy(sender_id_str, sender_id, sizeof(sender_id_str) - 1);
        sender_id_str[sizeof(sender_id_str) - 1] = '\0';
    } else {
        strncpy(sender_id_str, "(null)", sizeof(sender_id_str) - 1);
        sender_id_str[sizeof(sender_id_str) - 1] = '\0';
    }

    bool found = false;
    for (int i = 0; i < numTokens; i++) {
        if (tokens[i].id == id) {
            tokens[i].x = x;
            tokens[i].y = y;
            TraceLog(LOG_INFO, "Updating token %d to (%.2f, %.2f) from sender %s", id, x, y, sender_id_str);
            found = true;
            break;
        }
    }
    if (!found) {
        TraceLog(LOG_WARNING, "Token with ID %d not found for update from sender %s.", id, sender_id_str);
    }
}

void network_on_add_dice_roll_message(const char* sender_id, const char* message) {
    char sender_id_str[64];
    if (sender_id) {
        strncpy(sender_id_str, sender_id, sizeof(sender_id_str) - 1);
        sender_id_str[sizeof(sender_id_str) - 1] = '\0';
    } else {
        strncpy(sender_id_str, "(null)", sizeof(sender_id_str) - 1);
        sender_id_str[sizeof(sender_id_str) - 1] = '\0';
    }
    TraceLog(LOG_INFO, "Received dice roll message from %s: %s", sender_id_str, message);
    if (numDiceRollMessages < MAX_DICE_MESSAGES) {
        strncpy(diceRollMessages[numDiceRollMessages], message, 63);
        diceRollMessages[numDiceRollMessages][63] = '\0';
        numDiceRollMessages++;
    } else {
        // Shift messages up
        for (int i = 0; i < MAX_DICE_MESSAGES - 1; i++) {
            strncpy(diceRollMessages[i], diceRollMessages[i+1], 63);
        }
        strncpy(diceRollMessages[MAX_DICE_MESSAGES - 1], message, 63);
        diceRollMessages[MAX_DICE_MESSAGES - 1][63] = '\0';
    }
}

void network_on_room_joined(const char* room_id) {
    strncpy(current_room_id, room_id, sizeof(current_room_id) - 1);
    current_room_id[sizeof(current_room_id) - 1] = '\0';
    TraceLog(LOG_INFO, "Joined room: %s", current_room_id);
    char log_msg[128];
    sprintf(log_msg, "Joined room: %s", current_room_id);
    addRoomLogMessage(log_msg);
}

void network_on_room_left() {
    TraceLog(LOG_INFO, "Left room: %s", current_room_id);
    char log_msg[128];
    sprintf(log_msg, "Left room: %s", current_room_id);
    addRoomLogMessage(log_msg);
    current_room_id[0] = '\0'; // Clear room ID
}

void network_on_user_joined_room(const char* user_id) {
    TraceLog(LOG_INFO, "User %s joined room %s", user_id, current_room_id);
    char log_msg[128];
    sprintf(log_msg, "User %s joined", user_id);
    addRoomLogMessage(log_msg);
}

void network_on_user_left_room(const char* user_id) {
    TraceLog(LOG_INFO, "User %s left room %s", user_id, current_room_id);
    char log_msg[128];
    sprintf(log_msg, "User %s left", user_id);
    addRoomLogMessage(log_msg);
}

void network_on_ready() {
    isNetworkReady = true;
    TraceLog(LOG_INFO, "Network is ready to send messages.");
    addRoomLogMessage("Network ready!");
}

int main(void)
{
    TraceLog(LOG_INFO, "Initial my_client_id: %s", my_client_id);
    char test_id[64];
    strncpy(test_id, "TEST_UUID_12345", sizeof(test_id) - 1);
    test_id[sizeof(test_id) - 1] = '\0';
    TraceLog(LOG_INFO, "Strncpy test: %s", test_id);
    const int screenWidth = 800;
    const int screenHeight = 450;
    const int gameScreenWidth = 600;
    const int gridSize = 50;

    Rectangle uiPanel = { gameScreenWidth, 0, screenWidth - gameScreenWidth, screenHeight };

    Rectangle rollDiceButton = { uiPanel.x + 20, 60, uiPanel.width - 40, 40 };
    const char* rollDiceText = "Roll Dice";

    Rectangle endTurnButton = { uiPanel.x + 20, 110, uiPanel.width - 40, 40 };
    const char* endTurnText = "End Turn";

    // Room management UI elements
    Rectangle roomInputBox = { uiPanel.x + 20, 180, uiPanel.width - 40, 30 };
    char roomInputText[64] = "new_room";
    int roomInputTextLength = strlen(roomInputText);
    bool roomInputBoxActive = false;

    Rectangle joinRoomButton = { uiPanel.x + 20, 220, (uiPanel.width - 40) / 2 - 5, 30 };
    const char* joinRoomText = "Join";

    Rectangle leaveRoomButton = { uiPanel.x + 20 + (uiPanel.width - 40) / 2 + 5, 220, (uiPanel.width - 40) / 2 - 5, 30 };
    const char* leaveRoomText = "Leave";


    InitWindow(screenWidth, screenHeight, "RayVTT");

    Texture2D tokenTexture = LoadTexture("assets/token.png");

    // Initialize sample tokens
    numTokens = 3;
    tokens[0] = (Token){ 0, 100, 100, (float)gridSize, (float)gridSize };
    tokens[1] = (Token){ 1, 200, 150, (float)gridSize, (float)gridSize };
    tokens[2] = (Token){ 2, 300, 200, (float)gridSize, (float)gridSize };

    bool isDragging = false;
    Vector2 dragOffset = { 0.0f, 0.0f };

    network_init("ws://localhost:8080");
    TraceLog(LOG_INFO, "Client initialized. My ID: %s", my_client_id);

    srand(time(NULL)); // Seed random number generator

    SetTargetFPS(60);

    while (!WindowShouldClose())
    {
        // --- Event Handling ---
        if (IsMouseButtonPressed(MOUSE_BUTTON_LEFT))
        {
            Vector2 mousePoint = GetMousePosition();

            // Check for UI Panel clicks first
            if (CheckCollisionPointRec(mousePoint, uiPanel))
            {
                if (CheckCollisionPointRec(mousePoint, rollDiceButton))
                {
                    if (isNetworkReady) { // Only send if network is ready
                        TraceLog(LOG_INFO, "Client %s sending dice roll message.", my_client_id);
                        int roll = (rand() % 20) + 1;
                        char message[128];
                        sprintf(message, "{\"type\":\"dice_roll\",\"sender_id\":\"%s\",\"message\":\"Player rolled a d20: %d\"}", my_client_id, roll);
                        TraceLog(LOG_INFO, "Sending dice roll message: %s", message);
                        network_send(message);
                    } else {
                        TraceLog(LOG_WARNING, "Network not ready. Dice roll not sent.");
                    }
                }
                else if (CheckCollisionPointRec(mousePoint, endTurnButton))
                {
                    printf("End Turn button clicked!\n");
                }
                else if (CheckCollisionPointRec(mousePoint, roomInputBox))
                {
                    roomInputBoxActive = true;
                }
                else if (CheckCollisionPointRec(mousePoint, joinRoomButton))
                {
                    if (isNetworkReady) { // Only send if network is ready
                        network_join_room(roomInputText);
                        roomInputBoxActive = false;
                    } else {
                        TraceLog(LOG_WARNING, "Network not ready. Join room not sent.");
                    }
                }
                else if (CheckCollisionPointRec(mousePoint, leaveRoomButton))
                {
                    if (isNetworkReady) { // Only send if network is ready
                        network_leave_room();
                        roomInputBoxActive = false;
                    } else {
                        TraceLog(LOG_WARNING, "Network not ready. Leave room not sent.");
                    }
                }
                else {
                    roomInputBoxActive = false;
                }
            }
            // Check for game area clicks
            else
            {
                if (isNetworkReady) { // Only allow token drag if network is ready
                    for (int i = 0; i < numTokens; i++) {
                        if (CheckCollisionPointRec(mousePoint, (Rectangle){ tokens[i].x, tokens[i].y, tokens[i].width, tokens[i].height }))
                        {
                            isDragging = true;
                            draggedToken = &tokens[i];
                            dragOffset.x = mousePoint.x - draggedToken->x;
                            dragOffset.y = mousePoint.y - draggedToken->y;
                            break;
                        }
                    }
                } else {
                    TraceLog(LOG_WARNING, "Network not ready. Token drag not allowed.");
                }
            }
        }

        if (roomInputBoxActive) {
            int key = GetCharPressed();
            while (key > 0) {
                if ((key >= 32) && (key <= 125) && (roomInputTextLength < 63)) {
                    roomInputText[roomInputTextLength] = (char)key;
                    roomInputTextLength++;
                }
                key = GetCharPressed();
            }
            if (IsKeyPressed(KEY_BACKSPACE)) {
                if (roomInputTextLength > 0) {
                    roomInputTextLength--;
                    roomInputText[roomInputTextLength] = '\0';
                }
            }
        }


        if (IsMouseButtonReleased(MOUSE_BUTTON_LEFT))
        {
            if (isDragging && draggedToken != NULL) {
                if (isNetworkReady) { // Only send if network is ready
                    TraceLog(LOG_INFO, "Client %s sending move token message.", my_client_id);
                    // Send token position to server
                    char message[128];
                    sprintf(message, "{\"type\":\"move_token\",\"sender_id\":\"%s\",\"id\":%d,\"x\":%.2f,\"y\":%.2f}", my_client_id, draggedToken->id, draggedToken->x, draggedToken->y);
                    network_send(message);
                } else {
                    TraceLog(LOG_WARNING, "Network not ready. Move token not sent.");
                }
            }
            isDragging = false;
            draggedToken = NULL;
        }

        if (isDragging && draggedToken != NULL)
        {
            Vector2 mousePoint = GetMousePosition();
            draggedToken->x = mousePoint.x - dragOffset.x;
            draggedToken->y = mousePoint.y - dragOffset.y;

            // Constrain token to game area
            if (draggedToken->x < 0) draggedToken->x = 0;
            if (draggedToken->y < 0) draggedToken->y = 0;
            if (draggedToken->x + draggedToken->width > gameScreenWidth) draggedToken->x = gameScreenWidth - draggedToken->width;
            if (draggedToken->y + draggedToken->height > screenHeight) draggedToken->y = screenHeight - draggedToken->height;
        }

        BeginDrawing();

        ClearBackground(RAYWHITE);

        // Draw grid
        for (int i = 0; i <= gameScreenWidth; i += gridSize)
        {
            DrawLine(i, 0, i, screenHeight, LIGHTGRAY);
        }
        for (int i = 0; i <= screenHeight; i += gridSize)
        {
            DrawLine(0, i, gameScreenWidth, i, LIGHTGRAY);
        }

        // Draw all tokens
        for (int i = 0; i < numTokens; i++) {
            DrawTexturePro(tokenTexture, 
                         (Rectangle){ 0, 0, (float)tokenTexture.width, (float)tokenTexture.height }, 
                         (Rectangle){ tokens[i].x, tokens[i].y, tokens[i].width, tokens[i].height }, 
                         (Vector2){ 0, 0 }, 
                         0.0f, 
                         WHITE);
        }

        // Draw UI Panel
        DrawRectangleRec(uiPanel, Fade(LIGHTGRAY, 0.5f));
        DrawText("UI PANEL", uiPanel.x + 20, 20, 20, DARKGRAY);

        // Draw Buttons
        DrawRectangleRec(rollDiceButton, WHITE);
        DrawText(rollDiceText, rollDiceButton.x + 10, rollDiceButton.y + 10, 20, BLACK);

        DrawRectangleRec(endTurnButton, WHITE);
        DrawText(endTurnText, endTurnButton.x + 10, endTurnButton.y + 10, 20, BLACK);

        // Draw Room Management UI
        DrawText(TextFormat("Current Room: %s", current_room_id), uiPanel.x + 20, 160, 10, BLACK);
        DrawRectangleRec(roomInputBox, WHITE);
        if (roomInputBoxActive) {
            DrawRectangleLines((int)roomInputBox.x, (int)roomInputBox.y, (int)roomInputBox.width, (int)roomInputBox.height, RED);
        }
        DrawText(roomInputText, (int)roomInputBox.x + 5, (int)roomInputBox.y + 8, 10, BLACK);

        DrawRectangleRec(joinRoomButton, WHITE);
        DrawText(joinRoomText, joinRoomButton.x + 10, joinRoomButton.y + 10, 20, BLACK);

        DrawRectangleRec(leaveRoomButton, WHITE);
        DrawText(leaveRoomText, leaveRoomButton.x + 10, leaveRoomButton.y + 10, 20, BLACK);

        // Draw Dice Roll Messages
        for (int i = 0; i < numDiceRollMessages; i++) {
            DrawText(diceRollMessages[i], uiPanel.x + 20, 260 + (i * 20), 10, BLACK);
        }

        // Draw Room Log Messages
        for (int i = 0; i < numRoomLogMessages; i++) {
            DrawText(roomLogMessages[i], uiPanel.x + 20, 360 + (i * 15), 10, DARKGRAY);
        }


        EndDrawing();
    }

    UnloadTexture(tokenTexture);
    network_close();
    CloseWindow();

    return 0;
}
