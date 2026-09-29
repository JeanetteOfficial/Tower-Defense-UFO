#include <map>
#include "tinyECS/components.hpp"

#pragma once

// A2: new file that holds tile details for pathing (tile_dir)

// NOTE: this must be _manually_ aligned to components
// top (0), right (1), bottom (2), left(3)
struct tile_dir {
    bool top;
    bool right;
    bool bottom;
    bool left;

    tile_dir() { top = false; right = false; bottom = false; left = false; }
    tile_dir(bool t, bool r, bool b, bool l) : top(t), right(r), bottom(b), left(l) {}
    
    int directions() {
        int direction = 0;
        if (top) direction+=1; // 0001
        if (right) direction+=2; // 0010
        if (bottom) direction+=4; // 0100
        if (left) direction+=8; // 1000
        return direction;
    }

};

// inline: there's exactly one instance of this map shared across all files that include this header, without causing linker errors
// Given a tile's texture, can look up which sides are passable
static inline std::map<TEXTURE_ASSET_ID, tile_dir>
TILE_DIRECTIONS = {

    /* regular tiles for path */
    {TEXTURE_ASSET_ID::MAPTILE_121, tile_dir(false, true,  true,  false)},
    {TEXTURE_ASSET_ID::MAPTILE_122, tile_dir(false, false, true,  true )},
    {TEXTURE_ASSET_ID::MAPTILE_123, tile_dir(true,  true,  false, false)},
    {TEXTURE_ASSET_ID::MAPTILE_124, tile_dir(true,  false, false, true )},
    {TEXTURE_ASSET_ID::MAPTILE_125, tile_dir(false, true,  true,  true )},
    {TEXTURE_ASSET_ID::MAPTILE_126, tile_dir(true,  false, true,  false)},
    {TEXTURE_ASSET_ID::MAPTILE_127, tile_dir(false, true,  false, true )},
    {TEXTURE_ASSET_ID::MAPTILE_128, tile_dir(true,  true,  true,  true )},
    {TEXTURE_ASSET_ID::MAPTILE_129, tile_dir(false, false, true,  false)},
    {TEXTURE_ASSET_ID::MAPTILE_130, tile_dir(false, false, false, true )},
    {TEXTURE_ASSET_ID::MAPTILE_142, tile_dir(true,  true,  false, true )},
    {TEXTURE_ASSET_ID::MAPTILE_143, tile_dir(true,  true,  true,  false)},
    {TEXTURE_ASSET_ID::MAPTILE_144, tile_dir(true,  false, true,  true )},
    {TEXTURE_ASSET_ID::MAPTILE_146, tile_dir(true,  false, false, false)},
    {TEXTURE_ASSET_ID::MAPTILE_147, tile_dir(false, true,  false, false)},

    /* start tiles */
    {TEXTURE_ASSET_ID::MAPTILE_155, tile_dir(false, true,  true,  false)},
    {TEXTURE_ASSET_ID::MAPTILE_156, tile_dir(false, false, true,  true )},
    {TEXTURE_ASSET_ID::MAPTILE_159, tile_dir(false, true,  true,  true )},
    {TEXTURE_ASSET_ID::MAPTILE_160, tile_dir(true,  false, true,  false)},
    {TEXTURE_ASSET_ID::MAPTILE_161, tile_dir(false, true,  false, true )},
    {TEXTURE_ASSET_ID::MAPTILE_162, tile_dir(true,  true,  true,  true )},
    {TEXTURE_ASSET_ID::MAPTILE_163, tile_dir(false, false, true,  false)},
    {TEXTURE_ASSET_ID::MAPTILE_164, tile_dir(false, false, false, true )},
    {TEXTURE_ASSET_ID::MAPTILE_172, tile_dir(true,  true,  false, false)},
    {TEXTURE_ASSET_ID::MAPTILE_173, tile_dir(true,  false, false, true )},
    {TEXTURE_ASSET_ID::MAPTILE_176, tile_dir(true,  true,  false, true )},
    {TEXTURE_ASSET_ID::MAPTILE_177, tile_dir(true,  true,  true,  false)},
    {TEXTURE_ASSET_ID::MAPTILE_178, tile_dir(true,  false, true,  true )},
    {TEXTURE_ASSET_ID::MAPTILE_180, tile_dir(true,  false, false, false)},
    {TEXTURE_ASSET_ID::MAPTILE_181, tile_dir(false, true,  false, false)},

    /* exit tiles */
    {TEXTURE_ASSET_ID::MAPTILE_255, tile_dir(false, true,  true,  false)},
    {TEXTURE_ASSET_ID::MAPTILE_256, tile_dir(false, false, true,  true )},
    {TEXTURE_ASSET_ID::MAPTILE_259, tile_dir(false, true,  true,  true )},
    {TEXTURE_ASSET_ID::MAPTILE_260, tile_dir(true,  false, true,  false)},
    {TEXTURE_ASSET_ID::MAPTILE_261, tile_dir(false, true,  false, true )},
    {TEXTURE_ASSET_ID::MAPTILE_262, tile_dir(true,  true,  true,  true )},
    {TEXTURE_ASSET_ID::MAPTILE_263, tile_dir(false, false, true,  false)},
    {TEXTURE_ASSET_ID::MAPTILE_264, tile_dir(false, false, false, true )},
    {TEXTURE_ASSET_ID::MAPTILE_272, tile_dir(true,  true,  false, false)},
    {TEXTURE_ASSET_ID::MAPTILE_273, tile_dir(true,  false, false, true )},
    {TEXTURE_ASSET_ID::MAPTILE_276, tile_dir(true,  true,  false, true )},
    {TEXTURE_ASSET_ID::MAPTILE_277, tile_dir(true,  true,  true,  false)},
    {TEXTURE_ASSET_ID::MAPTILE_278, tile_dir(true,  false, true,  true )},
    {TEXTURE_ASSET_ID::MAPTILE_280, tile_dir(true,  false, false, false)},
    {TEXTURE_ASSET_ID::MAPTILE_281, tile_dir(false, true,  false, false)}
};

// a set of all texture IDs that count as start tiles
static inline std::map<TEXTURE_ASSET_ID, bool>
START_TILES = {
    {TEXTURE_ASSET_ID::MAPTILE_155, true},
    {TEXTURE_ASSET_ID::MAPTILE_156, true},
    {TEXTURE_ASSET_ID::MAPTILE_159, true},
    {TEXTURE_ASSET_ID::MAPTILE_160, true},
    {TEXTURE_ASSET_ID::MAPTILE_161, true},
    {TEXTURE_ASSET_ID::MAPTILE_162, true},
    {TEXTURE_ASSET_ID::MAPTILE_163, true},
    {TEXTURE_ASSET_ID::MAPTILE_164, true},
    {TEXTURE_ASSET_ID::MAPTILE_172, true},
    {TEXTURE_ASSET_ID::MAPTILE_173, true},
    {TEXTURE_ASSET_ID::MAPTILE_176, true},
    {TEXTURE_ASSET_ID::MAPTILE_177, true},
    {TEXTURE_ASSET_ID::MAPTILE_178, true},
    {TEXTURE_ASSET_ID::MAPTILE_180, true},
    {TEXTURE_ASSET_ID::MAPTILE_181, true}
};

// a set of all texture IDs that count as exit tiles
static inline std::map<TEXTURE_ASSET_ID, bool>
EXIT_TILES = {
    {TEXTURE_ASSET_ID::MAPTILE_255, true},
    {TEXTURE_ASSET_ID::MAPTILE_256, true},
    {TEXTURE_ASSET_ID::MAPTILE_259, true},
    {TEXTURE_ASSET_ID::MAPTILE_260, true},
    {TEXTURE_ASSET_ID::MAPTILE_261, true},
    {TEXTURE_ASSET_ID::MAPTILE_262, true},
    {TEXTURE_ASSET_ID::MAPTILE_263, true},
    {TEXTURE_ASSET_ID::MAPTILE_264, true},
    {TEXTURE_ASSET_ID::MAPTILE_272, true},
    {TEXTURE_ASSET_ID::MAPTILE_273, true},
    {TEXTURE_ASSET_ID::MAPTILE_276, true},
    {TEXTURE_ASSET_ID::MAPTILE_277, true},
    {TEXTURE_ASSET_ID::MAPTILE_278, true},
    {TEXTURE_ASSET_ID::MAPTILE_280, true},
    {TEXTURE_ASSET_ID::MAPTILE_281, true}
};
