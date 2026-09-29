
#define GL3W_IMPLEMENTATION
#include <gl3w.h>

// stdlib
#include <chrono> // used for high-precision timing
#include <iostream>

// internal
#include "ai_system.hpp"
#include "physics_system.hpp"
#include "render_system.hpp"
#include "world_system.hpp"
#include "tinyECS/registry.hpp"

using Clock = std::chrono::high_resolution_clock; // type alias

// Entry point
int main()
{
	// global systems
	AISystem	  ai_system;
	WorldSystem   world_system;
	RenderSystem  renderer_system;
	PhysicsSystem physics_system;

	// initialize window
	GLFWwindow* window = world_system.create_window();
	if (!window) {
		// Time to read the error message
		std::cerr << "ERROR: Failed to create window.  Press any key to exit" << std::endl;
		getchar();
		return EXIT_FAILURE;
	}

	if (!world_system.start_and_load_sounds()) {
		std::cerr << "ERROR: Failed to start or load sounds." << std::endl;
	} // the game continues even if audio fails

	// initialize the main systems
	renderer_system.init(window);
	world_system.init(&renderer_system);

	// A2: see GAME_SCREEN_ID in components.hpp
	GAME_SCREEN_ID game_screen = world_system.get_game_screen();

	// variable timestep loop
	auto t = Clock::now();
	while (!world_system.is_over()) {
		
		// processes system messages, if this wasn't present the window would become unresponsive
		glfwPollEvents();

		// calculate elapsed times in milliseconds from the previous iteration
		auto now = Clock::now();
		float elapsed_ms = (float)(std::chrono::duration_cast<std::chrono::microseconds>(now - t)).count() / 1000;
		t = now;

		// all screens need to update the window caption
		world_system.update_window_caption();
		world_system.update_outline();

		// A2: draw different game screens
		game_screen = world_system.get_game_screen();

		switch (game_screen) {

			case GAME_SCREEN_ID::INTRO:
				// A3 TODO:
				break;

			case GAME_SCREEN_ID::DRAWING:
				// A2: only draw the level, no updates
				break;

			case GAME_SCREEN_ID::PLAYING:
				// A2: draw all the things and update too
				// std::cout << (float)(std::chrono::duration_cast<std::chrono::microseconds>(Clock::now() - t)).count() / 1000 << " " ;
				world_system.step(elapsed_ms);
				// std::cout << (float)(std::chrono::duration_cast<std::chrono::microseconds>(Clock::now() - t)).count() / 1000 << " " ;
				ai_system.step(elapsed_ms);
				// std::cout << (float)(std::chrono::duration_cast<std::chrono::microseconds>(Clock::now() - t)).count() / 1000 << " " ;
				physics_system.step(elapsed_ms);
				// std::cout << (float)(std::chrono::duration_cast<std::chrono::microseconds>(Clock::now() - t)).count() / 1000 << " " ;
				world_system.handle_collisions();
				// std::cout << (float)(std::chrono::duration_cast<std::chrono::microseconds>(Clock::now() - t)).count() / 1000 << std::endl;
				break;

			case GAME_SCREEN_ID::TILE_SELECTOR:
				// A2: only draw the "selectable" tiles, no updates
				break;

			case GAME_SCREEN_ID::GAME_OVER:
				// A3 TODO:
				break;
		}

		// construct an HUD
		HUD hud;
		hud.curr_score = world_system.curr_score;
		hud.high_score = world_system.high_score;
		hud.new_high_score = world_system.new_high_score; // TODO A3
		hud.level_filename = world_system.world_level_filename;
		hud.max_towers = world_system.max_towers; // TODO A3
		hud.tower_countdown_ms = std::chrono::duration_cast<std::chrono::milliseconds> (TOWER_COUNTDOWN - (Clock::now() - world_system.last_tower_create)).count(); // TODO A3
		hud.tower_countdown_ms = hud.tower_countdown_ms > 0 ? hud.tower_countdown_ms : 0;
		hud.unspawned = world_system.level_invader_max_amount - world_system.invader_amount;
		hud.valid_path = world_system.has_valid_path;
		hud.is_victory = world_system.is_victory;
		hud.floating_texts = world_system.floating_texts; // TODO A3
		hud.show_new_high_score_alert = world_system.new_high_score_timer_ms > 0.f; // TODO A3
		hud.new_high_score_timer_ms = world_system.new_high_score_timer_ms;
		// render the current screen
		renderer_system.draw(game_screen, hud);
	}

	return EXIT_SUCCESS;
}
