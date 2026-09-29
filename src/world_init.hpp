#pragma once

#include "common.hpp"
#include "tinyECS/tiny_ecs.hpp"
#include "render_system.hpp"

// invaders
Entity createInvader(RenderSystem* renderer, vec2 position);

// towers
Entity createTower(RenderSystem* renderer, vec2 position);
void removeTower(vec2 position);

// A2: add level tile
Entity createLevelTile(RenderSystem* renderer, vec2 position, TEXTURE_ASSET_ID new_tile_id);

// A2: tile-selector tiles
Entity createSelectableTile(RenderSystem* renderer, vec2 position, TEXTURE_ASSET_ID new_tile_id);

// A2: tile cell outline overlay
Entity createFilledTile(RenderSystem* renderer, vec2 position, vec2 size, vec3 color);

// projectile
Entity createProjectile(vec2 pos, vec2 size, vec2 velocity, TEXTURE_ASSET_ID color = TEXTURE_ASSET_ID::PROJECTILE_GOLD);

// explosion effect for destroying the invaders
Entity createExplosion(vec2 position, vec2 size);

// grid lines to show tile positions
Entity createGridLine(vec2 start_pos, vec2 end_pos, vec3 color);

// debugging red lines
Entity createLine(vec2 position, vec2 size);

// legacy
// the player
Entity createChicken(RenderSystem* renderer, vec2 position);