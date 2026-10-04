#pragma once

#include <vector>

#include "core/game.hpp"

// Заменяет сгенерированный этаж открытой комнатой, игрок в (5,5).
inline World& prepareOpenFloor(Game& g, int w = 20, int h = 10) {
    World& world = g.world();
    std::vector<entt::entity> old;
    for (auto e : world.reg.view<Position>()) {
        if (e != world.player) old.push_back(e);
    }
    world.reg.destroy(old.begin(), old.end());
    world.map = Map(w, h);
    for (int y = 1; y < h - 1; ++y) {
        for (int x = 1; x < w - 1; ++x) world.map.setTile(x, y, Tile::Floor);
    }
    world.reg.get<Position>(world.player) = Position{5, 5};
    return world;
}
