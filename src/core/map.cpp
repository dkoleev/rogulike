#include "core/map.hpp"

#include <algorithm>

Map::Map(int width, int height)
    : width_(width),
      height_(height),
      tiles_(static_cast<std::size_t>(width * height), Tile::Wall),
      visible_(static_cast<std::size_t>(width * height), 0),
      explored_(static_cast<std::size_t>(width * height), 0) {}

bool Map::inBounds(int x, int y) const {
    return x >= 0 && y >= 0 && x < width_ && y < height_;
}

Tile Map::tile(int x, int y) const {
    return inBounds(x, y) ? tiles_[static_cast<std::size_t>(index(x, y))] : Tile::Wall;
}

void Map::setTile(int x, int y, Tile t) {
    if (inBounds(x, y)) tiles_[static_cast<std::size_t>(index(x, y))] = t;
}

bool Map::walkable(int x, int y) const { return tile(x, y) == Tile::Floor; }

bool Map::opaque(int x, int y) const { return tile(x, y) == Tile::Wall; }

bool Map::visible(int x, int y) const {
    return inBounds(x, y) && visible_[static_cast<std::size_t>(index(x, y))] != 0;
}

bool Map::explored(int x, int y) const {
    return inBounds(x, y) && explored_[static_cast<std::size_t>(index(x, y))] != 0;
}

void Map::setVisible(int x, int y, bool v) {
    if (!inBounds(x, y)) return;
    const auto i = static_cast<std::size_t>(index(x, y));
    visible_[i] = v ? 1 : 0;
    if (v) explored_[i] = 1;
}

void Map::clearVisible() { std::fill(visible_.begin(), visible_.end(), 0); }
