#pragma once

// stlib
#include <fstream> // stdout, stderr..
#include <string>
#include <tuple>
#include <vector>

// glfw (OpenGL)
#define NOMINMAX
#include <gl3w.h>
#include <GLFW/glfw3.h>

// The glm library provides vector and matrix operations as in GLSL
#include <glm/vec2.hpp>				// vec2
#include <glm/ext/vector_int2.hpp>  // ivec2
#include <glm/vec3.hpp>             // vec3
#include <glm/mat3x3.hpp>           // mat3
using namespace glm;

#include "tinyECS/tiny_ecs.hpp"

// Simple utility functions to avoid mistyping directory name
// audio_path("audio.ogg") -> data/audio/audio.ogg
// Get defintion of PROJECT_SOURCE_DIR from:
#include "../ext/project_path.hpp"
inline std::string data_path() { return std::string(PROJECT_SOURCE_DIR) + "data"; };
inline std::string shader_path(const std::string& name) {return std::string(PROJECT_SOURCE_DIR) + "/shaders/" + name;};
inline std::string textures_path(const std::string& name) {return data_path() + "/textures/" + std::string(name);};
inline std::string audio_path(const std::string& name) {return data_path() + "/audio/" + std::string(name);};
inline std::string mesh_path(const std::string& name) {return data_path() + "/meshes/" + std::string(name);};
// A2
inline std::string level_path(const std::string& name) { return data_path() + "/levels/" + std::string(name); };

//
// game constants
//
// A1
// const int WINDOW_WIDTH_PX = 840;
// const int WINDOW_HEIGHT_PX = 600;

// A2: number of grid cells increased from 14x10 (A1) to 21x12 (A2)
const int NUM_GRID_CELLS_WIDE = 21;
const int NUM_GRID_CELLS_HIGH = 12;

const int GRID_CELL_WIDTH_PX = 60;
const int GRID_CELL_HEIGHT_PX = 60;
const int GRID_LINE_WIDTH_PX = 2;

const int WINDOW_WIDTH_PX = NUM_GRID_CELLS_WIDE * GRID_CELL_WIDTH_PX; // 1260
const int WINDOW_HEIGHT_PX = NUM_GRID_CELLS_HIGH * GRID_CELL_HEIGHT_PX;

const int TOWER_TIMER_MS = 250;	// updated number of milliseconds between tower shots
const int MAX_TOWERS_START = 5;
const int TOWER_HEALTH = 40;
const float TOWER_RANGE = 5.f;
const std::chrono::duration TOWER_COUNTDOWN = std::chrono::seconds{2};

const int INVADER_HEALTH = 50;
const int INVADER_SPAWN_RATE_MS = 5 * 1000;
const int INVADER_MAX_AMOUNT = 10;
const float INVADER_RANGE = 4.f;

// A2: speed of the blue invader on the playing screen
const float BLUE_INVADER_SPEED = 80.f;

const int PROJECTILE_DAMAGE = 10;
const float PROJECTILE_SPEED = 200.f; // can change later

// These are hard coded to the dimensions of the entity's texture

// invaders are 64x64 px, but cells are 60x60
const float INVADER_BB_WIDTH = (float)GRID_CELL_WIDTH_PX;
const float INVADER_BB_HEIGHT = (float)GRID_CELL_HEIGHT_PX;

// towers are 64x64 px, but cells are 60x60
const float TOWER_BB_WIDTH = (float)GRID_CELL_WIDTH_PX;
const float TOWER_BB_HEIGHT = (float)GRID_CELL_HEIGHT_PX;

// tiles are 64x64 px, but cells are 60x60
const float TILE_BB_WIDTH = (float)GRID_CELL_WIDTH_PX;
const float TILE_BB_HEIGHT = (float)GRID_CELL_HEIGHT_PX;

#ifndef M_PI
#define M_PI 3.14159265358979323846f
#endif

// The 'Transform' component handles transformations passed to the Vertex shader
// (similar to the gl Immediate mode equivalent, e.g., glTranslate()...)
// We recommend making all components non-copyable by derving from ComponentNonCopyable
struct Transform {
	mat3 mat = { { 1.f, 0.f, 0.f }, { 0.f, 1.f, 0.f}, { 0.f, 0.f, 1.f} }; // start with the identity
	void scale(vec2 scale);
	void rotate(float radians);
	void translate(vec2 offset);
};

bool gl_has_errors(char*, int);

// for invader points animation
struct FloatingText {
	float x;
	float y;
	bool active; // whether the point has successfully float outta screen or not
	std::string point;

}; 

// for HUD text rendering
struct HUD {
	int curr_score;
	int high_score;
	bool new_high_score; // this is for the check when the player has reached a new high score
	int unspawned;
	int max_towers;
	int tower_countdown_ms;
	bool valid_path;
	bool is_victory;
	bool show_new_high_score_alert; 
	float new_high_score_timer_ms;
	std::string level_filename;
	std::vector<FloatingText> floating_texts;
};

