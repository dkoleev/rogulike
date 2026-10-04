#include <catch2/catch_test_macros.hpp>
#include <queue>
#include <set>
#include <utility>

#include "core/generator.hpp"
#include "core/systems/systems.hpp"
#include "test_helpers.hpp"

namespace {

// Достижимые walkable-клетки от start (4 направления).
std::set<std::pair<int, int>> flood(const Map& m, Position start) {
    std::set<std::pair<int, int>> seen;
    std::queue<std::pair<int, int>> q;
    seen.insert({start.x, start.y});
    q.push({start.x, start.y});
    while (!q.empty()) {
        const auto [x, y] = q.front();
        q.pop();
        for (const auto& [dx, dy] : {std::pair{1, 0}, std::pair{-1, 0}, std::pair{0, 1}, std::pair{0, -1}}) {
            const int nx = x + dx;
            const int ny = y + dy;
            if (!m.walkable(nx, ny) || seen.count({nx, ny})) continue;
            seen.insert({nx, ny});
            q.push({nx, ny});
        }
    }
    return seen;
}

}  // namespace

TEST_CASE("layouts are connected and in bounds for many seeds", "[generator]") {
    for (std::uint32_t seed = 1; seed <= 100; ++seed) {
        Rng rng(seed);
        const Layout layout = generateLayout(rng);
        REQUIRE(layout.rooms.size() >= 2);
        REQUIRE(layout.rooms.size() <= 9);
        const auto reach = flood(layout.map, layout.rooms.front().center());
        for (const auto& r : layout.rooms) {
            REQUIRE(r.x >= 1);
            REQUIRE(r.y >= 1);
            REQUIRE(r.x + r.w <= kMapWidth - 1);
            REQUIRE(r.y + r.h <= kMapHeight - 1);
            REQUIRE(reach.count({r.center().x, r.center().y}) == 1);
        }
    }
}

TEST_CASE("the same seed gives the same layout", "[generator]") {
    Rng a(5), b(5);
    const Layout la = generateLayout(a);
    const Layout lb = generateLayout(b);
    REQUIRE(la.rooms.size() == lb.rooms.size());
    for (int y = 0; y < kMapHeight; ++y) {
        for (int x = 0; x < kMapWidth; ++x) REQUIRE(la.map.tile(x, y) == lb.map.tile(x, y));
    }
}

TEST_CASE("buildFloor places player and stairs on floors 1-4", "[generator]") {
    for (int floor = 1; floor <= 4; ++floor) {
        World w(static_cast<std::uint32_t>(floor));
        buildFloor(w, floor);
        REQUIRE(w.floor == floor);
        REQUIRE(w.reg.valid(w.player));
        const Position start = w.reg.get<Position>(w.player);
        REQUIRE(w.map.walkable(start.x, start.y));
        REQUIRE(countWith<Stairs>(w.reg) == 1);
        const auto reach = flood(w.map, start);
        for (auto e : w.reg.view<const Position, const Stairs>()) {
            const auto& p = w.reg.get<Position>(e);
            REQUIRE(reach.count({p.x, p.y}) == 1);
        }
        REQUIRE(countWith<AI>(w.reg) >= 1);
    }
}

TEST_CASE("floor 5 has the boss and no stairs", "[generator]") {
    World w(9);
    buildFloor(w, kFinalFloor);
    REQUIRE(countWith<Stairs>(w.reg) == 0);
    int bosses = 0;
    for (auto e : w.reg.view<AI>()) {
        if (w.reg.get<AI>(e).kind == AIKind::Boss) {
            ++bosses;
            const auto& p = w.reg.get<Position>(e);
            REQUIRE(flood(w.map, w.reg.get<Position>(w.player)).count({p.x, p.y}) == 1);
        }
    }
    REQUIRE(bosses == 1);
}

TEST_CASE("everything stands on walkable cells and blockers never share a cell", "[generator]") {
    for (std::uint32_t seed = 1; seed <= 30; ++seed) {
        World w(seed);
        buildFloor(w, 3);
        std::set<std::pair<int, int>> blockers;
        for (auto e : w.reg.view<Position>()) {
            const auto& p = w.reg.get<Position>(e);
            REQUIRE(w.map.walkable(p.x, p.y));
            if (w.reg.all_of<BlocksMovement>(e)) {
                REQUIRE(blockers.insert({p.x, p.y}).second);
            }
        }
    }
}

TEST_CASE("buildFloor keeps the player and pack, drops the old floor", "[generator]") {
    World w(3);
    buildFloor(w, 1);
    const auto player = w.player;
    const auto dagger = giveItem(w, "dagger");
    std::vector<entt::entity> oldStuff;
    for (auto e : w.reg.view<Position>()) {
        if (e != player) oldStuff.push_back(e);
    }
    REQUIRE_FALSE(oldStuff.empty());

    buildFloor(w, 2);
    REQUIRE(w.player == player);
    REQUIRE(w.floor == 2);
    REQUIRE(w.reg.valid(dagger));
    REQUIRE_FALSE(w.reg.all_of<Position>(dagger));
    REQUIRE(w.reg.get<Inventory>(player).items.size() == 1);
    for (auto e : oldStuff) REQUIRE_FALSE(w.reg.valid(e));
}
