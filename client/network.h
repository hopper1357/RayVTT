#ifndef NETWORK_H
#define NETWORK_H

extern char my_client_id[64]; // Declare my_client_id as extern
extern char current_room_id[64]; // Declare current_room_id as extern

#include "raylib.h" // For TraceLog, etc.

// Function to initialize the WebSocket connection
void network_init(const char* url);

// Function to send a message over WebSocket
void network_send(const char* message);

// Function to close the WebSocket connection
void network_close();

// Functions for room management
void network_join_room(const char* room_id);
void network_leave_room();

// Callback functions for game logic (to be implemented in main.c or game.c)
// These will be called by the network module when relevant messages are received
void network_on_update_token(const char* sender_id, int id, float x, float y);
void network_on_add_dice_roll_message(const char* sender_id, const char* message);
void network_on_room_joined(const char* room_id);
void network_on_room_left();
void network_on_user_joined_room(const char* user_id);
void network_on_user_left_room(const char* user_id);
void network_on_ready(); // New callback for network ready

// Function to set the client's own ID
void network_set_client_id(const char* client_id);

#endif // NETWORK_H
