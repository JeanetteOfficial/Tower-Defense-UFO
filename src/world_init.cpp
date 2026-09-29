#include "world_init.hpp"
#include "tinyECS/registry.hpp"
#include <iostream>

// A1: implement grid lines as gridLines with renderRequests and colors
Entity createGridLine(vec2 start_pos, vec2 end_pos, vec3 color)
{
	Entity entity = Entity();

	// create gridLine
	GridLine& gridLine = registry.gridLines.emplace(entity);
	gridLine.start_pos = start_pos;
	gridLine.end_pos = end_pos;

	// re-use the "DEBUG_LINE"
	registry.renderRequests.insert(
		entity,
		{
			TEXTURE_ASSET_ID::TEXTURE_COUNT,
			EFFECT_ASSET_ID::EGG,
			GEOMETRY_BUFFER_ID::DEBUG_LINE
		}
	);

	// A2: use the supplied color
	vec3& lineColor = registry.colors.emplace(entity);
	lineColor = color;

	return entity;
}

// A2: add filled tiles
Entity createFilledTile(RenderSystem* renderer, vec2 position, vec2 size, vec3 color)
{
	// reserve an entity
	auto entity = Entity();

	// A2: add a FilledTile component
	FilledTile& filledTile = registry.filledTiles.emplace(entity);
	filledTile.pos = position;
	filledTile.size = size;
	filledTile.color = color;

	GridLine& gridLine = registry.gridLines.emplace(entity);
	gridLine.start_pos = position;
	gridLine.end_pos = size; // scale

	// re-use the "DEBUG_LINE"
	registry.renderRequests.insert(
		entity,
		{
			TEXTURE_ASSET_ID::TEXTURE_COUNT,
			EFFECT_ASSET_ID::EGG,
			GEOMETRY_BUFFER_ID::DEBUG_LINE
		}
	);

	vec3& lineColor = registry.colors.emplace(entity);
	lineColor = color;

	return entity;
}

// A2: add level tiles
Entity createLevelTile(RenderSystem* renderer, vec2 position, TEXTURE_ASSET_ID new_tile_id)
{
	// reserve an entity
	auto entity = Entity();

	// tile

	// calc tile position when given mouse position coordinates
	int tile_x = (int)(position.x / GRID_CELL_WIDTH_PX);
	int tile_y = (int)(position.y / GRID_CELL_HEIGHT_PX);
	// normalize the position to the cell plus 1/2 height/width, using the original mouse position px
	vec2 norm_position = { tile_x * GRID_CELL_WIDTH_PX + GRID_CELL_WIDTH_PX / 2.f,
						tile_y * GRID_CELL_HEIGHT_PX + GRID_CELL_HEIGHT_PX / 2.f };

	// A2: create Tile
	Tile& tile = registry.tiles.emplace(entity);
	tile.tile_id = new_tile_id;
	tile.tx = tile_x;
	tile.ty = tile_y;

	// store a reference to the potentially re-used mesh object
	Mesh& mesh = renderer->getMesh(GEOMETRY_BUFFER_ID::SPRITE);
	registry.meshPtrs.emplace(entity, &mesh);

	// initialize the position, scale, and physics components
	auto& motion = registry.motions.emplace(entity);
	motion.angle = 0.f;
	motion.velocity = { 0, 0 };
	motion.position = vec2(
		// A2: x, y
		norm_position.x,
		norm_position.y
	);
	motion.scale = vec2({ TILE_BB_WIDTH, TILE_BB_HEIGHT }); // Sets the tile's visual size to the bounding box constants

	registry.renderRequests.insert(
		entity,
		{
			new_tile_id,
			EFFECT_ASSET_ID::TEXTURED,
			GEOMETRY_BUFFER_ID::SPRITE
		}
	);

	return entity;
}

// helper function to convert the map into a graph for the A* search algorithm
void createNode () {

}


// A2: create a selectable tile for the tile-selector screen
Entity createSelectableTile(RenderSystem* renderer, vec2 position, TEXTURE_ASSET_ID new_tile_id)
{
	// A2: create a new (level) tile entity
	// auto entity = Entity(); // Entity(); needs to be replaced, used here to quell compiler warning
	auto entity = createLevelTile(renderer, position, new_tile_id);
	// A2: add the extra "selectable" component
	registry.selectables.emplace(entity);
	return entity;
}


Entity createInvader(RenderSystem* renderer, vec2 position)
{
	// reserve an entity
	auto entity = Entity();

	// invader
	Invader& invader = registry.invaders.emplace(entity);
	invader.range = INVADER_RANGE;
	invader.health = INVADER_HEALTH;
	invader.timer_ms = TOWER_TIMER_MS; // same as the tower
	invader.random_point = (rand() % 5) + 1; // random number from 1 to 5

	// store a reference to the potentially re-used mesh object
	Mesh& mesh = renderer->getMesh(GEOMETRY_BUFFER_ID::SPRITE);
	registry.meshPtrs.emplace(entity, &mesh);

	// initialize the position, scale, and physics components
	auto& motion = registry.motions.emplace(entity);
	motion.angle = 0.f;
	motion.velocity = { 0, 0 };
	motion.position = position;

	// resize, set scale to negative if you want to make it face the opposite way
	// motion.scale = vec2({ -INVADER_BB_WIDTH, INVADER_BB_WIDTH });
	motion.scale = vec2({ INVADER_BB_WIDTH, INVADER_BB_HEIGHT });

	// create an (empty) Bug component to be able to refer to all bug
	registry.eatables.emplace(entity);
	registry.renderRequests.insert(
		entity,
		{
			TEXTURE_ASSET_ID::INVADER,
			EFFECT_ASSET_ID::TEXTURED,
			GEOMETRY_BUFFER_ID::SPRITE
		}
	);

	// std::cout << "INFO: invader position: " << position.x << ", " << position.y << std::endl;

	return entity;
}

Entity createTower(RenderSystem* renderer, vec2 position)
{
	auto entity = Entity();

	// new tower
	auto& t = registry.towers.emplace(entity);
	// t.range = (float)WINDOW_WIDTH_PX / (float)GRID_CELL_WIDTH_PX; // 21 for now
	t.range = TOWER_RANGE; // towers will shoot at invaders that pass within 5 tiles of their position
	t.health = TOWER_HEALTH;
	t.timer_ms = TOWER_TIMER_MS;	// arbitrary for now

	// Store a reference to the potentially re-used mesh object (the value is stored in the resource cache)
	Mesh& mesh = renderer->getMesh(GEOMETRY_BUFFER_ID::SPRITE);
	registry.meshPtrs.emplace(entity, &mesh);

	// Initialize the motion
	auto& motion = registry.motions.emplace(entity);
	motion.angle = 180.f;	// A1-TD: CK: rotate to the left 180 degrees to fix orientation
	motion.velocity = { 0.0f, 0.0f };
	motion.position = position;

	std::cout << "INFO: tower position: " << position.x << ", " << position.y << std::endl;

	// Setting initial values, scale is negative to make it face the opposite way
	motion.scale = vec2({ -TOWER_BB_WIDTH, TOWER_BB_HEIGHT });

	// create an (empty) Tower component to be able to refer to all towers
	registry.deadlys.emplace(entity);
	registry.renderRequests.insert(
		entity,
		{
			TEXTURE_ASSET_ID::TOWER,
			EFFECT_ASSET_ID::TEXTURED,
			GEOMETRY_BUFFER_ID::SPRITE
		}
	);

	return entity;
}

void removeTower(vec2 position) {
	// remove any towers at this position
	for (Entity& tower_entity : registry.towers.entities) {
		// get each tower's position to determine it's row
		const Motion& tower_motion = registry.motions.get(tower_entity);
		
		if (tower_motion.position.x == position.x && 
			tower_motion.position.y == position.y) {
			// remove this tower
			registry.remove_all_components_of(tower_entity);
			std::cout << "tower removed" << std::endl;
			return; // stop iterating after removal
		}
	}
}

// A2: create a new projectile w/ pos, size, & velocity
Entity createProjectile(vec2 pos, vec2 size, vec2 velocity, TEXTURE_ASSET_ID color)
{
	auto entity = Entity();

	// projectile
	Projectile& projectile = registry.projectiles.emplace(entity);
	projectile.damage = PROJECTILE_DAMAGE;
	projectile.pf = color == TEXTURE_ASSET_ID::PROJECTILE_GOLD ? TOWER: INVADER;
	// motion
	auto& motion = registry.motions.emplace(entity);
	motion.angle = 0.f;
	motion.velocity = velocity;
	motion.position = pos;
	motion.scale = size;
	// Deadly component to indicate it can deal damage
	registry.deadlys.emplace(entity);
	// renderRequests
	registry.renderRequests.insert(
		entity,
		{
			color,
			EFFECT_ASSET_ID::TEXTURED,
			GEOMETRY_BUFFER_ID::SPRITE
		}
	);

	return entity;
}

// A2: adding explosion animation for invaders
Entity createExplosion(vec2 position, vec2 size)
{
    auto entity = Entity();

    // Motion (stationary)
    auto& motion = registry.motions.emplace(entity);
    motion.position = position;
    motion.velocity = {0, 0};
    motion.scale = size;
    motion.angle = 0.f;

    // Animation (one-shot)
    Animation& anim = registry.animations.emplace(entity);
    anim.current_frame = 0;
    anim.total_frames = 3;
    anim.frame_time_ms = 150.f;
    anim.timer_ms = 0.f;
    anim.base_texture = TEXTURE_ASSET_ID::EXPLOSION_1; // added animation textures
    anim.looping = false;

    // Render request
    registry.renderRequests.insert(
        entity,
        {
            TEXTURE_ASSET_ID::EXPLOSION_1, // added animation textures
            EFFECT_ASSET_ID::TEXTURED,
            GEOMETRY_BUFFER_ID::SPRITE
        }
    );

    return entity;
}


Entity createLine(vec2 position, vec2 scale)
{
	Entity entity = Entity();

	// Store a reference to the potentially re-used mesh object (the value is stored in the resource cache)
	registry.renderRequests.insert(
		entity,
		{
			// usage TEXTURE_COUNT when no texture is needed, i.e., an .obj or other vertices are used instead
			TEXTURE_ASSET_ID::TEXTURE_COUNT,
			EFFECT_ASSET_ID::EGG,
			GEOMETRY_BUFFER_ID::DEBUG_LINE
		}
	);

	// Create motion
	Motion& motion = registry.motions.emplace(entity);
	motion.angle = 0.f;
	motion.velocity = { 0, 0 };
	motion.position = position;
	motion.scale = scale;

	registry.debugComponents.emplace(entity);
	return entity;
}

// LEGACY
Entity createChicken(RenderSystem* renderer, vec2 pos)
{
	auto entity = Entity();

	// Store a reference to the potentially re-used mesh object
	Mesh& mesh = renderer->getMesh(GEOMETRY_BUFFER_ID::CHICKEN);
	registry.meshPtrs.emplace(entity, &mesh);

	// Setting initial motion values
	Motion& motion = registry.motions.emplace(entity);
	motion.position = pos;
	motion.angle = 0.f;
	motion.velocity = { 0.f, 0.f };
	motion.scale = mesh.original_size * 300.f;
	motion.scale.y *= -1; // point front to the right

	// create an (empty) Chicken component to be able to refer to all towers
	registry.players.emplace(entity);
	registry.renderRequests.insert(
		entity,
		{
			// usage TEXTURE_COUNT when no texture is needed, i.e., an .obj or other vertices are used instead
			TEXTURE_ASSET_ID::TEXTURE_COUNT,
			EFFECT_ASSET_ID::CHICKEN,
			GEOMETRY_BUFFER_ID::CHICKEN
		}
	);

	return entity;
}