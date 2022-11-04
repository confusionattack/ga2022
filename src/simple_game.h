#pragma once

// Simple Test Game
// Brings together major engine systems to make a very simple "game."

typedef struct simple_game_t simple_game_t;

typedef struct fs_t fs_t;
typedef struct heap_t heap_t;
typedef struct render_t render_t;
typedef struct wm_window_t wm_window_t;

// Create an instance of simple test game.
simple_game_t* simple_game_create(heap_t* heap, fs_t* fs, wm_window_t* window, render_t* render);

// Destroy an instance of simple test game.
void simple_game_destroy(simple_game_t* game);

// Per-frame update for our simple test game.
void simple_game_update(simple_game_t* game);
