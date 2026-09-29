// Header
#include "world_system.hpp"
#include "world_init.hpp"
#include "wfc.hpp"

// stlib
#include <cassert>
#include <sstream>		// string-stream for splitting lines
#include <iostream>
#include <map>
#include <vector>
#include <queue>

// for A2 level loading
#include <fstream>
// for A3 floating texts
#include <algorithm>

#include "physics_system.hpp"
#include "tiles.hpp"
#include "pathing.hpp"

// create the world
WorldSystem::WorldSystem() :
	next_invader_spawn(0),
	invader_spawn_rate_ms(INVADER_SPAWN_RATE_MS),
	max_towers(MAX_TOWERS_START),
	points(0),
	// A2 -> A3 first screen should be the intro screen
	game_screen(GAME_SCREEN_ID::INTRO)
{
	// seeding rng with random device
	rng = std::default_random_engine(std::random_device()());
	// A3 load high score file
	load_high_score("highscore.txt");
}

WorldSystem::~WorldSystem() {
	// Destroy music components
	if (background_music != nullptr)
		Mix_FreeMusic(background_music);
	if (invader_hit_sound != nullptr)
		Mix_FreeChunk(invader_hit_sound);
	if (tower_hit_sound != nullptr)
		Mix_FreeChunk(tower_hit_sound);
	Mix_CloseAudio();

	// Destroy all created components
	registry.clear_all_components();

	// Close the window
	glfwDestroyWindow(window);
}

// Debugging
namespace {
	void glfw_err_cb(int error, const char *desc) {
		std::cerr << error << ": " << desc << std::endl;
	}
}

// call to close the window, wrapper around GLFW commands
void WorldSystem::close_window() {
	glfwSetWindowShouldClose(window, GLFW_TRUE);
}

// World initialization
// Note, this has a lot of OpenGL specific things, could be moved to the renderer
GLFWwindow* WorldSystem::create_window() {

	///////////////////////////////////////
	// Initialize GLFW
	glfwSetErrorCallback(glfw_err_cb);
	if (!glfwInit()) {
		std::cerr << "ERROR: Failed to initialize GLFW in world_system.cpp" << std::endl;
		return nullptr;
	}

	//-------------------------------------------------------------------------
	// If you are on Linux or Windows, you can change these 2 numbers to 4 and 3 and
	// enable the glDebugMessageCallback to have OpenGL catch your mistakes for you.
	// GLFW / OGL Initialization
	glfwWindowHint(GLFW_CONTEXT_VERSION_MAJOR, 3);
	glfwWindowHint(GLFW_CONTEXT_VERSION_MINOR, 3);
	glfwWindowHint(GLFW_OPENGL_PROFILE, GLFW_OPENGL_CORE_PROFILE);
	glfwWindowHint(GLFW_OPENGL_DEBUG_CONTEXT, GL_TRUE);
#if __APPLE__
	glfwWindowHint(GLFW_OPENGL_FORWARD_COMPAT, GL_TRUE);
#endif
	glfwWindowHint(GLFW_RESIZABLE, GL_FALSE);
	// CK: setting GLFW_SCALE_TO_MONITOR to true will rescale window but then you must handle different scalings
	// glfwWindowHint(GLFW_SCALE_TO_MONITOR, GL_TRUE);		// GLFW 3.3+
	glfwWindowHint(GLFW_SCALE_TO_MONITOR, GL_FALSE);		// GLFW 3.3+

	// Create the main window (for rendering, keyboard, and mouse input)
	window = glfwCreateWindow(WINDOW_WIDTH_PX, WINDOW_HEIGHT_PX, "Towers vs Invaders Assignment", nullptr, nullptr);
	if (window == nullptr) {
		std::cerr << "ERROR: Failed to glfwCreateWindow in world_system.cpp" << std::endl;
		return nullptr;
	}

	// Setting callbacks to member functions (that's why the redirect is needed)
	// Input is handled using GLFW, for more info see
	// http://www.glfw.org/docs/latest/input_guide.html
	glfwSetWindowUserPointer(window, this);
	auto key_redirect = [](GLFWwindow* wnd, int _0, int _1, int _2, int _3) { ((WorldSystem*)glfwGetWindowUserPointer(wnd))->on_key(_0, _1, _2, _3); };
	auto cursor_pos_redirect = [](GLFWwindow* wnd, double _0, double _1) { ((WorldSystem*)glfwGetWindowUserPointer(wnd))->on_mouse_move({ _0, _1 }); };
	auto mouse_button_pressed_redirect = [](GLFWwindow* wnd, int _button, int _action, int _mods) { ((WorldSystem*)glfwGetWindowUserPointer(wnd))->on_mouse_button_pressed(_button, _action, _mods); };
	
	glfwSetKeyCallback(window, key_redirect);
	glfwSetCursorPosCallback(window, cursor_pos_redirect);
	glfwSetMouseButtonCallback(window, mouse_button_pressed_redirect);

	return window;
}

bool WorldSystem::start_and_load_sounds() {
	
	//////////////////////////////////////
	// Loading music and sounds with SDL
	if (SDL_Init(SDL_INIT_AUDIO) < 0) {
		fprintf(stderr, "Failed to initialize SDL Audio");
		return false;
	}

	if (Mix_OpenAudio(44100, MIX_DEFAULT_FORMAT, 2, 2048) == -1) {
		fprintf(stderr, "Failed to open audio device");
		return false;
	}

	background_music = Mix_LoadMUS(audio_path("music.wav").c_str());
	invader_hit_sound = Mix_LoadWAV(audio_path("invader_hit.wav").c_str());
	tower_hit_sound = Mix_LoadWAV(audio_path("tower_hit.wav").c_str());

	if (background_music == nullptr || invader_hit_sound == nullptr || tower_hit_sound == nullptr) {
		fprintf(stderr, "Failed to load sounds\n %s\n %s\n %s\n make sure the data directory is present",
			audio_path("music.wav").c_str(),
			audio_path("invader_hit.wav").c_str(),
			audio_path("tower_hit.wav").c_str());
		return false;
	}

	return true;
}

void WorldSystem::init(RenderSystem* renderer_arg) {

	this->renderer = renderer_arg;
	last_tower_create = std::chrono::steady_clock::now() - TOWER_COUNTDOWN;

// CK: disabled starting music for A2
#if 0
	// start playing background music indefinitely
	std::cout << "Starting music..." << std::endl;
	Mix_PlayMusic(background_music, -1);
#endif

	// Set all states to default
    restart_game(true); // reset score
}

// A2: separated from WorldSystem::step for screens that do not use ::step
void WorldSystem::update_window_caption() {

	// update window title with various state variables
	std::stringstream title_ss;
	title_ss << "Level: " << world_level_filename;
	title_ss << " | " << "Screen: " << GAME_SCREEN_ID_NAMES[(int)game_screen];
	title_ss << " | " << "Points: " << points;
	title_ss << " | " << "Max Towers: " << max_towers;
	title_ss << " | " << "Debug: " << (debugging.in_debug_mode ? "true" : "false");
	glfwSetWindowTitle(window, title_ss.str().c_str());
}

// handles the outline of the tile grids in the non-playing screens
void WorldSystem::update_outline() {

	// Remove debug info from the last step
	while (registry.debugComponents.entities.size() > 0){
		registry.remove_all_components_of(registry.debugComponents.entities.back());
	}

	// show the red outline of the tile the player has selected in the tile-selector screen
	if (game_screen == GAME_SCREEN_ID::TILE_SELECTOR) {
		vec3 color = { 1, 0, 0 }; // red
		// find selectable entity where tile.tile_id == drawing_tile
		for (Entity entity : registry.selectables.entities) {
			if (registry.tiles.has(entity) && registry.tiles.get(entity).tile_id == drawing_tile) {
				Tile& tile = registry.tiles.get(entity);
				// the selected position
				vec2 tile_position = { 
					tile.tx * GRID_CELL_WIDTH_PX + GRID_CELL_WIDTH_PX / 2.f,
					tile.ty * GRID_CELL_HEIGHT_PX + GRID_CELL_HEIGHT_PX / 2.f 
				};

        		// draw 4 red gridlines around it (tagged debug)
				vec2 start_pos_top = {tile_position.x, tile_position.y - GRID_CELL_HEIGHT_PX/2};
				vec2 end_pos_top = {GRID_CELL_WIDTH_PX, GRID_LINE_WIDTH_PX};
				Entity debug_gridline_entity_top = createGridLine(start_pos_top, end_pos_top, color);
				vec2 start_pos_bot = {tile_position.x, tile_position.y + GRID_CELL_HEIGHT_PX/2};
				vec2 end_pos_bot = {GRID_CELL_WIDTH_PX, GRID_LINE_WIDTH_PX};
				Entity debug_gridline_entity_bot = createGridLine(start_pos_bot, end_pos_bot, color);
				vec2 start_pos_left = {tile_position.x - GRID_CELL_WIDTH_PX/2, tile_position.y};
				vec2 end_pos_left = {GRID_LINE_WIDTH_PX, GRID_CELL_HEIGHT_PX};
				Entity debug_gridline_entity_left = createGridLine(start_pos_left, end_pos_left, color);
				vec2 start_pos_right = {tile_position.x + GRID_CELL_WIDTH_PX/2, tile_position.y};
				vec2 end_pos_right = {GRID_LINE_WIDTH_PX, GRID_CELL_HEIGHT_PX};
				Entity debug_gridline_entity_right = createGridLine(start_pos_right, end_pos_right, color);
				// tag it with a DebugComponent so it gets removed in the next step
				registry.debugComponents.emplace(debug_gridline_entity_top);
				registry.debugComponents.emplace(debug_gridline_entity_bot);
				registry.debugComponents.emplace(debug_gridline_entity_left);
				registry.debugComponents.emplace(debug_gridline_entity_right);
			}
		}
	}

	// A2: show the darker outlined grid when the mouse hover over tiles in non-playing modes
	// calculate which cell the mouse is in
	int tile_x = (int)(mouse_pos_x / GRID_CELL_WIDTH_PX);
	int tile_y = (int)(mouse_pos_y / GRID_CELL_HEIGHT_PX);
	vec2 position = { tile_x * GRID_CELL_WIDTH_PX + GRID_CELL_WIDTH_PX / 2.f,
						tile_y * GRID_CELL_HEIGHT_PX + GRID_CELL_HEIGHT_PX / 2.f };
	// vec2 size = { GRID_CELL_WIDTH_PX, GRID_CELL_HEIGHT_PX };
	vec3 color = { 0, 0, 0 }; // black
	// create GridLine component to render the tile edge the mouse is hovering over (except for row 0!)
	if (tile_y != 0) {
		vec2 start_pos_top = {position.x, position.y - GRID_CELL_HEIGHT_PX/2};
		vec2 end_pos_top = {GRID_CELL_WIDTH_PX, GRID_LINE_WIDTH_PX};
		Entity debug_gridline_entity_top = createGridLine(start_pos_top, end_pos_top, color);
		vec2 start_pos_bot = {position.x, position.y + GRID_CELL_HEIGHT_PX/2};
		vec2 end_pos_bot = {GRID_CELL_WIDTH_PX, GRID_LINE_WIDTH_PX};
		Entity debug_gridline_entity_bot = createGridLine(start_pos_bot, end_pos_bot, color);
		vec2 start_pos_left = {position.x - GRID_CELL_WIDTH_PX/2, position.y};
		vec2 end_pos_left = {GRID_LINE_WIDTH_PX, GRID_CELL_HEIGHT_PX};
		Entity debug_gridline_entity_left = createGridLine(start_pos_left, end_pos_left, color);
		vec2 start_pos_right = {position.x + GRID_CELL_WIDTH_PX/2, position.y};
		vec2 end_pos_right = {GRID_LINE_WIDTH_PX, GRID_CELL_HEIGHT_PX};
		Entity debug_gridline_entity_right = createGridLine(start_pos_right, end_pos_right, color);
		// tag it with a DebugComponent so it gets removed in the next step
		registry.debugComponents.emplace(debug_gridline_entity_top);
		registry.debugComponents.emplace(debug_gridline_entity_bot);
		registry.debugComponents.emplace(debug_gridline_entity_left);
		registry.debugComponents.emplace(debug_gridline_entity_right);
	}
	
}

// Update our game world
bool WorldSystem::step(float elapsed_ms_since_last_update) {

	// // Remove debug info from the last step
	// while (registry.debugComponents.entities.size() > 0){
	// 	registry.remove_all_components_of(registry.debugComponents.entities.back());
	// }

	// Update animations (only if game is not over)
	if (registry.deathTimers.entities.size() == 0) {

		// A3 TODO: the new high score alert flash timer
		if (new_high_score_timer_ms > 0.f) {
			new_high_score_timer_ms -= elapsed_ms_since_last_update;
		}

		for (int i = (int)registry.animations.entities.size() - 1; i >= 0; --i) {
			Entity entity = registry.animations.entities[i];
			Animation& anim = registry.animations.get(entity);
			anim.timer_ms += elapsed_ms_since_last_update;
			
			if (anim.timer_ms >= anim.frame_time_ms) {
				anim.timer_ms = 0.f;
				// anim.current_frame = (anim.current_frame + 1) % anim.total_frames;
				anim.current_frame += 1;

				if (anim.current_frame >= anim.total_frames) {
					if (anim.looping) { // invader running
						anim.current_frame = 0; // loop back to start
					} else { // explosion
						registry.remove_all_components_of(entity); // explosion done, remove it
						continue; // skip texture update since entity is gone
					}
				}
				// Update the texture in renderRequest
				if (registry.renderRequests.has(entity)) {
					RenderRequest& rr = registry.renderRequests.get(entity);
					rr.used_texture = (TEXTURE_ASSET_ID)((int)anim.base_texture + anim.current_frame);
				}
			}
		}
	}

	// Removing out of screen entities
	auto& motions_registry = registry.motions;

	// Remove entities that leave the screen on the left side
	// Iterate backwards to be able to remove without unterfering with the next object to visit
	// (the containers exchange the last element with the current)
	for (int i = (int)motions_registry.components.size()-1; i>=0; --i) {
	    Motion& motion = motions_registry.components[i];
		if (motion.position.x + abs(motion.scale.x) < 0.f) {
			if(!registry.players.has(motions_registry.entities[i])) // don't remove the player
				registry.remove_all_components_of(motions_registry.entities[i]);
		}
	}

	// A2: spawn INVADER_MAX_AMOUNT invaders in playing mode
	// TODO A3: invaders to spawn for each level = current level + 1 * INVADER_MAX_AMOUNT
	if (game_screen == GAME_SCREEN_ID::PLAYING) {
		if (registry.deathTimers.entities.size() == 0) { // if game is not over

			// A3 TODO: update the floating texts above invader's head
			for (FloatingText& ft : floating_texts) {
				if (ft.active) {
					// move upward (In FreeType screen space, y increases upward)
					ft.y += current_speed * elapsed_ms_since_last_update;
					// check if reached top
					if (ft.y >= WINDOW_HEIGHT_PX) {
						ft.active = false;
					}
				}
			}

			// clean up the inactive floating texts
			floating_texts.erase(
				std::remove_if(floating_texts.begin(), floating_texts.end(), [](const FloatingText& ft) { return !ft.active; }),
				floating_texts.end()
			);

			next_invader_spawn -= elapsed_ms_since_last_update * current_speed;
			// A2: spawn 10 invaders (for A3 we will change this)
			if (next_invader_spawn < 0.f) {
				// reset timer
				next_invader_spawn = INVADER_SPAWN_RATE_MS;

				// A2: create invader if we have a start position
				ivec2 start_pos;
				std::vector<ivec2> path;
				if (find_first_start_tile(start_pos)) { // new invader spawn
					// std::cout << "Start tile found at: " << start_pos.x << ", " << start_pos.y << std::endl;

					// - use find_path(...) to find a path
					if (invader_amount < level_invader_max_amount && find_path(start_pos, path)) {
						// A2: create the invader
						Entity invader = createInvader(renderer, vec2(start_pos.x * GRID_CELL_WIDTH_PX + GRID_CELL_WIDTH_PX / 2.f,
																		start_pos.y * GRID_CELL_HEIGHT_PX + GRID_CELL_HEIGHT_PX / 2.f));
						invader_amount += 1;
						// - attach the path and set the invader to the starting position
						WalkingPath invader_path;
						invader_path.path = path;
						registry.walkingPaths.insert(invader, invader_path);
					}
				}
			}

			// check if the path is edited
			if (map_edited) {
				map_edited = false;
				// if the invader is spawned (has motion), read its current position and update its startPos
				for (Entity &invader : registry.invaders.entities) {
					// std::cout << "Invader already has motion, updating its position to the new start tile." << std::endl;
					Motion& motion = registry.motions.get(invader);
					// std::cout << "Motion position: " << motion.position.x << ", " << motion.position.y << std::endl;
					// std::cout << "Motion velocity: " << motion.velocity.x << ", " << motion.velocity.y << std::endl;

					ivec2 start_pos = ivec2((motion.position.x + motion.velocity.x * elapsed_ms_since_last_update / 1000.f) / GRID_CELL_WIDTH_PX,
										(motion.position.y + motion.velocity.y * elapsed_ms_since_last_update / 1000.f) / GRID_CELL_HEIGHT_PX);
					
					std::vector<ivec2> path_forloop;
					if(!find_path(start_pos, path_forloop)){ // invaders stops moving if the original path is gone
						registry.walkingPaths.get(invader).hold = true;
					} else {
						// change it back if we find a new path after editing
						registry.walkingPaths.get(invader).hold = false; 
					}

					if (path_forloop.size() > 1) {
						registry.walkingPaths.get(invader).editing_path = true;
						registry.walkingPaths.get(invader).path = path_forloop; // update path to new path
					}
				}
			}
		}
	}

	// game over logic: if any invader reaches the exit tile, the game is over
	for (int i = (int)registry.walkingPaths.components.size()-1; i>=0; --i) {
		WalkingPath& walkingPath = registry.walkingPaths.components[i];
		Entity entity = registry.walkingPaths.entities[i];
		if (walkingPath.path.size() == 0 && registry.invaders.has(entity)) { // invader has reached the end of the path, so game over
			is_victory = false;
			std::cout << "Game Over! An invader has reached the exit tile." << std::endl;
			// record high score file
			save_high_score("highscore.txt");
			// Create a new entity just for the death timer
    		Entity game_over_entity = Entity();
			registry.deathTimers.emplace(game_over_entity);

			registry.remove_all_components_of(entity);
			// Stop all invaders and projectiles
			for (int j = (int)motions_registry.components.size()-1; j>=0; --j) {
				Entity e = motions_registry.entities[j];
				if (registry.invaders.has(e) || registry.projectiles.has(e)) {
					motions_registry.components[j].velocity = vec2(0, 0);
					registry.walkingPaths.get(e).hold = true;
				}
			}
        	break; // Game is over, no need to check more
		}
	}

	if (registry.deathTimers.entities.size() == 0) {
		// get the current level number
		int curr_level = world_level_filename[5] - '0';
		// check if all the invaders are dead, if so, the current level is over
		// TODO A3: load the next new level!
		if (registry.invaders.entities.size() == 0 && invader_amount >= level_invader_max_amount) {
			std::cout << "Current Level Completed! All invaders are dead." << std::endl;
			// increase the curr_level number by 1 if it's less than 9
			if (curr_level < 9) {
				curr_level += 1;
				std::string next_level_filename = "level" + std::to_string(curr_level) + ".txt"; // levelx.txt
				// update the world_level_filename to be the next level
				world_level_filename = next_level_filename; 
				path_visualized = false;
				restart_game(false); // do not reset score if it's level advancing
				game_screen = GAME_SCREEN_ID::PLAYING;

			}
			else {
				// TODO A3: player has finished level9.txt, end game, introduce victory game over page
				is_victory = true;

				// record high score
				save_high_score("highscore.txt");
				// Create a new entity just for the death timer
				Entity game_over_entity = Entity();
				registry.deathTimers.emplace(game_over_entity);

				// Stop all invaders and projectiles
				for (int j = (int)motions_registry.components.size()-1; j>=0; --j) {
					Entity e = motions_registry.entities[j];
					if (registry.invaders.has(e) || registry.projectiles.has(e)) {
						motions_registry.components[j].velocity = vec2(0, 0);
					}
				}
			}
		}
	}

	// takes care of game over logic
	if (registry.deathTimers.entities.size() > 0) {
		game_over_fade_out(elapsed_ms_since_last_update);
	}
	return true;
}

// helper function for game_over fade out effect:
void WorldSystem::game_over_fade_out(float elapsed_ms_since_last_update) {
	// A1: game over fade out (not used in A2, need this again in A3)
	assert(registry.screenStates.components.size() <= 1);
    ScreenState &screen = registry.screenStates.components[0];

    float min_counter_ms = 3000.f;
	for (Entity entity : registry.deathTimers.entities) {
		// progress timer
		DeathTimer& counter = registry.deathTimers.get(entity);
		counter.counter_ms -= elapsed_ms_since_last_update;
		if(counter.counter_ms < min_counter_ms){
		    min_counter_ms = counter.counter_ms;
		}
	}
	if (min_counter_ms <= 0) {
		game_screen = GAME_SCREEN_ID::GAME_OVER; 

	}
	// reduce window brightness if any of the present chickens is dying
	screen.darken_screen_factor = 1 - min_counter_ms / 3000;
}



// A2: find first start tile and return true if found with location set in start_tile param
bool WorldSystem::find_first_start_tile(glm::ivec2& start_tile_)
{
	if (start_tile.x == -1 || start_tile.y == -1) {
		return false;
	}
	start_tile_ = this->start_tile;
	return true;
}


// A2: find first exit tile and return true if found with location set in exit_tile param
bool WorldSystem::find_first_exit_tile(glm::ivec2& exit_tile)
{
	if (exit_tile.x == -1 || exit_tile.y == -1) {
		return false;
	}
	exit_tile = this->exit_tile;
	return true;
}

// Reset the world state to its initial state
void WorldSystem::restart_game(bool reset_score) {

	std::cout << "Restarting..." << std::endl;

	// reset screen darkness
	registry.screenStates.components[0].darken_screen_factor = -1;

	// resets the score (lost all progress)
	if (reset_score) {
		curr_score = 0;
		new_high_score_shown = false;
	}
	load_high_score("highscore.txt");

	// debugging for memory/component leaks
	std::cout << "Current registry Entities, before restart" << std::endl;
	registry.list_all_components();

	// reset the game speed
	current_speed = 1.f;

	points = 0;
	max_towers = MAX_TOWERS_START;
	next_invader_spawn = 0;
	invader_amount = 0;
	invader_spawn_rate_ms = INVADER_SPAWN_RATE_MS;

	// remove all motion entities
	while (registry.motions.entities.size() > 0)
	    registry.remove_all_components_of(registry.motions.entities.back());

	// no longer game over
	while (registry.deathTimers.entities.size() > 0)
		registry.remove_all_components_of(registry.deathTimers.entities.back());

	// A2: remove the filled tiles too (b/c they do not have motion)
	//     legacy - only motion elements were removed, but we need to remove other things too
	while (registry.filledTiles.entities.size() > 0)
		registry.remove_all_components_of(registry.filledTiles.entities.back());

	// debugging for memory/component leaks
	std::cout << "Registry Entities after restart" << std::endl;
	registry.list_all_components();


	// A1: create grid lines (fixed in A2 template)
	// create grid lines if they do not already exist
	// NOTE: grid lines 'grow' from the center of their location, so must center lines
	//       move center 1/2 cell height so we can remove the top row
	int center_vertical = (WINDOW_HEIGHT_PX / 2) + (GRID_CELL_HEIGHT_PX / 2);

	if (grid_lines.size() == 0) {
		vec3 grid_line_color = { 0.5f, 0.5f, 0.5f };

		// vertical lines
		for (int col = 1; col < NUM_GRID_CELLS_WIDE; col++) {
			grid_lines.push_back(createGridLine(
				vec2(col * GRID_CELL_WIDTH_PX, center_vertical),
				vec2(GRID_LINE_WIDTH_PX, WINDOW_HEIGHT_PX - GRID_CELL_HEIGHT_PX), // remove the top row
				grid_line_color)
			);
		}

		// horizontal lines (from row 1, not row 0)
		for (int row = 1; row < NUM_GRID_CELLS_HIGH + 1; row++) {
			grid_lines.push_back(createGridLine(
				vec2(0, row * GRID_CELL_HEIGHT_PX),
				vec2(2 * WINDOW_WIDTH_PX, GRID_LINE_WIDTH_PX),
				grid_line_color)
			);
		}
	}


	// A2: create the selectable tiles unless we have already done so
	// only create the selectable tiles once 
	// (tx, ty: grid position counter)
	int tx = 0;
	int ty = 1;

	// loop through all the tile texture IDs and place one selectable tile per cell, advancing tx each time and wrapping to the next row when we reach the end
	if (registry.selectables.size() == 0) {
		// A2: create the selectable tiles for the tile-selector screen
		for (int i = (int)TEXTURE_ASSET_ID::MAPTILE_121; i <= (int)TEXTURE_ASSET_ID::MAPTILE_281; i++) {
			// calculate pixel position from tx, ty
			vec2 position = { tx * GRID_CELL_WIDTH_PX + GRID_CELL_WIDTH_PX / 2.f,
							 ty * GRID_CELL_HEIGHT_PX + GRID_CELL_HEIGHT_PX / 2.f };
			createSelectableTile(renderer, position, (TEXTURE_ASSET_ID)i);
			tx++;
			if (tx >= NUM_GRID_CELLS_WIDE) {
				tx = 0;
				ty++;
			}
		}
	}

	// A2: automatically load current level, if it exists, at restart
	std::cout << "INFO: Reloading level file: " << world_level_filename << std::endl;
	if (!load_level(world_level_filename)) {
		// prints an err msg
		std::cout << "ERROR: failed to load level file: " << world_level_filename << std::endl;
		// but still creates an empty level file
	}
	// update the current level number and so the current max invader amount
	int level = world_level_filename[5] - '0';
	level_invader_max_amount = (level + 1) * INVADER_MAX_AMOUNT;
}

// helper function of find_path
std::vector<ivec2> WorldSystem::get_neighbors(const ivec2& pos) {
	std::vector<ivec2> neighbors;
	tile_dir current_tile = level_tiles[pos.x][pos.y];
	int tile_directions = current_tile.directions();
	// !!!!! DO NOT CHANGE THIS directions vector order !!!!!
	std::vector<ivec2> directions = { {0, -1}, {1, 0}, {0, 1}, {-1, 0} }; // top, right, bottom, left
	
	for (int i = 0; i < directions.size(); i++) {
		ivec2 neighbor_pos = pos + directions[i];
		
		// check if neighbor is within bounds and walkable
		if (neighbor_pos.x >= 0 && neighbor_pos.x < NUM_GRID_CELLS_WIDE &&
			neighbor_pos.y >= 0 && neighbor_pos.y < NUM_GRID_CELLS_HIGH &&
			(tile_directions & (1 << i)) != 0) { // bitwise check if the direction is valid for the neighbor tile
			// also check neighbor's opposite direction
			// top(0)<->bottom(2), right(1)<->left(3)
			int opposite = (i + 2) % 4; // top->bottom, right->left, bottom->top, left->right
			tile_dir neighbor_tile = level_tiles[neighbor_pos.x][neighbor_pos.y];
			int neighbor_directions = neighbor_tile.directions();
			
			if ((neighbor_directions & (1 << opposite)) != 0) {
				neighbors.push_back(neighbor_pos);
			}
		}
	}
	return neighbors;
}

// helper function to calculate Manhattan distance heuristic for A*
int manhattan_distance(const ivec2& a, const ivec2& b) {
	return abs(a.x - b.x) + abs(a.y - b.y);
}

// A2: search for a path from start to exit, only on the yellow tile lines
//     If a path is found, return true and store it in reverse order from
//     exit to start in the vector of ivec2 tile coordinates.
//     If a path is not found, return false.
bool WorldSystem::find_path(ivec2 startPos, std::vector<ivec2> & path)
{

	struct CompareNodes {
	bool operator()(Node* a, Node* b) const {
		return (a->G + a->H) > (b->G + b->H); // min-heap based on F = G + H
	}
	};

	// colors for drawing the path
	bool visualize = (game_screen == GAME_SCREEN_ID::DRAWING);
	vec3 color_red = { 1, 0, 0 }; // red
	vec3 color_green = { 0, 1, 0 }; // green
	vec3 color_blue = { 0, 0, 1 }; // blue
	vec3 color_magenta = { 1, 0, 1 }; // magenta

	// use A*
	// return true when a path is found and return the path via the &path vector

	// array of array of Node
	std::vector<std::vector<Node>> node_grid;
	node_grid.resize(NUM_GRID_CELLS_WIDE);
	for (int i = 0; i < NUM_GRID_CELLS_WIDE; i++) {
		node_grid[i].resize(NUM_GRID_CELLS_HIGH);
		for (int j = 0; j < NUM_GRID_CELLS_HIGH; j++) {
			// position, parent, G, H
			node_grid[i][j] = Node(ivec2(i, j), ivec2(-1, -1), UINT_MAX, UINT_MAX); // initialize all nodes with max G and H values
		}
	}

	std::priority_queue<Node*, std::vector<Node*>, CompareNodes> Open; // nodes to be evaluated

	bool Closed[NUM_GRID_CELLS_WIDE][NUM_GRID_CELLS_HIGH] = {false}; // 21 * 12

	// initialize start node
	ivec2 start = startPos;
	ivec2 exit = this->exit_tile;

	// paint the exit tile red
	vec2 exit_px(exit.x * GRID_CELL_WIDTH_PX + GRID_CELL_WIDTH_PX / 2.f,
             exit.y * GRID_CELL_HEIGHT_PX + GRID_CELL_HEIGHT_PX / 2.f);
	if (visualize)
		createFilledTile(renderer, exit_px, vec2(GRID_CELL_WIDTH_PX, GRID_CELL_HEIGHT_PX), color_red);

	// std::cout << "Finding path from start: (" << start.x << ", " << start.y << ") to exit: (" << exit.x << ", " << exit.y << ")" << std::endl;

    Open.push(&node_grid[start.x][start.y]);
    Closed[start.x][start.y] = true;

    while(!Open.empty()){
        Node* current = Open.top();
        Open.pop();
		// std::cout << "Visiting node: (" << current->position.x << ", " << current->position.y << ")" << std::endl;

		// draw blue for visited nodes (will be overridden by magenta for path nodes)
		vec2 pixel_pos(current->position.x * GRID_CELL_WIDTH_PX + GRID_CELL_WIDTH_PX / 2.f,
					current->position.y * GRID_CELL_HEIGHT_PX + GRID_CELL_HEIGHT_PX / 2.f);
		if (visualize)
			createFilledTile(renderer, pixel_pos, vec2(GRID_CELL_WIDTH_PX, GRID_CELL_HEIGHT_PX), color_blue);

        if (current->position == exit) {
            // path found, reconstruct and return it
            path.clear();
            ivec2 pos = current->position;
            while (pos != start) {
                path.push_back(pos);
                pos = node_grid[pos.x][pos.y].parent;
            }
            path.push_back(start);
            std::reverse(path.begin(), path.end());
			// std::cout << "Path found!!" << std::endl;

			// loop through path tiles and paint them magenta
			for (const ivec2& p : path) {
				vec2 pixel_pos(p.x * GRID_CELL_WIDTH_PX + GRID_CELL_WIDTH_PX / 2.f,
							p.y * GRID_CELL_HEIGHT_PX + GRID_CELL_HEIGHT_PX / 2.f);
				if (visualize)
					createFilledTile(renderer, pixel_pos, vec2(GRID_CELL_WIDTH_PX, GRID_CELL_HEIGHT_PX), color_magenta);
			}

			// repaint exit tile red (always on top)
			if (visualize)
				createFilledTile(renderer, exit_px, vec2(GRID_CELL_WIDTH_PX, GRID_CELL_HEIGHT_PX), color_red);

			has_valid_path = true;
            return true;
        }

        // get neighbors of current node
        std::vector<ivec2> neighbors = get_neighbors(current->position);
        for (const ivec2& neighbor : neighbors) {
			// std::cout << "Checking neighbor: (" << neighbor.x << ", " << neighbor.y << ")" << std::endl;
            if (!Closed[neighbor.x][neighbor.y]) {
                int g = current->G + 1; // uniform cost of 1
                int h = manhattan_distance(neighbor, exit);
                Node& neighbor_node = node_grid[neighbor.x][neighbor.y];
                if (g < neighbor_node.G) {
                    neighbor_node.G = g;
                    neighbor_node.H = h;
                    neighbor_node.parent = current->position;
                    Open.push(&neighbor_node);
                }
            }
        }
    }

	// paint the start tile green if no path found
	vec2 start_px(start.x * GRID_CELL_WIDTH_PX + GRID_CELL_WIDTH_PX / 2.f,
				start.y * GRID_CELL_HEIGHT_PX + GRID_CELL_HEIGHT_PX / 2.f);
	if (visualize)
		createFilledTile(renderer, start_px, vec2(GRID_CELL_WIDTH_PX, GRID_CELL_HEIGHT_PX), color_green);

	// we did not find a valid path...
	std::cout << "Path not found!!" << std::endl;
	has_valid_path = false;
	return false;
}

// Compute collisions between entities
void WorldSystem::handle_collisions() {

	// A2: Loop over all collisions detected by the physics system
	ComponentContainer<Collision>& collision_container = registry.collisions;
	for (uint i = 0; i < collision_container.components.size(); i++) {
		
		// A2: handle collision between projectile and invader
		// get the two entities involved in the collision
		Entity entity_a = collision_container.entities[i];
		Entity entity_b = collision_container.components[i].other;
		// check if it's a projectile <-> invader collision:
		if ((registry.projectiles.has(entity_a) && registry.invaders.has(entity_b)) 
			|| (registry.projectiles.has(entity_b) && registry.invaders.has(entity_a))) {

			Entity projectile_entity;
			Entity invader_entity;

			if (registry.projectiles.has(entity_a)) {
				projectile_entity = entity_a;
				invader_entity = entity_b;
			}
			else {
				projectile_entity = entity_b;
				invader_entity = entity_a;
			}
			if (registry.projectiles.get(projectile_entity).pf != TOWER) {
				continue;
			}
			// // remove projectile
			registry.remove_all_components_of(projectile_entity);
			// std::cout << "Projectile hit invader!" << std::endl;
			// damage invader by PROJECTILE_DAMAGE, print message
			Invader& invader = registry.invaders.get(invader_entity);
			invader.health -= PROJECTILE_DAMAGE;
			// std::cout << "Invader remaining health: " << invader.health << std::endl;
			// check invader's health, if <=0 remove invader, add points, play sound
			if (invader.health <= 0) { 
				points += 1;
				// TODO A3: add the invader's random_score to the curr_score
				curr_score += invader.random_point;
				// check if curr_score has gone over high_score to display the new high score alert
				if (curr_score > high_score && !new_high_score_shown) {
					new_high_score_shown = true;
					new_high_score_timer_ms = 2000.f; // 2 seconds
				}
				// A2: create explosion at invader position
				Motion& inv_motion = registry.motions.get(invader_entity);
				// invader explodes animation!
				createExplosion(inv_motion.position, inv_motion.scale);
				// A3 TODO: invader point floating
				FloatingText fl_txt;
				fl_txt.active = true;
				fl_txt.x = inv_motion.position.x - 26.f;
				fl_txt.y = WINDOW_HEIGHT_PX - inv_motion.position.y + (INVADER_BB_HEIGHT / 2.f);
				fl_txt.point = std::to_string(invader.random_point);
				floating_texts.push_back(fl_txt);

				registry.remove_all_components_of(invader_entity);
				Mix_PlayChannel(-1, invader_hit_sound, 0);
			}
		}
		// no tower <-> invader collisions in A2 since towers can't be placed on the path tiles
		// A3 TODO: if it's a projectile <-> tower collision
		if ((registry.projectiles.has(entity_a) && registry.towers.has(entity_b)) 
			|| (registry.projectiles.has(entity_b) && registry.towers.has(entity_a))) {

			Entity projectile_entity;
			Entity tower_entity;

			if (registry.projectiles.has(entity_a)) {
				projectile_entity = entity_a;
				tower_entity = entity_b;
			}
			else {
				projectile_entity = entity_b;
				tower_entity = entity_a;
			}
			if (registry.projectiles.get(projectile_entity).pf != INVADER) {
				continue;
			}
			// remove projectile
			registry.remove_all_components_of(projectile_entity);
			std::cout << "Projectile hit tower!" << std::endl;
			// damage invader by PROJECTILE_DAMAGE, print message
			Tower& tower = registry.towers.get(tower_entity);
			tower.health -= PROJECTILE_DAMAGE;
			std::cout << "Tower remaining health: " << tower.health << std::endl;
			// check tower's health, if <=0 remove tower, max_tower--
			if (tower.health <= 0) { 
				max_towers--;
				// check if max_towers is below 0
				if (max_towers <= 0) {
					max_towers = 0;
				}
				// A3: create explosion at tower position
				Motion& tow_motion = registry.motions.get(tower_entity);
				// invader explodes animation!
				createExplosion(tow_motion.position, tow_motion.scale);
				registry.remove_all_components_of(tower_entity);
			}
		}
	}

	// !!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!
	// TODO A3: When invaders reach the exit, their walkable path will be empty
	// !!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!
	for (auto& wpe : registry.walkingPaths.entities) {
		// TODO: detect and remove any finished entities
	}

	// Remove all collisions from this simulation step
	registry.collisions.clear();
}

// Should the game be over ?
bool WorldSystem::is_over() const {
	return bool(glfwWindowShouldClose(window));
}


// on key callback
void WorldSystem::on_key(int key, int, int action, int mod) {

	// ESC - exits the game on the intro screen, returns to intro screen on the game over, tile-selector,
	// playing, or drawing screens.
	// TODO A3
	if (action == GLFW_RELEASE && key == GLFW_KEY_ESCAPE) {
		if (game_screen == GAME_SCREEN_ID::INTRO) {
			// exit game
			save_high_score("highscore.txt");
			close_window();
		}
		else {
			// clear all path visualization
			while (registry.filledTiles.entities.size() > 0)
				registry.remove_all_components_of(registry.filledTiles.entities.back());

			// return to intro screen
			game_screen = GAME_SCREEN_ID::INTRO;
			// lose player's progress since they abandoned the game half way
    		curr_score = 0;
			// reset screen darkness
			registry.screenStates.components[0].darken_screen_factor = -1;
			// reset new_high_score since the player has restarted the game
			new_high_score = false;

		}
	}

	// SHIFT - record whether the SHIFT key is depressed or not to support macOS "right-click" --> SHIFT + left-click
	if (key == GLFW_KEY_LEFT_SHIFT || key == GLFW_KEY_RIGHT_SHIFT) {
		if (action == GLFW_RELEASE)
			shift_key_pressed = false;
		else
			shift_key_pressed = true;
	}

	// D - Debugging - not used in A1, but left intact for the debug lines
	// A3 TODO: load the currently selected level in drawing screen
	if (action == GLFW_RELEASE && key == GLFW_KEY_D) {
		if (game_screen == GAME_SCREEN_ID::INTRO) {
			// switch to drawing screen from intro screen for the currently selected level
			restart_game(false); // does not reset the score if the player wants to draw the current level
			game_screen = GAME_SCREEN_ID::DRAWING;
			std::cout << "intro -> drawing screen" << std::endl;
		}
		else { // debug mode when not in intro screen
			if (debugging.in_debug_mode) {
				debugging.in_debug_mode = false;
				std::cout << "INFO: debug_mode disabled" << std::endl;
			}
			else {
				debugging.in_debug_mode = true;
				std::cout << "INFO: debug_mode enabled" << std::endl;
			}
		}
	}

	// A2: E - tile selector toggle
	if (key == GLFW_KEY_E) {
		// clear all path visualization
		while (registry.filledTiles.entities.size() > 0)
			registry.remove_all_components_of(registry.filledTiles.entities.back());

		if (action == GLFW_RELEASE) {

			// toggle between the drawing screen and the tile selector screen
			if (game_screen == GAME_SCREEN_ID::DRAWING) {
				game_screen = GAME_SCREEN_ID::TILE_SELECTOR;
				std::cout << "tile-selector screen" << std::endl;
			}
			else if (game_screen == GAME_SCREEN_ID::TILE_SELECTOR) {
				game_screen = GAME_SCREEN_ID::DRAWING;
				std::cout << "level drawing screen" << std::endl;
			}
		}
	}

	// A2: K - save your level map
	if (key == GLFW_KEY_K) {
		if (action == GLFW_RELEASE) {
			if (save_level(world_level_filename)) {
				std::cout << "INFO: level saved to: " << world_level_filename << std::endl;
				// load_level(world_level_filename); // immediately load the level back to update the level_tiles data structure for pathfinding
			}
			else {
				std::cout << "ERROR: failed to save level: " << world_level_filename << std::endl;
			}
		}
	}

	// A2: L - load level map
	if (key == GLFW_KEY_L) {
		if (action == GLFW_RELEASE) {
			if (load_level(world_level_filename)) {
				path_visualized = false;
				std::cout << "INFO: level loaded: " << world_level_filename << std::endl;
			}
			else {
				std::cout << "ERROR: failed to load level: " << world_level_filename << std::endl;
			}
		}
	}

	// A2: O - calculate path manually and visualize it
	if (action == GLFW_RELEASE && key == GLFW_KEY_O) {
		// clear old visualization
		while (registry.filledTiles.entities.size() > 0)
			registry.remove_all_components_of(registry.filledTiles.entities.back());

		std::vector<ivec2> invader_path;
		if (!find_path(this->start_tile, invader_path)) {
			path_visualized = false;
			std::cout << "ERROR: failed to find path." << std::endl;
		}
		else { // path found! visualized!
			path_visualized = true;
		}
	}

	// A2 -> A3 TODO: P - switch to playing mode 
	// if there is no valid path then an error message is printed on the top row
	if (key == GLFW_KEY_P) {
		if (action == GLFW_RELEASE) {
			// toggle between playing and drawing modes
			if (game_screen == GAME_SCREEN_ID::DRAWING) {
				map_edited = true;
				game_screen = GAME_SCREEN_ID::PLAYING;
				// clear all path visualization on playing screen
				while (registry.filledTiles.entities.size() > 0)
					registry.remove_all_components_of(registry.filledTiles.entities.back());
				
				// update has_valid_path again just in case the playe editted the path
				std::vector<ivec2> invader_path;
				if (!find_path(this->start_tile, invader_path)) {
					path_visualized = false;
					std::cout << "ERROR: failed to find path." << std::endl;
				}
				std::cout << "drawing -> playing screen" << std::endl;
			}
			else if (game_screen == GAME_SCREEN_ID::PLAYING) {
				// if O was once pressed for this current level, then this should show the visualized path
				game_screen = GAME_SCREEN_ID::DRAWING;
				if (path_visualized) {
					// show the visualization of the valid path
					std::vector<ivec2> invader_path;
					if (!find_path(this->start_tile, invader_path)) {
						path_visualized = false;
						std::cout << "ERROR: failed to find path." << std::endl;
					}
				}
				std::cout << "playing -> drawing screen" << std::endl;
			}
		}
	}

	// R - Resetting game on the current level
	// A3 TODO: resets score, max towers, invaders, and towers; removes all projectiles too
	// this should only work on playing screen!
	if (action == GLFW_RELEASE && key == GLFW_KEY_R) {
		if (game_screen == GAME_SCREEN_ID::PLAYING) {
			std::cout << "R key hit on PLAYING screen! Resetting game..." << std::endl;
			path_visualized = false;
			int w, h;
			glfwGetWindowSize(window, &w, &h);
			restart_game(true); // reset score so that if player wants to reset their curr level, they lost all progress from beginning to this level
		}
	}

	// A2: number keys for different levels (0-9)
	// TODO A3: select levels on intro screen, 
	// but then directly takes the player to the corresponding level only after the player pressed Space.
	if (action == GLFW_RELEASE && (key >= GLFW_KEY_0 && key <= GLFW_KEY_9)) {
		int key_num = key - GLFW_KEY_0;
		world_level_filename = "level" + std::to_string(key_num) + ".txt";
		if (game_screen != GAME_SCREEN_ID::INTRO) {
			// reset the visualization of the path
			path_visualized = false;
			// reset to drawing state too
			game_screen = GAME_SCREEN_ID::DRAWING;
			restart_game(true); // reset score so that if player wants to change the level they are playing, they lost all progress from beginning
		}
	}

	// A3 TODO: Space - start the game with the current selected level
	if (action == GLFW_RELEASE && key == GLFW_KEY_SPACE) {
		// clear old visualization
		while (registry.filledTiles.entities.size() > 0)
			registry.remove_all_components_of(registry.filledTiles.entities.back());

		if (game_screen == GAME_SCREEN_ID::INTRO) {
			// TODO A3: load the current selected level in playing mode
			restart_game(true); // reset score to start from the beginning
			game_screen = GAME_SCREEN_ID::PLAYING;
		}
	}

	// TODO A3: G - generates a map and starts the game if on the intro screen
	// OR generates a random level on the drawing screen, which can be saved or played by the player's choice
	if (action == GLFW_RELEASE && key == GLFW_KEY_G) {
		int seed = world_level_filename[5] - '0';
		if (game_screen == GAME_SCREEN_ID::DRAWING) {
			export_txt(level_path(world_level_filename), NUM_GRID_CELLS_WIDE, NUM_GRID_CELLS_HIGH, seed);
			load_level(world_level_filename);

		}
		else if (game_screen == GAME_SCREEN_ID::INTRO) {
			export_txt(level_path(world_level_filename), NUM_GRID_CELLS_WIDE, NUM_GRID_CELLS_HIGH, seed);
			restart_game(true);
			game_screen = GAME_SCREEN_ID::PLAYING;
		}
	}

}

// A2: load a level file
// FORMAT: tile <tile-x> <tile-y> <TEXTURE_ASSET_ID>
bool WorldSystem::load_level(const std::string& filename) {

	// clear the current (non-selectable) tile Entities
	level_tiles.clear();
	start_tile = ivec2(-1, -1);
	exit_tile = ivec2(-1, -1);
	level_tiles.resize(NUM_GRID_CELLS_WIDE); // 21
	for (int i = 0; i < NUM_GRID_CELLS_WIDE; i++) {
		level_tiles[i].clear();
		level_tiles[i].resize(NUM_GRID_CELLS_HIGH); // 12
	}
	
	for (int i = (int)registry.tiles.entities.size() - 1; i >= 0; i--) { // backwards
		Entity entity = registry.tiles.entities[i];
		if (! registry.selectables.has(entity)){ // skip selectables, only load the level tiles
			registry.remove_all_components_of(entity);
		} 
	}

	has_valid_path = false;

	std::cout << "Loading level file: " << filename << std::endl;

	// Open the file at level_path(filename) for reading
	std::ifstream file(level_path(filename));
	if (!file.is_open()) {
		std::cout << "ERROR: failed to open level file: " << filename << std::endl;
		return false;
	}

	// A2: open the file, read each line, parse the format, and create a level tile
	std::string line;
	while (std::getline(file, line)) {
		std::stringstream ss(line);
		std::string token;
		int tx, ty, texture_id;
		ss >> token >> tx >> ty >> texture_id;
		// token -> "tile"
		if (token == "tile") {
        	createLevelTile(renderer, vec2(tx * GRID_CELL_WIDTH_PX + GRID_CELL_WIDTH_PX / 2.f,
										ty * GRID_CELL_HEIGHT_PX + GRID_CELL_HEIGHT_PX / 2.f), 
										(TEXTURE_ASSET_ID)texture_id);
			level_tiles[tx][ty] = TILE_DIRECTIONS[(TEXTURE_ASSET_ID)texture_id]; // store the tile in level_tiles for pathfinding use
    	}
		// check start tile and end tile
		if (START_TILES[(TEXTURE_ASSET_ID)texture_id]) {
			std::cout << "Found start tile at: " << tx << ", " << ty << std::endl;
			start_tile = ivec2(tx, ty);
		}

		if (EXIT_TILES[(TEXTURE_ASSET_ID)texture_id]) {
			std::cout << "Found exit tile at: " << tx << ", " << ty << std::endl;
			exit_tile = ivec2(tx, ty);
		}
	}

	return true;
}

// A2: save a level file
// FORMAT: tile <tile-x> <tile-y> <TEXTURE_ASSET_ID>
bool WorldSystem::save_level(const std::string& filename) {

	std::cout << "Saving level file: " << filename << std::endl;
	// Open a file at level_path(filename) for writing
	std::ofstream file(level_path(filename));
	if (!file.is_open()){
		std::cout << "ERROR: failed to save level file: " << filename << std::endl;
		return false;
	}
	// save the non-selectable tiles using the correct format for loading
	// Loop through registry.tiles.entities, skipping selectables
	for (Entity entity : registry.tiles.entities) {
		if (registry.selectables.has(entity)) continue; // skip selectables, only save the level tiles
        Tile& tile = registry.tiles.get(entity);
		// Write one line per tile
		file << "tile" << " " << tile.tx << " " << tile.ty << " " << (int)tile.tile_id << "\n";
	}

	return true;
}

// TODO A3: save and load the high score file
bool WorldSystem::load_high_score(const std::string& filename) {
	std::cout << "Loading high score file: " << filename << std::endl;
    std::ifstream file(data_path() + "/" + filename); // data/highscore.txt
    if (!file.is_open()) {
        // file doesn't exist yet, high score stays 0
		std::cout << " file does not exist, high score = 0. " << std::endl;
        return false;
    }
	// read one single int
    file >> high_score;
    return true;
}

bool WorldSystem::save_high_score(const std::string& filename) {
	// writing one single int if curr_score > high_score
	if (curr_score > high_score) {
		new_high_score = true;
		std::ofstream file(data_path() + "/" + filename); // data/highscore.txt
		if (!file.is_open()) {
			std::cout << " file does not exist, save high score failed" << std::endl;
			return false;
		}
		file << curr_score;
		high_score = curr_score;
		std::cout << "Saving high score file: " << filename << std::endl;
		return true;
	}
	new_high_score = false;
	return false;
}


void WorldSystem::on_mouse_move(vec2 mouse_position) {

	// record the current mouse position
	mouse_pos_x = mouse_position.x;
	mouse_pos_y = mouse_position.y;
}

void WorldSystem::on_mouse_button_pressed(int button, int action, int mods) {

	// Disable mouse input if game is over
    if (registry.deathTimers.entities.size() > 0) {
        return;
    }

	// on button press
	if (action == GLFW_PRESS) {

		int tile_x = (int)(mouse_pos_x / GRID_CELL_WIDTH_PX);
		int tile_y = (int)(mouse_pos_y / GRID_CELL_HEIGHT_PX);

		// std::cout << "mouse position: " << mouse_pos_x << ", " << mouse_pos_y << std::endl;
		// std::cout << "mouse tile position: " << tile_x << ", " << tile_y << std::endl;


		// A2: left-click adds a tile (or cycles to next tile if already occupied)
		// calculate the tile location based on the mouse location
		// grid
		// int tile_x = (int)(mouse_pos_x / GRID_CELL_WIDTH_PX);
		// int tile_y = (int)(mouse_pos_y / GRID_CELL_HEIGHT_PX);
		// px
		vec2 position = { tile_x * GRID_CELL_WIDTH_PX + GRID_CELL_WIDTH_PX / 2.f,
							tile_y * GRID_CELL_HEIGHT_PX + GRID_CELL_HEIGHT_PX / 2.f };

		if (button == GLFW_MOUSE_BUTTON_LEFT) {
			// on the drawing screen (not row 0):
			if (game_screen == GAME_SCREEN_ID::DRAWING){
				if (tile_y == 0) return; // do not allow editing the top row when mouse click on row 0
			// loop through all the tile entities and check if one exists at the tile location
				for (Entity entity : registry.tiles.entities) {
					if (registry.selectables.has(entity)) continue; // skip selectable tiles on the tile-selector screen
					Tile& tile = registry.tiles.get(entity);
					if (tile.tx == tile_x && tile.ty == tile_y) {
						// - remove any existing tile at the location
						registry.remove_all_components_of(entity);
						break;
					}
				}
				// - create a new level tile using the currently selected drawing_tile
				createLevelTile(renderer, position, drawing_tile);
				// - update level_tiles to reflect the new tile for pathfinding use
				level_tiles[tile_x][tile_y] = TILE_DIRECTIONS[drawing_tile];

			} else if (game_screen == GAME_SCREEN_ID::TILE_SELECTOR) {
				// on the tile-selector screen:
				// - set the selected tile as the drawing_tile
				
				// loop through registry.selectables.entities
				for (Entity entity : registry.selectables.entities) {
					Tile& tile = registry.tiles.get(entity);
					// find the one whose grid position matches the clicked tile_x, tile_y
					if (tile.tx == tile_x && tile.ty == tile_y) {
						// set drawing_tile to that tile's tile_id
						drawing_tile = tile.tile_id;
						break;
					}
				}
			} else if (game_screen == GAME_SCREEN_ID::PLAYING) {
				// A2: place a tower anywhere, except top row or on the path tiles
				if (tile_y != 0 && !is_tile_on_path(tile_x, tile_y)) { // do not allow placing towers on the top row or on the path tiles
					// create tower at this position
					float x_pos = position.x; // center of the cell
					float y_pos = position.y; // center of the cell

					// left-click adds new tower (removing any existing towers), up to max_towers
					// if tower count down is over
					if (last_tower_create + TOWER_COUNTDOWN <= std::chrono::steady_clock::now()) {
						removeTower(vec2(x_pos, y_pos));
						if (registry.towers.entities.size() < max_towers){ // max tower = 5
							createTower(renderer, vec2(x_pos, y_pos));
						}
						last_tower_create = std::chrono::steady_clock::now();
					}
				}

			}
		}

		// A2: right-click (or shift+left-click) removes a tile
		if (button == GLFW_MOUSE_BUTTON_RIGHT
			|| (button == GLFW_MOUSE_BUTTON_LEFT && shift_key_pressed)) {
			if (game_screen == GAME_SCREEN_ID::DRAWING) {
				// on the drawing screen, remove any tiles at the clicked location
				for (Entity entity : registry.tiles.entities) {
					if (registry.selectables.has(entity)) continue; // skip selectable tiles on the tile-selector screen
					Tile& tile = registry.tiles.get(entity);
					if (tile.tx == tile_x && tile.ty == tile_y) {
						// - remove any existing tile at the location
						registry.remove_all_components_of(entity);
						// update level_tiles to reflect the removed tile for pathfinding use
						level_tiles[tile_x][tile_y] = tile_dir(); // empty tile with no directions
						break;
					}
				}
			}
			else if (game_screen == GAME_SCREEN_ID::PLAYING) {
				// on the playing screen, remove any towers at the clicked location
				removeTower(vec2(position.x, position.y));
			}
		}
	}
}

// helper function to check if a tile is on any of the invader paths
bool WorldSystem::is_tile_on_path(int tile_x, int tile_y) {
	// loop through all the walkingPath entities and check if any of them have a path that contains the tile
	for (Entity entity : registry.tiles.entities) {
		// WalkingPath& wp = registry.walkingPaths.get(entity);
		// for (ivec2 path_tile : wp.path) {
		// 	if (path_tile.x == tile_x && path_tile.y == tile_y) {
		// 		return true;
		// 	}
		// }
		if (registry.selectables.has(entity)) continue;
        Tile& tile = registry.tiles.get(entity);
        if (tile.tx == tile_x && tile.ty == tile_y) {
            return true;
        }
	}
	return false;
}