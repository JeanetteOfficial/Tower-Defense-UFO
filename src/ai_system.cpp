#include <iostream>
#include <cmath>
#include "ai_system.hpp"
#include "world_init.hpp"

void AISystem::step(float elapsed_ms)
{
	// Don't shoot if game is over
    if (registry.deathTimers.entities.size() > 0) { // game is over
        return;
    }
	// A2: scan for invaders and shoot at them
	// invader detection system for towers
	// - for each tower, scan the radius of 5 tiles of their position:
	//   - if an invader is detected and the tower's shooting timer has expired,
	//     then shoot in that direction (create a projectile) and reset the tower's shot timer
	for (const Entity& tower_entity : registry.towers.entities) {
		// get each tower's position to determine it's position
		Motion& tower_motion = registry.motions.get(tower_entity); // no longer const
		float tower_y_pos = tower_motion.position.y;
		float tower_x_pos = tower_motion.position.x;

		// get the tower's shooting timer
		Tower& tower = registry.towers.get(tower_entity);

		// progress the tower's shooting timer
		tower.timer_ms -= elapsed_ms;

		// scan for invaders within the tower's range
		for (const Entity& invader_entity : registry.invaders.entities) {
			const Motion& invader_motion = registry.motions.get(invader_entity);
			float invader_y_pos = invader_motion.position.y;
			float invader_x_pos = invader_motion.position.x;
			// convert to tile units
			float dx = (invader_x_pos - tower_x_pos) / GRID_CELL_WIDTH_PX;
			float dy = (invader_y_pos - tower_y_pos) / GRID_CELL_HEIGHT_PX;
			float distance = sqrt(dx*dx + dy*dy);
			// calculate target angle from tower to invader (in degrees)
            // use actual pixel dx/dy not tile units for angle calculation
			float pixel_dx = invader_x_pos - tower_x_pos;
			float pixel_dy = invader_y_pos - tower_y_pos;
			float target_angle = std::atan2(pixel_dx, pixel_dy) * (180.f / M_PI) - 90.f; // might need to add an offset here
			if (distance <= tower.range) {
            	// calculate shortest angle_diff to prepare for rotation
				float angle_diff = target_angle - tower_motion.angle;
				// normalize to [-180, 180] range
				while (angle_diff > 180.f) angle_diff -= 360.f;
				while (angle_diff < -180.f) angle_diff += 360.f;

         		// smoothly rotate tower_motion.angle toward target_angle
				float rotation_speed = 180.f; // degrees per second
				float max_rotation = rotation_speed * elapsed_ms / 1000.f;
				if (std::abs(angle_diff) < max_rotation) {
					tower_motion.angle = target_angle; // snap to target if close enough
				} 
				else {
					tower_motion.angle += (angle_diff > 0 ? max_rotation : -max_rotation);
				}
            	// only shoot if timer expired AND angle_diff is small enough
				if (tower.timer_ms <= 0.f && std::abs(angle_diff) < 10.f) {
					// create projectile at the tower's position
					vec2 projectile_pos = vec2(tower_x_pos, tower_y_pos);
					// moving towards the invader
					vec2 projectile_velocity = normalize(vec2(invader_x_pos - tower_x_pos, invader_y_pos - tower_y_pos)) * PROJECTILE_SPEED;
					vec2 projectile_size = vec2(15.0f, 15.0f); // can change later
					createProjectile(projectile_pos, projectile_size, projectile_velocity);

					// reset tower's shooting timer
					tower.timer_ms = TOWER_TIMER_MS;
					break;
				}
				break; // only shoot one invader at a time
			}
		}
	}

	// invaders scan for towers and shoot at them
	for (const Entity& invader_entity : registry.invaders.entities) {
		// get each tower's position to determine it's position
		Motion& invader_motion = registry.motions.get(invader_entity); // no longer const
		float invader_y_pos = invader_motion.position.y;
		float invader_x_pos = invader_motion.position.x;

		// get the invader's shooting timer
		Invader& invader = registry.invaders.get(invader_entity);
		// progress the invader's shooting timer
		invader.timer_ms -= elapsed_ms;
		// scan for towers within the invader's range
		for (const Entity& tower_entity : registry.towers.entities) {
			const Motion& tower_motion = registry.motions.get(tower_entity);
			float tower_y_pos = tower_motion.position.y;
			float tower_x_pos = tower_motion.position.x;
			// convert to tile units
			float dx = (tower_x_pos - invader_x_pos) / GRID_CELL_WIDTH_PX;
			float dy = (tower_y_pos - invader_y_pos) / GRID_CELL_HEIGHT_PX;
			float distance = sqrt(dx*dx + dy*dy);
			if (distance <= invader.range && invader.timer_ms <= 0.f) {
            	// only shoot if timer expired
				// create projectile at the tower's position
				vec2 projectile_pos = vec2(invader_x_pos, invader_y_pos);
				// moving towards the tower
				vec2 projectile_velocity = normalize(vec2(tower_x_pos - invader_x_pos, tower_y_pos - invader_y_pos)) * PROJECTILE_SPEED;
				vec2 projectile_size = vec2(15.0f, 15.0f); // can change later
				createProjectile(projectile_pos, projectile_size, projectile_velocity, TEXTURE_ASSET_ID::PROJECTILE_WHITE);

				// reset tower's shooting timer
				invader.timer_ms = TOWER_TIMER_MS;
				break; // only shoot one tower at a time
			}
		}
	}


}