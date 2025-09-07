#include "network.h"
#include <emscripten/emscripten.h>
#include <stdio.h>
#include <string.h>
#include <stdlib.h>

// Global variable to store the client's own ID (defined in main.c)
// static char client_id[64] = {0}; // Removed as it's now extern
char current_room_id[64] = {0}; // Initialize current_room_id

// EM_JS functions for WebSocket communication
EM_JS(void, js_websocket_init_internal, (const char* url_cstr), {
    var url = UTF8ToString(url_cstr);
    var reconnectDelay = 1000; // start with 1 second
    var heartbeatInterval;
    var lastPongTime;

    // Persist client ID across reconnects
    if (!localStorage.rayvttClientId) {
        localStorage.rayvttClientId = crypto.randomUUID();
        console.log("Generated new client ID:", localStorage.rayvttClientId);
    }

    function setupWebSocket() {
        var ws = new WebSocket(url);

        ws.onopen = function () {
            console.log("WebSocket connected");

            reconnectDelay = 1000; // reset on success

            // Heartbeat loop
            lastPongTime = Date.now();
            heartbeatInterval = setInterval(function () {
                if (ws.readyState === WebSocket.OPEN) {
                    ws.send(JSON.stringify({ type: "ping" }));
                    if (Date.now() - lastPongTime > 10000) {
                        console.warn("Pong timeout — closing WebSocket");
                        ws.close();
                    }
                }
            }, 5000); // Send ping every 5 seconds

            // Optionally notify server of persistent ID
            ws.send(JSON.stringify({
                type: "reconnect_request",
                client_id: localStorage.rayvttClientId
            }));
        };

        ws.onclose = function () {
            console.warn("WebSocket closed. Reconnecting in " + reconnectDelay + "ms...");
            clearInterval(heartbeatInterval);
            setTimeout(setupWebSocket, reconnectDelay);
            reconnectDelay = Math.min(reconnectDelay * 2, 10000); // exponential backoff, cap at 10s
        };

        ws.onerror = function (event) {
            console.error("WebSocket error:", event);
        };

        ws.onmessage = function (event) {
            var data = event.data;
            try {
                var msg = JSON.parse(data);
                var sender_id_ptr = 0;
                var room_id_ptr = 0;
                var user_id_ptr = 0;

                if (msg.sender_id) {
                    sender_id_ptr = Module.allocateUTF8(msg.sender_id);
                }
                if (msg.roomId) {
                    room_id_ptr = Module.allocateUTF8(msg.roomId);
                }
                if (msg.userId) {
                    user_id_ptr = Module.allocateUTF8(msg.userId);
                }

                if (msg.type === "init_state") {
                    console.log("Received init_state");
                    if (msg.client_id) {
                        localStorage.rayvttClientId = msg.client_id; // Store the client ID
                        var client_id_ptr = Module.allocateUTF8(msg.client_id);
                        _network_set_client_id(client_id_ptr);
                        Module._free(client_id_ptr);
                    }
                    for (var id in msg.tokens) {
                        var tokenData = msg.tokens[id];
                        _network_on_update_token(0, parseInt(tokenData.id), tokenData.x, tokenData.y);
                    }
                    _network_on_ready(); // Call network_on_ready after init_state
                } else if (msg.type === "update_token") {
                    _network_on_update_token(sender_id_ptr, parseInt(msg.id), msg.x, msg.y);
                } else if (msg.type === "dice_roll" || msg.type === "chat_message" || msg.type === "broadcast") {
                    var message_ptr = Module.allocateUTF8(msg.message);
                    _network_on_add_chat_message(sender_id_ptr, message_ptr);
                    Module._free(message_ptr);
                } else if (msg.type === "pong") {
                    lastPongTime = Date.now();
                } else if (msg.type === "room_joined") {
                    _network_on_room_joined(room_id_ptr);
                } else if (msg.type === "room_left") {
                    _network_on_room_left();
                } else if (msg.type === "user_joined") {
                    _network_on_user_joined_room(user_id_ptr);
                } else if (msg.type === "user_left") {
                    _network_on_user_left_room(user_id_ptr);
                }

                if (sender_id_ptr) {
                    Module._free(sender_id_ptr);
                }
                if (room_id_ptr) {
                    Module._free(room_id_ptr);
                }
                if (user_id_ptr) {
                    Module._free(user_id_ptr);
                }

            } catch (e) {
                console.error("Failed to handle WebSocket message:", e, data);
            }
        };

        window.rayvttWebSocket = ws;
    }

    setupWebSocket();
});

EM_JS(void, js_websocket_send_internal, (const char* message_cstr), {
    if (window.rayvttWebSocket && window.rayvttWebSocket.readyState === WebSocket.OPEN) {
        window.rayvttWebSocket.send(UTF8ToString(message_cstr));
    } else {
        console.warn("WebSocket not open. Message not sent: " + UTF8ToString(message_cstr));
    }
});

EM_JS(void, js_websocket_close_internal, (), {
    if (window.rayvttWebSocket) {
        window.rayvttWebSocket.close();
    }
});

// Public network functions
void network_init(const char* url) {
    js_websocket_init_internal(url);
}

void network_send(const char* message) {
    js_websocket_send_internal(message);
}

void network_close() {
    js_websocket_close_internal();
}

void network_join_room(const char* room_id) {
    char message[128];
    sprintf(message, "{\"type\":\"join_room\",\"roomId\":\"%s\"}", room_id);
    network_send(message);
}

void network_leave_room() {
    char message[64];
    sprintf(message, "{\"type\":\"leave_room\"}");
    network_send(message);
}

void network_broadcast(const char* message) {
    char full_message[256];
    sprintf(full_message, "{\"type\":\"broadcast\",\"message\":\"%s\"}", message);
    network_send(full_message);
}

void network_set_client_id(const char* new_client_id) {
    strncpy(my_client_id, new_client_id, sizeof(my_client_id) - 1);
    my_client_id[sizeof(my_client_id) - 1] = '\0';
}
