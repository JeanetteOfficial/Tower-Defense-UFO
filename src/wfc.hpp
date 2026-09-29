#pragma once

#include <vector>
#include <set>
#include <map>
#include <string>
#include <random>
#include <queue>
#include <algorithm>
#include <stdexcept>

enum Dir {
    UP = 0,
    DOWN = 1,
    LEFT = 2,
    RIGHT = 3
};

static const Dir OPPOSITE[] = {
    DOWN, UP, RIGHT, LEFT
};

static const int DX[] = {
    0, 0, -1, 1
};

static const int DY[] = {
    -1, 1, 0, 0
};

struct WFC_Tile {
    std::string name;
    std::set<Dir> connections;
    double weight;
};

struct TileIds { int reg, start, exit; };

struct MapResult {
    std::vector<std::vector<std::string>> names;
    std::pair<int,int> start, goal;
    unsigned used_seed;
};

std::vector<WFC_Tile> make_tiles();
std::map<std::string, TileIds> make_tile_id_map();

class WFC {
public:
    int w, h;
    std::pair<int,int> start, goal;

    WFC(int width, int height, const std::vector<WFC_Tile>& tiles,
                    std::pair<int,int> start, std::pair<int,int> goal, unsigned seed);

    bool solve();
    std::string collapsed_tile(int x, int y) const;

private:
    std::vector<WFC_Tile> tiles;
    std::map<std::string, int> tile_index;
    std::vector<std::vector<std::set<int>>> grid;
    std::mt19937 rng;

    bool in_bounds(int x, int y) const;
    bool has_conn(int tidx, Dir d) const;
    bool boundary_ok(int tidx, int x, int y) const;
    bool edge_compatible(int a, int b, Dir d) const;
    void filter_by_conn_count(int x, int y, int min_conns);
    bool reduce(int x, int y);
    bool propagate(const std::vector<std::pair<int,int>>& initial);
    std::set<Dir> optimistic_edges(int x, int y) const;
    bool connectivity_possible() const;
    bool final_path_exists() const;
    std::vector<int> weighted_shuffle(const std::set<int>& candidates);
    bool solve_recursive();
};

MapResult generate_map(int width, int height, unsigned seed,
                       int max_retries = 100, int min_empty = 5);
void export_txt(const std::string& path, int width = 21, int height = 12,
                unsigned seed = 42);
