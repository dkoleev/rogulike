#include <catch2/catch_test_macros.hpp>

#include "core/map.hpp"
#include "core/rng.hpp"

TEST_CASE("Map starts as solid wall", "[map]") {
    Map m(10, 5);
    REQUIRE(m.width() == 10);
    REQUIRE(m.height() == 5);
    REQUIRE(m.tile(3, 3) == Tile::Wall);
    REQUIRE_FALSE(m.walkable(3, 3));
    REQUIRE(m.opaque(3, 3));
}

TEST_CASE("Map out of bounds is safe", "[map]") {
    Map m(10, 5);
    REQUIRE_FALSE(m.inBounds(-1, 0));
    REQUIRE_FALSE(m.inBounds(10, 0));
    REQUIRE_FALSE(m.inBounds(0, 5));
    REQUIRE(m.tile(-1, -1) == Tile::Wall);
    REQUIRE_FALSE(m.walkable(99, 99));
    REQUIRE(m.opaque(99, 99));
    m.setTile(99, 99, Tile::Floor);
    m.setVisible(99, 99, true);
    REQUIRE_FALSE(m.visible(99, 99));
    REQUIRE_FALSE(m.explored(99, 99));
}

TEST_CASE("Floor tiles are walkable and transparent", "[map]") {
    Map m(10, 5);
    m.setTile(2, 2, Tile::Floor);
    REQUIRE(m.walkable(2, 2));
    REQUIRE_FALSE(m.opaque(2, 2));
}

TEST_CASE("Visible cells become explored and stay explored", "[map]") {
    Map m(10, 5);
    m.setVisible(2, 2, true);
    REQUIRE(m.visible(2, 2));
    REQUIRE(m.explored(2, 2));
    m.clearVisible();
    REQUIRE_FALSE(m.visible(2, 2));
    REQUIRE(m.explored(2, 2));
}

TEST_CASE("Rng is deterministic per seed", "[rng]") {
    Rng a(42), b(42);
    for (int i = 0; i < 20; ++i) REQUIRE(a.range(0, 1000) == b.range(0, 1000));
}

TEST_CASE("Rng range is inclusive and bounded", "[rng]") {
    Rng r(1);
    bool seen[3] = {false, false, false};
    for (int i = 0; i < 1000; ++i) {
        const int v = r.range(3, 5);
        REQUIRE(v >= 3);
        REQUIRE(v <= 5);
        seen[v - 3] = true;
    }
    REQUIRE(seen[0]);
    REQUIRE(seen[1]);
    REQUIRE(seen[2]);
}

TEST_CASE("Rng range with hi <= lo returns lo", "[rng]") {
    Rng r(1);
    REQUIRE(r.range(7, 7) == 7);
    REQUIRE(r.range(7, 3) == 7);
}

TEST_CASE("Rng chance extremes", "[rng]") {
    Rng r(1);
    for (int i = 0; i < 200; ++i) {
        REQUIRE_FALSE(r.chance(0));
        REQUIRE(r.chance(100));
    }
}
