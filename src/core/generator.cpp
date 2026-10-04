#include "core/generator.hpp"

#include <algorithm>
#include <optional>

#include "core/templates.hpp"

namespace {

bool intersects(const Room& a, const Room& b) {
    return a.x - 1 < b.x + b.w && a.x + a.w + 1 > b.x && a.y - 1 < b.y + b.h && a.y + a.h + 1 > b.y;
}

void carveRoom(Map& m, const Room& r) {
    for (int y = r.y; y < r.y + r.h; ++y) {
        for (int x = r.x; x < r.x + r.w; ++x) m.setTile(x, y, Tile::Floor);
    }
}

void carveCorridor(Map& m, Rng& rng, Position a, Position b) {
    const auto hline = [&](int x1, int x2, int y) {
        for (int x = std::min(x1, x2); x <= std::max(x1, x2); ++x) m.setTile(x, y, Tile::Floor);
    };
    const auto vline = [&](int y1, int y2, int x) {
        for (int y = std::min(y1, y2); y <= std::max(y1, y2); ++y) m.setTile(x, y, Tile::Floor);
    };
    if (rng.chance(50)) {
        hline(a.x, b.x, a.y);
        vline(a.y, b.y, b.x);
    } else {
        vline(a.y, b.y, a.x);
        hline(a.x, b.x, b.y);
    }
}

}  // namespace

Layout generateLayout(Rng& rng) {
    for (;;) {
        Layout layout{Map(kMapWidth, kMapHeight), {}};
        const int target = rng.range(6, 9);
        for (int attempt = 0; attempt < 200 && static_cast<int>(layout.rooms.size()) < target;
             ++attempt) {
            Room r{0, 0, rng.range(4, 10), rng.range(3, 6)};
            r.x = rng.range(1, kMapWidth - r.w - 2);
            r.y = rng.range(1, kMapHeight - r.h - 2);
            const bool overlaps = std::any_of(layout.rooms.begin(), layout.rooms.end(),
                                              [&](const Room& o) { return intersects(r, o); });
            if (overlaps) continue;
            carveRoom(layout.map, r);
            layout.rooms.push_back(r);
        }
        if (layout.rooms.size() < 2) continue;
        for (std::size_t i = 1; i < layout.rooms.size(); ++i) {
            carveCorridor(layout.map, rng, layout.rooms[i - 1].center(), layout.rooms[i].center());
        }
        return layout;
    }
}

void buildFloor(World& w, int floor) {
    auto& reg = w.reg;

    std::vector<entt::entity> old;
    for (auto e : reg.view<Position>()) {
        if (e != w.player) old.push_back(e);
    }
    reg.destroy(old.begin(), old.end());

    Layout layout = generateLayout(w.rng);
    w.map = std::move(layout.map);
    w.floor = floor;

    const Position start = layout.rooms.front().center();
    if (w.player == entt::null) {
        w.player = spawnPlayer(reg, start);
    } else {
        reg.get<Position>(w.player) = start;
    }

    std::vector<Position> used{start};
    const auto freeCell = [&](const Room& r) -> std::optional<Position> {
        for (int tries = 0; tries < 30; ++tries) {
            const Position p{w.rng.range(r.x, r.x + r.w - 1), w.rng.range(r.y, r.y + r.h - 1)};
            if (std::find(used.begin(), used.end(), p) != used.end()) continue;
            used.push_back(p);
            return p;
        }
        return std::nullopt;
    };

    const Room& last = layout.rooms.back();
    used.push_back(last.center());
    if (floor == kFinalFloor) {
        spawn(reg, "dragon", last.center());
    } else {
        spawnStairs(reg, last.center());
    }

    for (std::size_t i = 1; i < layout.rooms.size(); ++i) {
        const Room& r = layout.rooms[i];
        const int monsters = w.rng.range(1, 2 + floor / 3);
        for (int m = 0; m < monsters; ++m) {
            const auto p = freeCell(r);
            const auto* def = pickMonsterDef(w.rng, floor);
            if (p && def) spawn(reg, def->id, *p);
        }
        if (w.rng.chance(45)) {
            const auto p = freeCell(r);
            const auto* def = pickItemDef(w.rng, floor);
            if (p && def) spawn(reg, def->id, *p);
        }
        if (w.rng.chance(50)) {
            if (const auto p = freeCell(r)) spawnGold(reg, *p, w.rng.range(5, 15) * floor);
        }
    }
}
