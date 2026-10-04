#include <catch2/catch_test_macros.hpp>

#include "core/messages.hpp"
#include "core/systems/systems.hpp"
#include "core/templates.hpp"
#include "test_helpers.hpp"

TEST_CASE("tryMove walks into a free cell", "[movement]") {
    auto w = makeOpenWorld();
    REQUIRE(tryMove(*w, w->player, 1, 0) == MoveResult::Moved);
    REQUIRE(w->reg.get<Position>(w->player) == Position{6, 5});
}

TEST_CASE("tryMove is blocked by walls, map edge and zero delta", "[movement]") {
    auto w = makeOpenWorld();
    w->map.setTile(6, 5, Tile::Wall);
    REQUIRE(tryMove(*w, w->player, 1, 0) == MoveResult::Blocked);
    REQUIRE(w->reg.get<Position>(w->player) == Position{5, 5});

    w->reg.get<Position>(w->player) = Position{1, 1};
    REQUIRE(tryMove(*w, w->player, -1, 0) == MoveResult::Blocked);
    REQUIRE(tryMove(*w, w->player, 0, -1) == MoveResult::Blocked);
    REQUIRE(tryMove(*w, w->player, 0, 0) == MoveResult::Blocked);
    REQUIRE(w->reg.get<Position>(w->player) == Position{1, 1});
}

TEST_CASE("moving into a monster attacks it", "[movement][combat]") {
    auto w = makeOpenWorld();
    MessageLog log;
    log.attach(w->events);
    const auto g = spawn(w->reg, "goblin", Position{6, 5});
    REQUIRE(tryMove(*w, w->player, 1, 0) == MoveResult::Attacked);
    REQUIRE(w->reg.get<Position>(w->player) == Position{5, 5});
    const int hp = w->reg.get<Health>(g).cur;
    REQUIRE(hp < 8);
    REQUIRE(hp >= 4);  // атака 4 - защита 1 = 3, плюс 0..1
    REQUIRE(log.recent(1)[0].rfind("You hit the Goblin for ", 0) == 0);
}

TEST_CASE("monsters block each other without fighting", "[movement]") {
    auto w = makeOpenWorld();
    const auto a = spawn(w->reg, "goblin", Position{8, 5});
    const auto b = spawn(w->reg, "goblin", Position{9, 5});
    REQUIRE(tryMove(*w, a, 1, 0) == MoveResult::Blocked);
    REQUIRE(w->reg.get<Health>(b).cur == w->reg.get<Health>(b).max);
}

TEST_CASE("damage is never below 1", "[combat]") {
    auto w = makeOpenWorld();
    const auto g = spawn(w->reg, "goblin", Position{6, 5});
    w->reg.get<Stats>(g).defense = 100;
    attack(*w, w->player, g);
    const int lost = 8 - w->reg.get<Health>(g).cur;
    REQUIRE(lost >= 1);
    REQUIRE(lost <= 2);
}

TEST_CASE("equipment adds to effective stats", "[combat]") {
    auto w = makeOpenWorld();
    REQUIRE(effectiveAttack(w->reg, w->player) == 4);
    REQUIRE(effectiveDefense(w->reg, w->player) == 1);
    auto& eq = w->reg.get<Equipment>(w->player);
    eq.weapon = spawn(w->reg, "dagger", Position{0, 0});
    eq.armor = spawn(w->reg, "leather_armor", Position{0, 0});
    REQUIRE(effectiveAttack(w->reg, w->player) == 6);
    REQUIRE(effectiveDefense(w->reg, w->player) == 2);
}

TEST_CASE("deathSystem removes dead monsters and awards xp", "[death]") {
    auto w = makeOpenWorld();
    MessageLog log;
    log.attach(w->events);
    const auto g = spawn(w->reg, "goblin", Position{6, 5});
    w->reg.get<Health>(g).cur = 0;
    deathSystem(*w);
    REQUIRE_FALSE(w->reg.valid(g));
    REQUIRE(w->reg.get<Level>(w->player).xp == 10);
    REQUIRE(log.recent(10).front() == "The Goblin dies.");
}

TEST_CASE("killing the boss sets bossKilled and drops no loot", "[death]") {
    auto w = makeOpenWorld();
    const auto d = spawn(w->reg, "dragon", Position{6, 5});
    w->reg.get<Health>(d).cur = 0;
    deathSystem(*w);
    REQUIRE(w->bossKilled);
    REQUIRE(countWith<Item>(w->reg) == 0);
}

TEST_CASE("deathSystem keeps a dead player and announces it", "[death]") {
    auto w = makeOpenWorld();
    MessageLog log;
    log.attach(w->events);
    w->reg.get<Health>(w->player).cur = 0;
    deathSystem(*w);
    REQUIRE(w->reg.valid(w->player));
    REQUIRE(log.recent(1)[0] == "You die...");
}

TEST_CASE("monsters sometimes drop loot", "[death]") {
    auto w = makeOpenWorld();
    for (int i = 0; i < 200; ++i) {
        const auto r = spawn(w->reg, "rat", Position{6, 5});
        w->reg.get<Health>(r).cur = 0;
        deathSystem(*w);
    }
    const int items = countWith<Item>(w->reg);
    REQUIRE(items > 20);
    REQUIRE(items < 120);
}

TEST_CASE("progression levels up and carries over xp", "[progression]") {
    auto w = makeOpenWorld();
    w->reg.get<Level>(w->player).xp = 65;
    progressionSystem(*w);
    const auto& lvl = w->reg.get<Level>(w->player);
    REQUIRE(lvl.lvl == 3);
    REQUIRE(lvl.xp == 5);
    REQUIRE(w->reg.get<Health>(w->player).max == 40);
    REQUIRE(w->reg.get<Stats>(w->player).attack == 6);
}

TEST_CASE("progression does nothing below the threshold", "[progression]") {
    auto w = makeOpenWorld();
    w->reg.get<Level>(w->player).xp = 19;
    progressionSystem(*w);
    REQUIRE(w->reg.get<Level>(w->player).lvl == 1);
    REQUIRE(xpToNext(1) == 20);
    REQUIRE(xpToNext(2) == 40);
}
