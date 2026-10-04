#include <catch2/catch_test_macros.hpp>

#include "core/systems/systems.hpp"
#include "core/templates.hpp"
#include "test_helpers.hpp"

namespace {
void wallColumn(World& w, int x) {
    for (int y = 1; y < w.map.height() - 1; ++y) w.map.setTile(x, y, Tile::Wall);
}
}  // namespace

TEST_CASE("fov sees open cells in radius and marks the player cell", "[fov]") {
    auto w = makeOpenWorld();
    fovSystem(*w);
    REQUIRE(w->map.visible(5, 5));
    REQUIRE(w->map.visible(5, 8));
    REQUIRE(w->map.visible(9, 5));
}

TEST_CASE("fov respects radius", "[fov]") {
    auto w = makeOpenWorld();
    w->reg.get<Viewshed>(w->player).radius = 3;
    fovSystem(*w);
    REQUIRE(w->map.visible(7, 5));
    REQUIRE_FALSE(w->map.visible(10, 5));
}

TEST_CASE("walls are lit but block what is behind them", "[fov]") {
    auto w = makeOpenWorld();
    wallColumn(*w, 7);
    fovSystem(*w);
    REQUIRE(w->map.visible(7, 5));
    REQUIRE_FALSE(w->map.visible(9, 5));
}

TEST_CASE("explored cells stay explored after moving away", "[fov]") {
    auto w = makeOpenWorld();
    fovSystem(*w);
    REQUIRE(w->map.visible(9, 5));
    w->reg.get<Position>(w->player) = Position{1, 1};
    w->reg.get<Viewshed>(w->player).radius = 2;
    fovSystem(*w);
    REQUIRE_FALSE(w->map.visible(9, 5));
    REQUIRE(w->map.explored(9, 5));
}

TEST_CASE("an adjacent goblin attacks the player", "[ai]") {
    auto w = makeOpenWorld();
    spawn(w->reg, "goblin", Position{6, 5});
    fovSystem(*w);
    aiSystem(*w);
    const int hp = w->reg.get<Health>(w->player).cur;
    REQUIRE(hp < 30);
    REQUIRE(hp >= 27);  // 3 - 1 = 2, плюс 0..1
}

TEST_CASE("a goblin that sees the player steps closer", "[ai]") {
    auto w = makeOpenWorld();
    const auto g = spawn(w->reg, "goblin", Position{10, 5});
    fovSystem(*w);
    aiSystem(*w);
    REQUIRE(w->reg.get<Position>(g) == Position{9, 5});
}

TEST_CASE("a goblin behind a wall does not move", "[ai]") {
    auto w = makeOpenWorld();
    wallColumn(*w, 7);
    const auto g = spawn(w->reg, "goblin", Position{10, 5});
    fovSystem(*w);
    aiSystem(*w);
    REQUIRE(w->reg.get<Position>(g) == Position{10, 5});
}

TEST_CASE("the boss hunts without line of sight, but copes with no path", "[ai]") {
    {
        auto w = makeOpenWorld();
        const auto d = spawn(w->reg, "dragon", Position{10, 5});
        aiSystem(*w);  // fov не считали: видимости нет
        REQUIRE(w->reg.get<Position>(d) == Position{9, 5});
    }
    {
        auto w = makeOpenWorld();
        wallColumn(*w, 7);
        const auto d = spawn(w->reg, "dragon", Position{10, 5});
        aiSystem(*w);
        REQUIRE(w->reg.get<Position>(d) == Position{10, 5});
    }
}

TEST_CASE("wandering rats stay on walkable cells", "[ai]") {
    auto w = makeOpenWorld();
    w->reg.get<Health>(w->player) = Health{1000, 1000};
    const auto r = spawn(w->reg, "rat", Position{15, 5});
    for (int i = 0; i < 100; ++i) {
        fovSystem(*w);
        aiSystem(*w);
        const auto& p = w->reg.get<Position>(r);
        REQUIRE(w->map.walkable(p.x, p.y));
    }
}

TEST_CASE("monsters do nothing once the player is dead", "[ai]") {
    auto w = makeOpenWorld();
    const auto g = spawn(w->reg, "goblin", Position{6, 5});
    w->reg.get<Health>(w->player).cur = 0;
    fovSystem(*w);
    aiSystem(*w);
    REQUIRE(w->reg.get<Health>(w->player).cur == 0);
    REQUIRE(w->reg.get<Position>(g) == Position{6, 5});
}
