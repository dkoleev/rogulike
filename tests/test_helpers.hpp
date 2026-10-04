#pragma once

#include <memory>
#include <string_view>

#include "core/templates.hpp"
#include "core/world.hpp"

// Открытая комната w×h с рамкой из стен, игрок в (5,5).
inline std::unique_ptr<World> makeOpenWorld(int w = 20, int h = 10) {
    auto world = std::make_unique<World>(1u);
    world->map = Map(w, h);
    for (int y = 1; y < h - 1; ++y) {
        for (int x = 1; x < w - 1; ++x) world->map.setTile(x, y, Tile::Floor);
    }
    world->player = spawnPlayer(world->reg, Position{5, 5});
    return world;
}

template <typename... C>
int countWith(const entt::registry& reg) {
    int n = 0;
    for ([[maybe_unused]] auto e : reg.view<const C...>()) ++n;
    return n;
}

// Кладёт предмет сразу в рюкзак игрока (без Position, с InInventory).
inline entt::entity giveItem(World& w, std::string_view id) {
    const auto e = spawn(w.reg, id, w.reg.get<Position>(w.player));
    w.reg.remove<Position>(e);
    w.reg.emplace<InInventory>(e, InInventory{w.player});
    w.reg.get<Inventory>(w.player).items.push_back(e);
    return e;
}
