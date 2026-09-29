#include "wfc.hpp"
#include "tiles.hpp"
#include "components.hpp"

#include <iostream>
#include <fstream>

// Tile definitions
std::vector<WFC_Tile> make_tiles() {
    auto t = [](const std::string& name, const std::string& dirs, double w) -> WFC_Tile {
        std::set<Dir> c;
        for (char ch : dirs) {
            if (ch == 'U') c.insert(UP);
            else if (ch == 'D') c.insert(DOWN);
            else if (ch == 'L') c.insert(LEFT);
            else if (ch == 'R') c.insert(RIGHT);
        }
        return { name, c, w };
    };
    return {
        t("empty",   "",     3.0),
        t("h",       "LR",   2.5),
        t("v",       "UD",   2.0),
        t("ur",      "UR",   1.5),
        t("ul",      "UL",   1.5),
        t("dr",      "DR",   1.5),
        t("dl",      "DL",   1.5),
        t("t_up",    "ULRU", 0.8),
        t("t_down",  "DLRD", 0.8),
        t("t_left",  "UDLU", 0.8),
        t("t_right", "UDRU", 0.8),
        t("end_u",   "U",    0.5),
        t("end_d",   "D",    0.5),
        t("end_l",   "L",    0.5),
        t("end_r",   "R",    0.5),
    };
}

// TILE_ID_MAP
std::map<std::string, TileIds> make_tile_id_map() {
    return {
        {"empty",   {0,  0,  0}},
        {"dr",      {static_cast<int>(TEXTURE_ASSET_ID::MAPTILE_121), static_cast<int>(TEXTURE_ASSET_ID::MAPTILE_155), static_cast<int>(TEXTURE_ASSET_ID::MAPTILE_255)}},
        {"dl",      {static_cast<int>(TEXTURE_ASSET_ID::MAPTILE_122), static_cast<int>(TEXTURE_ASSET_ID::MAPTILE_156), static_cast<int>(TEXTURE_ASSET_ID::MAPTILE_256)}},
        {"ur",      {static_cast<int>(TEXTURE_ASSET_ID::MAPTILE_123), static_cast<int>(TEXTURE_ASSET_ID::MAPTILE_172), static_cast<int>(TEXTURE_ASSET_ID::MAPTILE_272)}},
        {"ul",      {static_cast<int>(TEXTURE_ASSET_ID::MAPTILE_124), static_cast<int>(TEXTURE_ASSET_ID::MAPTILE_173), static_cast<int>(TEXTURE_ASSET_ID::MAPTILE_273)}},
        {"t_down",  {static_cast<int>(TEXTURE_ASSET_ID::MAPTILE_125), static_cast<int>(TEXTURE_ASSET_ID::MAPTILE_159), static_cast<int>(TEXTURE_ASSET_ID::MAPTILE_259)}},
        {"v",       {static_cast<int>(TEXTURE_ASSET_ID::MAPTILE_126), static_cast<int>(TEXTURE_ASSET_ID::MAPTILE_160), static_cast<int>(TEXTURE_ASSET_ID::MAPTILE_260)}},
        {"h",       {static_cast<int>(TEXTURE_ASSET_ID::MAPTILE_127), static_cast<int>(TEXTURE_ASSET_ID::MAPTILE_161), static_cast<int>(TEXTURE_ASSET_ID::MAPTILE_261)}},
        {"end_d",   {static_cast<int>(TEXTURE_ASSET_ID::MAPTILE_128), static_cast<int>(TEXTURE_ASSET_ID::MAPTILE_163), static_cast<int>(TEXTURE_ASSET_ID::MAPTILE_263)}},
        {"end_l",   {static_cast<int>(TEXTURE_ASSET_ID::MAPTILE_129), static_cast<int>(TEXTURE_ASSET_ID::MAPTILE_164), static_cast<int>(TEXTURE_ASSET_ID::MAPTILE_264)}},
        {"t_up",    {static_cast<int>(TEXTURE_ASSET_ID::MAPTILE_130), static_cast<int>(TEXTURE_ASSET_ID::MAPTILE_176), static_cast<int>(TEXTURE_ASSET_ID::MAPTILE_276)}},
        {"t_right", {static_cast<int>(TEXTURE_ASSET_ID::MAPTILE_142), static_cast<int>(TEXTURE_ASSET_ID::MAPTILE_177), static_cast<int>(TEXTURE_ASSET_ID::MAPTILE_277)}},
        {"t_left",  {static_cast<int>(TEXTURE_ASSET_ID::MAPTILE_143), static_cast<int>(TEXTURE_ASSET_ID::MAPTILE_178), static_cast<int>(TEXTURE_ASSET_ID::MAPTILE_278)}},
        {"end_u",   {static_cast<int>(TEXTURE_ASSET_ID::MAPTILE_144), static_cast<int>(TEXTURE_ASSET_ID::MAPTILE_180), static_cast<int>(TEXTURE_ASSET_ID::MAPTILE_280)}},
        {"end_r",   {static_cast<int>(TEXTURE_ASSET_ID::MAPTILE_146), static_cast<int>(TEXTURE_ASSET_ID::MAPTILE_181), static_cast<int>(TEXTURE_ASSET_ID::MAPTILE_281)}},
    };
}

WFC::WFC(int width, int height, const std::vector<WFC_Tile>& tiles,
                                 std::pair<int,int> start, std::pair<int,int> goal, unsigned seed)
    : w(width), h(height), tiles(tiles), start(start), goal(goal), rng(seed)
{
    int n = (int)this->tiles.size();
    for (int i = 0; i < n; i++)
        tile_index[this->tiles[i].name] = i;

    std::set<int> all;
    for (int i = 0; i < n; i++) all.insert(i);

    grid.assign(h, std::vector<std::set<int>>(w, all));

    int empty_idx = tile_index["empty"];
    for (int x = 0; x < w; x++)
        grid[0][x] = { empty_idx };
}

bool WFC::solve() {
    std::vector<std::pair<int,int>> all_cells;
    for (int y = 0; y < h; y++)
        for (int x = 0; x < w; x++)
            all_cells.push_back({x, y});

    if (!propagate(all_cells)) return false;

    auto [sx, sy] = start;
    auto [gx, gy] = goal;

    filter_by_conn_count(sx, sy, 1);
    filter_by_conn_count(gx, gy, 1);

    if (!propagate({start, goal})) return false;
    if (!connectivity_possible()) return false;
    return solve_recursive();
}

std::string WFC::collapsed_tile(int x, int y) const {
    int idx = *grid[y][x].begin();
    return tiles[idx].name;
}

bool WFC::in_bounds(int x, int y) const {
    return x >= 0 && x < w && y >= 0 && y < h;
}

bool WFC::has_conn(int tidx, Dir d) const {
    return tiles[tidx].connections.count(d) > 0;
}

bool WFC::boundary_ok(int tidx, int x, int y) const {
    for (int d = 0; d < 4; d++) {
        int nx = x + DX[d], ny = y + DY[d];
        if (!in_bounds(nx, ny) && has_conn(tidx, (Dir)d))
            return false;
    }
    return true;
}

bool WFC::edge_compatible(int a, int b, Dir d) const {
    return has_conn(a, d) == has_conn(b, OPPOSITE[d]);
}

void WFC::filter_by_conn_count(int x, int y, int min_conns) {
    auto& cell = grid[y][x];
    std::set<int> filtered;
    for (int t : cell)
        if ((int)tiles[t].connections.size() >= min_conns)
            filtered.insert(t);
    cell = filtered;
}

bool WFC::reduce(int x, int y) {
    auto& current = grid[y][x];
    std::set<int> new_set;

    for (int t : current) {
        if (!boundary_ok(t, x, y)) continue;
        bool ok = true;
        for (int d = 0; d < 4 && ok; d++) {
            int nx = x + DX[d], ny = y + DY[d];
            if (!in_bounds(nx, ny)) continue;
            bool any_match = false;
            for (int nt : grid[ny][nx]) {
                if (edge_compatible(t, nt, (Dir)d)) {
                    any_match = true;
                    break;
                }
            }
            if (!any_match) ok = false;
        }
        if (ok) new_set.insert(t);
    }

    if (new_set.empty()) return false;
    current = new_set;
    return true;
}

bool WFC::propagate(const std::vector<std::pair<int,int>>& initial) {
    std::queue<std::pair<int,int>> q;
    std::set<std::pair<int,int>> inq;
    for (auto& p : initial) { q.push(p); inq.insert(p); }

    while (!q.empty()) {
        auto [x, y] = q.front(); q.pop();
        inq.erase({x, y});

        for (int d = 0; d < 4; d++) {
            int nx = x + DX[d], ny = y + DY[d];
            if (!in_bounds(nx, ny)) continue;

            size_t before = grid[ny][nx].size();
            if (!reduce(nx, ny)) return false;
            if (grid[ny][nx].size() < before && inq.find({nx, ny}) == inq.end()) {
                q.push({nx, ny});
                inq.insert({nx, ny});
            }
        }
    }
    return true;
}

std::set<Dir> WFC::optimistic_edges(int x, int y) const {
    std::set<Dir> result;
    for (int d = 0; d < 4; d++) {
        int nx = x + DX[d], ny = y + DY[d];
        if (!in_bounds(nx, ny)) continue;
        for (int t : grid[y][x]) {
            if (has_conn(t, (Dir)d)) { result.insert((Dir)d); break; }
        }
    }
    return result;
}

bool WFC::connectivity_possible() const {
    std::queue<std::pair<int,int>> q;
    std::set<std::pair<int,int>> seen;
    q.push(start); seen.insert(start);

    while (!q.empty()) {
        auto [x, y] = q.front(); q.pop();
        if (std::make_pair(x, y) == goal) return true;

        for (Dir d : optimistic_edges(x, y)) {
            int nx = x + DX[d], ny = y + DY[d];
            if (!in_bounds(nx, ny)) continue;
            if (optimistic_edges(nx, ny).count(OPPOSITE[d]) == 0) continue;
            if (seen.find({nx, ny}) == seen.end()) {
                seen.insert({nx, ny});
                q.push({nx, ny});
            }
        }
    }
    return false;
}

bool WFC::final_path_exists() const {
    auto [sx, sy] = start;
    auto [gx, gy] = goal;
    if (grid[sy][sx].size() != 1 || grid[gy][gx].size() != 1) return false;

    std::queue<std::pair<int,int>> q;
    std::set<std::pair<int,int>> seen;
    q.push(start); seen.insert(start);

    while (!q.empty()) {
        auto [x, y] = q.front(); q.pop();
        if (std::make_pair(x, y) == goal) return true;

        int t = *grid[y][x].begin();
        for (Dir d : tiles[t].connections) {
            int nx = x + DX[d], ny = y + DY[d];
            if (!in_bounds(nx, ny)) continue;
            int nt = *grid[ny][nx].begin();
            if (tiles[nt].connections.count(OPPOSITE[d]) == 0) continue;
            if (seen.find({nx, ny}) == seen.end()) {
                seen.insert({nx, ny});
                q.push({nx, ny});
            }
        }
    }
    return false;
}

std::vector<int> WFC::weighted_shuffle(const std::set<int>& candidates) {
    std::vector<std::pair<double, int>> scored;
    std::uniform_real_distribution<double> dist(0.0, 1.0);
    for (int c : candidates) {
        double r = dist(rng);
        double key = std::pow(r, 1.0 / tiles[c].weight);
        scored.push_back({key, c});
    }
    std::sort(scored.begin(), scored.end(), [](auto& a, auto& b) {
        return a.first > b.first;
    });
    std::vector<int> result;
    for (auto& [_, c] : scored) result.push_back(c);
    return result;
}

bool WFC::solve_recursive() {
    int best_entropy = INT_MAX;
    std::vector<std::pair<int,int>> candidates;
    for (int y = 0; y < h; y++) {
        for (int x = 0; x < w; x++) {
            int e = (int)grid[y][x].size();
            if (e <= 1) continue;
            if (e < best_entropy) {
                best_entropy = e;
                candidates.clear();
                candidates.push_back({x, y});
            } else if (e == best_entropy) {
                candidates.push_back({x, y});
            }
        }
    }

    if (candidates.empty()) return final_path_exists();

    std::uniform_int_distribution<int> pick(0, (int)candidates.size() - 1);
    auto [cx, cy] = candidates[pick(rng)];

    for (int t : weighted_shuffle(grid[cy][cx])) {
        auto backup = grid;
        grid[cy][cx] = { t };
        if (propagate({{cx, cy}}) && connectivity_possible()) {
            if (solve_recursive()) return true;
        }
        grid = backup;
    }
    return false;
}


MapResult generate_map(int width, int height, unsigned seed,
                       int max_retries, int min_empty) {
    auto tiles = make_tiles();
    std::pair<int,int> start = {0, height / 2};
    int gy = height / 2 + 1 < height ? height / 2 + 1 : height / 2;
    std::pair<int,int> goal = {width - 1, gy};

    for (int attempt = 0; attempt < max_retries; attempt++) {
        unsigned s = seed + attempt;
        WFC solver(width, height, tiles, start, goal, s);
        if (solver.solve()) {
            std::vector<std::vector<std::string>> names(height,
                std::vector<std::string>(width));
            int empty_count = 0;
            for (int y = 0; y < height; y++)
                for (int x = 0; x < width; x++) {
                    names[y][x] = solver.collapsed_tile(x, y);
                    if (names[y][x] == "empty") empty_count++;
                }
            if (empty_count >= min_empty)
                return { names, start, goal, s };
        }
    }
    throw std::runtime_error("Failed to generate map after retries");
}

// ── Export ──
void export_txt(const std::string& path, int width, int height, unsigned seed) {
    auto id_map = make_tile_id_map();
    auto result = generate_map(width, height, seed);

    std::ofstream f(path);
    if (!f.is_open()) {
        std::cerr << "Cannot open " << path << std::endl;
        return;
    }

    for (int y = 0; y < height; y++) {
        for (int x = 0; x < width; x++) {
            const auto& name = result.names[y][x];
            auto& ids = id_map[name];

            int tid;
            if (std::make_pair(x, y) == result.start)
                tid = ids.start;
            else if (std::make_pair(x, y) == result.goal)
                tid = ids.exit;
            else
                tid = ids.reg;

            if (tid != 0)
                f << "tile " << x << " " << y << " " << tid << "\n";
        }
    }
    f.close();

    int empty_count = 0;
    for (auto& row : result.names)
        for (auto& t : row)
            if (t == "empty") empty_count++;

    std::cout << "Exported " << width << "x" << height << " map to " << path << std::endl;
    std::cout << "Seed: " << result.used_seed
              << ", Start: (" << result.start.first << "," << result.start.second
              << "), Goal: (" << result.goal.first << "," << result.goal.second << ")" << std::endl;
    std::cout << "Empty tiles: " << empty_count << std::endl;
}

// int main(int argc, char* argv[]) {
//     unsigned seed = 42;
//     if (argc > 1) seed = std::stoul(argv[1]);

//     export_txt("map_output.txt", 21, 12, seed);
//     return 0;
// }
