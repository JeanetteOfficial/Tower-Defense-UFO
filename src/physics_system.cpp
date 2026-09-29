// internal
#include "physics_system.hpp"
#include "world_init.hpp"
#include <iostream>

// Returns the local bounding coordinates scaled by the current size of the entity
vec2 get_bounding_box(const Motion& motion)
{
	// abs is to avoid negative scale due to the facing direction.
	return { abs(motion.scale.x), abs(motion.scale.y) };
}

// This is a SUPER APPROXIMATE check that puts a circle around the bounding boxes and sees
// if the center point of either object is inside the other's bounding-box-circle. You can
// surely implement a more accurate detection
bool collides(const Motion& motion1, const Motion& motion2)
{
	vec2 dp = motion1.position - motion2.position;
	float dist_squared = dot(dp,dp);
	const vec2 other_bonding_box = get_bounding_box(motion1) / 2.f;
	const float other_r_squared = dot(other_bonding_box, other_bonding_box);
	const vec2 my_bonding_box = get_bounding_box(motion2) / 2.f;
	const float my_r_squared = dot(my_bonding_box, my_bonding_box);
	const float r_squared = max(other_r_squared, my_r_squared);
	if (dist_squared < r_squared)
		return true;
	return false;
}

void PhysicsSystem::step(float elapsed_ms)
{
	// Move each entity that has motion (invaders, projectiles, and even towers [they have 0 for velocity])
	// based on how much time has passed, this is to (partially) avoid
	// having entities move at different speed based on the machine.
	auto& motion_registry = registry.motions;
	for(uint i = 0; i< motion_registry.size(); i++)
	{
		// A1: update motion.position based on step_seconds and motion.velocity
		Motion& motion = motion_registry.components[i];
		Entity entity = motion_registry.entities[i];
		float step_seconds = elapsed_ms / 1000.f;

		// A2: enable motion so the invaders move
		motion.position.x += motion.velocity.x * step_seconds;
		motion.position.y += motion.velocity.y * step_seconds;
		

		// A2: change invader direction when they reach the center of next tile
		if (registry.walkingPaths.has(entity)) {
			// walk the path, bit by bit until the invader reaches the exit, then delete the invader
			WalkingPath& walking_path = registry.walkingPaths.get(entity);

			if (walking_path.hold) { // if the invader has lost its original path, it stops moving
				motion.velocity = vec2(0, 0);
				continue;
			}

			if (walking_path.path.size() > 0) {
				ivec2 next_tile = walking_path.path[0];
				vec2 next_tile_center = vec2(next_tile.x * GRID_CELL_WIDTH_PX + GRID_CELL_WIDTH_PX / 2.f,
											 next_tile.y * GRID_CELL_HEIGHT_PX + GRID_CELL_HEIGHT_PX / 2.f);

				// check if invader has reached the center of the next tile
				vec2 predict_position = motion.position + motion.velocity * step_seconds ;
				if (distance(motion.position, next_tile_center) <= distance(predict_position, next_tile_center) || registry.walkingPaths.get(entity).editing_path ) {
					registry.walkingPaths.get(entity).editing_path = false; // stop editing path once invader starts moving again
					// snap invader to the center of the tile if it's close enough, or if it has somehow passed the tile without landing exactly on it
					if (!registry.walkingPaths.get(entity).editing_path) {
						motion.position = next_tile_center;
						// remove the reached tile from the path
						walking_path.path.erase(walking_path.path.begin());
					}
					// if we have more tiles in the path, update velocity to head towards the center of the next tile
					if (walking_path.path.size() > 0) {
						ivec2 new_next_tile = walking_path.path[0];
						vec2 new_next_tile_center = vec2(new_next_tile.x * GRID_CELL_WIDTH_PX + GRID_CELL_WIDTH_PX / 2.f,
														new_next_tile.y * GRID_CELL_HEIGHT_PX + GRID_CELL_HEIGHT_PX / 2.f);
						vec2 direction = normalize(new_next_tile_center - motion.position);
						motion.velocity = direction * BLUE_INVADER_SPEED; // set velocity towards the next tile
					}
					else {
						// invader has reached the exit, remove it, end game
						// registry.remove_all_components_of(entity);
						std::cout << " [Phy sys] An invader reached the exit!" << std::endl;

					}
				}
			}
		}
	}

	// check for collisions between all moving entities (from A1)
    ComponentContainer<Motion> &motion_container = registry.motions;
	for(uint i = 0; i < motion_container.components.size(); i++)
	{
		Motion& motion_i = motion_container.components[i];
		Entity entity_i = motion_container.entities[i];
		
		// note starting j at i+1 to compare all (i,j) pairs only once (and to not compare with itself)
		for(uint j = i+1; j < motion_container.components.size(); j++)
		{
			Motion& motion_j = motion_container.components[j];
			if (collides(motion_i, motion_j))
			{
				Entity entity_j = motion_container.entities[j];
				// Create a collisions event
				// We are abusing the ECS system a bit in that we potentially insert muliple collisions for the same entity
				// CK: why the duplication, except to allow searching by entity_id
				registry.collisions.emplace_with_duplicates(entity_i, entity_j);
				// registry.collisions.emplace_with_duplicates(entity_j, entity_i);
			}
		}
	}
}