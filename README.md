# RayVTT

RayVTT is a virtual tabletop application built with raylib and Emscripten, designed to run in the browser. It aims to be a system-agnostic, easy-to-use, performant, open-source, moddable, and cross-platform solution for online tabletop gaming.

## Core Features

*   Real-time multiplayer
*   Grid-based map system
*   Character sheets
*   Dice rolling
*   Fog of war
*   Asset management

## Data Models

*   **User:**
    *   `id`
    *   `username`
    *   `password` (hashed)
    *   `email`
*   **Game:**
    *   `id`
    *   `name`
    *   `gm` (user id)
    *   `players` (array of user ids)
    *   `maps` (array of map ids)
    *   `characters` (array of character ids)
*   **Map:**
    *   `id`
    *   `name`
    *   `grid_size`
    *   `background_image`
    *   `tokens` (array of token objects)
*   **Character:**
    *   `id`
    *   `name`
    *   `sheet` (json object)
    *   `token` (token object)

## Roadmap

*   **Q1 2026:** Basic prototype with map and token movement.
*   **Q2 2026:** Implement character sheets and dice rolling.
*   **Q3 2026:** Add fog of war and asset management.
*   **Q4 2026:** Alpha release.

---

## ✅ GOAL:

Create a **browser-based** online multiplayer **Virtual Tabletop (VTT)** using **Raylib + Emscripten** with basic features like:

* Multiplayer sync (maps, tokens, chat)
* Drag-and-drop UI
* Dice roller
* Grid system
* Basic networking

---

## 🧭 HIGH-LEVEL PLAN

---

### 1. **Define the Scope (MVP First)**

Build a Minimum Viable Product (MVP) first:

* Basic grid/map display
* Place/move tokens
* Simple multiplayer sync (e.g., WebSocket)
* Basic chat
* Single map
* Browser-compatible with Emscripten

You can layer advanced features later:

* Fog of war
* Character sheets
* Audio/video
* Campaign saving/loading
* Dice animations

---

### 2. **Architecture Overview**

**Client (Raylib + Emscripten)**

* Render UI and map
* Handle user input
* Sync state with server via WebSockets

**Server (Node.js or Rust or Go preferred)**

* Authoritative game state
* Room and player management
* Event broadcasting
* Socket.io / raw WebSocket

---

### 3. **Technology Stack**

| Component   | Tech Choice                                                  |
| ----------- | ------------------------------------------------------------ |
| Client-side | **Raylib + Emscripten (C/C++)**                              |
| Networking  | **WebSockets** via Emscripten's `emscripten_websocket_*` API |
| Server-side | **Node.js + ws** or **Rust (tokio + warp)**                  |
| Assets      | PNG, WAV, etc. served from server or CDN                     |
| Build       | `emcc` for Emscripten builds                                 |
| Storage     | JSON file-based or in-memory (initially)                     |

---

## 🛠️ DEVELOPMENT PLAN (Step-by-step)

---

### 🔹 Phase 1: Project Setup

* [x] Set up raylib project with Emscripten:

  ```bash
  emcc main.c -o index.html -s USE_GLFW=3 -s FULL_ES2=1 -Iraylib/include -Lraylib/lib -lraylib
  ```
* [x] Make sure you can run a simple raylib demo in the browser.
* [x] Set up a basic dev server to serve HTML/JS files (e.g., using Python or Node.js)

---

### 🔹 Phase 2: Render Basic Grid & Tokens

* [x] Draw a grid using raylib (`DrawLine` or `DrawRectangleLines`)
* [x] Allow drag-and-drop of tokens (rectangles or images)
* [x] Handle mouse input to select/move items
* [ ] Abstract basic game state: `Map`, `Token`, `Player`

---

### 🔹 Phase 3: Add Multiplayer Support

* [x] Set up a basic WebSocket server (Node.js or Rust)
* [ ] Implement room management: `join`, `leave`, `broadcast`
* [x] From raylib (via Emscripten), connect using `emscripten_websocket_*` functions
* [x] Send/receive JSON messages: `{"type":"move_token","id":5,"x":100,"y":120}`

---

### 🔹 Phase 4: Sync Game State

* [x] On token move, send message to server
* [x] Server updates state and broadcasts to others
* [x] Clients receive and update positions

Use a simple protocol:

```json
{
  "type": "move_token",
  "token_id": "123",
  "x": 200,
  "y": 150
}
```

---

### 🔹 Phase 5: UI & Chat

* [ ] Add in-game chat (text box + log area)
* [ ] Add player name display
* [ ] Add simple turn indicator or player list

You may need to write your own UI system using raylib or integrate a minimal one.

---

### 🔹 Phase 6: Dice Roller

* [ ] Implement `/roll d20` style parser
* [ ] Display results in chat window
* [ ] Optional: Animate dice roll visually

---

### 🔹 Phase 7: Polish & Host

* [ ] Bundle assets
* [ ] Host via static hosting (e.g., GitHub Pages for client, Heroku/Render for server)
* [ ] Add reconnect logic & error handling

---

## 🧪 Testing Plan

* [ ] Test multiplayer with 2+ clients
* [ ] Test reconnection
* [ ] Test on different browsers (Chrome/Firefox)
* [ ] Test mobile responsiveness (if applicable)

---

## 🧩 Optional Features (Future)

* Save/load game state (local storage or server-side)
* Map import (image drop or upload)
* Dynamic lighting / fog of war
* Token HP bars / stats
* GM tools (hide tokens, fog, notes)

---

## 🗃️ Directory Structure Suggestion

```
vtt-project/
├── client/
│   ├── main.c
│   ├── raylib/
│   ├── assets/
│   └── build/ (index.html, .wasm, etc.)
├── server/
│   ├── index.js
│   └── rooms.js
├── shared/
│   └── protocol.json
├── README.md
└── package.json
```

---

## 🧠 Pro Tips

* Raylib doesn’t provide HTML UI: all UI must be drawn manually unless you integrate an external UI lib like [raygui](https://github.com/raysan5/raygui), which may or may not work well with Emscripten.
* Limit draw calls and texture sizes for web performance.
* Use Emscripten's FS (virtual filesystem) if you need persistent storage (e.g. for campaigns).
* Use versioning on network protocol messages early.
* Use JSON for networking unless you're optimizing for bandwidth.

## 🚀 How to Run RayVTT

Follow these steps to set up, compile, and run the RayVTT application.

### Prerequisites

*   **Node.js:** Make sure Node.js is installed (version 16 or higher recommended).
*   **Emscripten SDK:** Install and activate the Emscripten SDK. You can find instructions on the [Emscripten website](https://emscripten.org/docs/getting_started/downloads.html). Ensure `emcc` is accessible in your path, or note its full path.
*   **http-server:** Install globally using npm:
    ```bash
    npm install -g http-server
    ```

### Server Setup and Run

1.  Navigate to the `server` directory:
    ```bash
    cd /home/dell/gemini_projects/rayVTT/home/dell/gemini_projects/RayVTT/server
    ```
2.  Install Node.js dependencies:
    ```bash
    npm install
    ```
3.  Start the server. For development, it's useful to run it in the foreground to see logs:
    ```bash
    node index.js
    ```
    (To run in the background, use `node index.js &`)

### Client Compilation

1.  Navigate to the `client` directory:
    ```bash
    cd /home/dell/gemini_projects/rayVTT/home/dell/gemini_projects/RayVTT/client
    ```
2.  Compile the client using `emcc`. Replace `/path/to/your/emsdk/emcc` with the actual path to your `emcc` executable if it's not in your system's PATH.
    ```bash
    /home/dell/emsdk/upstream/emscripten/emcc main.c network.c -o index.html -s USE_GLFW=3 -s FULL_ES2=1 -Iraylib/src -Lraylib/raylib -lraylib --preload-file assets/token.png -s ASYNCIFY -s EXPORTED_RUNTIME_METHODS='["allocateUTF8", "UTF8ToString", "stringToUTF8"]', -s EXPORTED_FUNCTIONS='["_network_set_client_id", "_network_on_update_token", "_network_on_add_dice_roll_message", "_network_on_room_joined", "_network_on_room_left", "_network_on_user_joined_room", "_network_on_user_left_room", "_network_on_ready"]
    ```
    *Note: The output HTML file name (`index.html` in this case) will be overwritten with each compilation. If you need to force a browser cache refresh, consider adding a version number to the output filename (e.g., `-o index_v1.0.html`).*

### Serving the Client

1.  Navigate to the `client` directory (if you're not already there):
    ```bash
    cd /home/dell/gemini_projects/rayVTT/home/dell/gemini_projects/RayVTT/client
    ```
2.  Start `http-server` to serve the compiled client files:
    ```bash
    http-server -p 8000 &
    ```

### Accessing the Application

1.  Ensure both the Node.js server and `http-server` are running.
2.  Open your web browser and navigate to:
    ```
    http://localhost:8000/index.html
    ```
    *If you compiled with a versioned output file (e.g., `index_v1.0.html`), use that filename in the URL.*

3.  To test multiplayer and room management, open multiple browser tabs or windows with the same URL.
