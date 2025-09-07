const WebSocket = require('ws');
const { v4: uuidv4 } = require('uuid');

const wss = new WebSocket.Server({ port: 8080 });

// Map to store rooms: room_id -> Set<WebSocket>
const rooms = new Map();
const DEFAULT_ROOM = 'lobby';

// Helper function to broadcast messages within a room
function broadcastToRoom(roomId, message, { excludeWs = null } = {}) {
    const room = rooms.get(roomId);
    if (room) {
        room.forEach(client => {
            if (client !== excludeWs && client.readyState === WebSocket.OPEN) {
                client.send(message);
            }
        });
    }
}

// Authoritative game state
let gameState = {
    tokens: {
        "0": { id: 0, x: 100, y: 100 },
        "1": { id: 1, x: 200, y: 150 },
        "2": { id: 2, x: 300, y: 200 }
    }
};

// Heartbeat configuration
const heartbeatInterval = 30 * 1000; // 30 seconds
const heartbeatTimeout = 60 * 1000;  // 60 seconds

wss.on('connection', ws => {
    // Assign a unique ID to the client
    ws.id = uuidv4();
    ws.roomId = DEFAULT_ROOM; // Assign to default room

    // Add client to the default room
    if (!rooms.has(DEFAULT_ROOM)) {
        rooms.set(DEFAULT_ROOM, new Set());
    }
    rooms.get(DEFAULT_ROOM).add(ws);

    console.log(`Client ${ws.id} connected. Assigned to room: ${ws.roomId}`);
    console.log('Total clients in lobby: ' + rooms.get(DEFAULT_ROOM).size);

    // Heartbeat setup for new connection
    ws.isAlive = true;
    ws.on('pong', () => {
        ws.isAlive = true;
    });

    // Send initial state to the newly connected client
    ws.send(JSON.stringify({ type: "init_state", client_id: ws.id, tokens: gameState.tokens }));
    broadcastToRoom(ws.roomId, JSON.stringify({ type: "user_joined", userId: ws.id }), { excludeWs: ws });


    ws.on('message', message => {
        ws.isAlive = true; // Reset heartbeat on any message
        console.log('Received: %s', message);
        try {
            const msg = JSON.parse(message.toString());

            if (msg.type === "reconnect_request") {
                // Reconnect logic: if client_id is provided, try to re-assign
                let foundExisting = false;
                if (typeof msg.client_id === 'string' && msg.client_id.length > 0) {
                    for (let client of rooms.get(ws.roomId) || []) { // Check current room first
                        if (client.id === msg.client_id) {
                            // Transfer properties from old ws to new ws
                            ws.id = client.id;
                            ws.roomId = client.roomId;
                            ws.isAlive = client.isAlive;
                            // Remove old client from room and add new one
                            rooms.get(client.roomId).delete(client);
                            rooms.get(ws.roomId).add(ws);
                            console.log(`Client ${ws.id} reconnected. Re-assigned to room: ${ws.roomId}`);
                            foundExisting = true;
                            break;
                        }
                    }
                    if (!foundExisting) { // If not found in current room, assign new ID and default room
                        ws.id = msg.client_id; // Use requested ID if not found
                        ws.roomId = DEFAULT_ROOM;
                        if (!rooms.has(DEFAULT_ROOM)) {
                            rooms.set(DEFAULT_ROOM, new Set());
                        }
                        rooms.get(DEFAULT_ROOM).add(ws);
                        console.log(`Client ${ws.id} reconnected with new ID. Assigned to room: ${ws.roomId}`);
                    }
                } else {
                    // If no client_id provided in reconnect_request, treat as new connection
                    ws.id = uuidv4();
                    ws.roomId = DEFAULT_ROOM;
                    if (!rooms.has(DEFAULT_ROOM)) {
                        rooms.set(DEFAULT_ROOM, new Set());
                    }
                    rooms.get(DEFAULT_ROOM).add(ws);
                    console.log(`Assigned new client ID (reconnect_request without client_id): ${ws.id}. Assigned to room: ${ws.roomId}`);
                }
                ws.send(JSON.stringify({ type: "init_state", client_id: ws.id, tokens: gameState.tokens }));
                broadcastToRoom(ws.roomId, JSON.stringify({ type: "user_joined", userId: ws.id }), { excludeWs: ws });
                return; // Handled reconnect request, exit message handler
            } else if (msg.type === "join_room") {
                const { roomId } = msg;
                if (typeof roomId !== 'string' || roomId.length === 0) {
                    console.warn("Invalid join_room data received:", msg);
                    return;
                }

                // Remove from current room
                if (ws.roomId && rooms.has(ws.roomId)) {
                    rooms.get(ws.roomId).delete(ws);
                    broadcastToRoom(ws.roomId, JSON.stringify({ type: "user_left", userId: ws.id }));
                    console.log(`Client ${ws.id} left room: ${ws.roomId}`);
                }

                // Add to new room
                ws.roomId = roomId;
                if (!rooms.has(roomId)) {
                    rooms.set(roomId, new Set());
                }
                rooms.get(roomId).add(ws);
                console.log(`Client ${ws.id} joined room: ${ws.roomId}`);
                broadcastToRoom(ws.roomId, JSON.stringify({ type: "user_joined", userId: ws.id }), { excludeWs: ws });
                ws.send(JSON.stringify({ type: "room_joined", roomId: ws.roomId })); // Confirm join
            } else if (msg.type === "leave_room") {
                if (ws.roomId && rooms.has(ws.roomId)) {
                    rooms.get(ws.roomId).delete(ws);
                    broadcastToRoom(ws.roomId, JSON.stringify({ type: "user_left", userId: ws.id }));
                    console.log(`Client ${ws.id} left room: ${ws.roomId}`);
                    ws.roomId = null; // Clear room ID
                    ws.send(JSON.stringify({ type: "room_left" })); // Confirm leave
                }
            } else if (msg.type === "move_token") {
                const { id, x, y } = msg;
                // Input Validation for move_token
                if (typeof id === 'undefined' || (typeof id !== 'string' && typeof id !== 'number') || typeof x !== 'number' || typeof y !== 'number') {
                    console.warn("Invalid move_token data received:", msg);
                    return;
                }
                // Update authoritative state
                if (gameState.tokens[id]) {
                    gameState.tokens[id].x = x;
                    gameState.tokens[id].y = y;
                }

                // Broadcast the updated token to clients in the same room
                console.log(`Server broadcasting update_token from ${ws.id} in room ${ws.roomId}: id=${id}, x=${x}, y=${y}`);
                const updateTokenMessage = JSON.stringify({ type: "update_token", sender_id: ws.id, id, x, y });
                broadcastToRoom(ws.roomId, updateTokenMessage, { excludeWs: ws });
            } else if (msg.type === "dice_roll") {
                // Input Validation for dice_roll
                if (typeof msg.message !== 'string') {
                    console.warn("Invalid dice_roll data received:", msg);
                    return;
                }
                // Broadcast dice roll to clients in the same room
                console.log(`Server broadcasting dice_roll from ${ws.id} in room ${ws.roomId}: ${msg.message}`);
                const diceRollMessage = JSON.stringify({ type: "dice_roll", sender_id: ws.id, message: msg.message });
                broadcastToRoom(ws.roomId, diceRollMessage);
            } else if (msg.type === "chat_message") {
                // Input Validation for chat_message
                if (typeof msg.message !== 'string') {
                    console.warn("Invalid chat_message data received:", msg);
                    return;
                }
                // Broadcast chat message to clients in the same room
                const chatMessage = JSON.stringify({ type: "chat_message", sender_id: ws.id, message: msg.message });
                console.log(`Server sending chat_message from ${ws.id} in room ${ws.roomId}: ${chatMessage}`);
                broadcastToRoom(ws.roomId, chatMessage);
            } else if (msg.type === "broadcast") {
                // Input Validation for broadcast
                if (typeof msg.message !== 'string') {
                    console.warn("Invalid broadcast data received:", msg);
                    return;
                }
                // Broadcast the message to clients in the same room
                const broadcastMessage = JSON.stringify({ type: "broadcast", sender_id: ws.id, message: msg.message });
                console.log(`Server broadcasting message from ${ws.id} in room ${ws.roomId}: ${broadcastMessage}`);
                broadcastToRoom(ws.roomId, broadcastMessage);
            } else if (msg.type === "ping") {
                ws.send(JSON.stringify({ type: "pong" }));
            } else {
                console.warn("Unknown message type received:", msg.type);
            }
        } catch (error) {
            console.error("Failed to parse message or invalid message format: ", error);
        }
    });

    ws.on('close', () => {
        if (ws.roomId && rooms.has(ws.roomId)) {
            rooms.get(ws.roomId).delete(ws);
            broadcastToRoom(ws.roomId, JSON.stringify({ type: "user_left", userId: ws.id }));
            console.log(`Client ${ws.id} disconnected. Left room: ${ws.roomId}. Clients remaining in room: ${rooms.get(ws.roomId).size}`);
            if (rooms.get(ws.roomId).size === 0 && ws.roomId !== DEFAULT_ROOM) {
                rooms.delete(ws.roomId); // Clean up empty rooms (except default)
                console.log(`Room ${ws.roomId} is now empty and deleted.`);
            }
        }
    });

    ws.on('error', error => {
        console.error('WebSocket error: ', error);
    });
});

// Heartbeat interval
const interval = setInterval(() => {
    wss.clients.forEach(ws => {
        if (ws.isAlive === false) {
            console.log(`Client ${ws.id} timed out. Terminating connection.`);
            return ws.terminate();
        }
        ws.isAlive = false;
        ws.ping();
    });
}, heartbeatInterval);

wss.on('close', () => {
    clearInterval(interval);
    console.log('WebSocket server closed.');
});

console.log('WebSocket server started on port 8080');
