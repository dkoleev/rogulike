#include <catch2/catch_test_macros.hpp>

#include "core/game.hpp"
#include "core/strings.hpp"
#include "core/templates.hpp"
#include "game_helpers.hpp"
#include "test_helpers.hpp"

namespace {
Command move(int dx, int dy) { return Command{CommandType::Move, dx, dy}; }
Command cmd(CommandType t) { return Command{t}; }
Command use(int index) { return Command{CommandType::UseItem, 0, 0, index}; }
}  // namespace

TEST_CASE("a new game starts on floor 1 with a living hero", "[game]") {
    Game g(11);
    REQUIRE(g.state() == GameState::Playing);
    REQUIRE(g.world().floor == 1);
    REQUIRE(g.world().reg.get<Health>(g.world().player).cur == 30);
    REQUIRE(g.log().recent(1)[0] == str::kEnterDungeon);
}

TEST_CASE("the same seed gives the same first floor", "[game]") {
    Game a(77), b(77);
    REQUIRE(a.world().reg.get<Position>(a.world().player) ==
            b.world().reg.get<Position>(b.world().player));
    for (int y = 0; y < kMapHeight; ++y) {
        for (int x = 0; x < kMapWidth; ++x) {
            REQUIRE(a.world().map.tile(x, y) == b.world().map.tile(x, y));
        }
    }
}

TEST_CASE("moving into a wall costs no turn", "[game]") {
    Game g(1);
    auto& w = prepareOpenFloor(g);
    const auto goblin = spawn(w.reg, "goblin", Position{12, 5});
    w.map.setTile(6, 5, Tile::Wall);
    REQUIRE_FALSE(g.tick(move(1, 0)));
    REQUIRE(w.reg.get<Position>(w.player) == Position{5, 5});
    REQUIRE(w.reg.get<Position>(goblin) == Position{12, 5});
}

TEST_CASE("a free move is a turn and monsters react", "[game]") {
    Game g(1);
    auto& w = prepareOpenFloor(g);
    const auto goblin = spawn(w.reg, "goblin", Position{12, 5});
    REQUIRE(g.tick(move(1, 0)));
    REQUIRE(w.reg.get<Position>(w.player) == Position{6, 5});
    REQUIRE(w.reg.get<Position>(goblin) == Position{11, 5});
}

TEST_CASE("waiting lets an adjacent monster hit", "[game]") {
    Game g(1);
    auto& w = prepareOpenFloor(g);
    spawn(w.reg, "goblin", Position{6, 5});
    REQUIRE(g.tick(cmd(CommandType::Wait)));
    REQUIRE(w.reg.get<Health>(w.player).cur < 30);
}

TEST_CASE("killing a monster removes it and awards xp", "[game]") {
    Game g(1);
    auto& w = prepareOpenFloor(g);
    const auto rat = spawn(w.reg, "rat", Position{6, 5});
    REQUIRE(g.tick(move(1, 0)));
    REQUIRE_FALSE(w.reg.valid(rat));
    REQUIRE(w.reg.get<Level>(w.player).xp == 4);
    const auto lines = g.log().recent(5);
    bool died = false;
    for (const auto& l : lines) died = died || l == "The Rat dies.";
    REQUIRE(died);
}

TEST_CASE("pickup is a turn only when something is taken", "[game]") {
    Game g(1);
    auto& w = prepareOpenFloor(g);
    REQUIRE_FALSE(g.tick(cmd(CommandType::Pickup)));
    spawn(w.reg, "dagger", Position{5, 5});
    REQUIRE(g.tick(cmd(CommandType::Pickup)));
    REQUIRE(w.reg.get<Inventory>(w.player).items.size() == 1);
}

TEST_CASE("descend off stairs does nothing", "[game][focus]") {
    Game g(1);
    auto& w = prepareOpenFloor(g);
    REQUIRE_FALSE(g.tick(cmd(CommandType::Descend)));
    REQUIRE(w.floor == 1);
    REQUIRE(g.log().recent(1)[0] == str::kNoStairs);
}

TEST_CASE("descend on stairs builds the next floor", "[game]") {
    Game g(1);
    auto& w = prepareOpenFloor(g);
    spawnStairs(w.reg, Position{5, 5});
    giveItem(w, "dagger");
    REQUIRE(g.tick(cmd(CommandType::Descend)));
    REQUIRE(w.floor == 2);
    REQUIRE(g.state() == GameState::Playing);
    REQUIRE(w.reg.get<Inventory>(w.player).items.size() == 1);
    REQUIRE(g.log().recent(1)[0] == "You descend to floor 2.");
}

TEST_CASE("inventory flow: open, use, close", "[game]") {
    Game g(1);
    auto& w = prepareOpenFloor(g);
    w.reg.get<Health>(w.player).cur = 10;
    giveItem(w, "health_potion");

    REQUIRE_FALSE(g.tick(cmd(CommandType::OpenInventory)));
    REQUIRE(g.state() == GameState::Inventory);
    REQUIRE_FALSE(g.tick(move(1, 0)));  // движение в меню игнорируется
    REQUIRE(w.reg.get<Position>(w.player) == Position{5, 5});

    REQUIRE(g.tick(use(0)));
    REQUIRE(g.state() == GameState::Playing);
    REQUIRE(w.reg.get<Health>(w.player).cur == 20);

    REQUIRE_FALSE(g.tick(cmd(CommandType::OpenInventory)));
    REQUIRE_FALSE(g.tick(cmd(CommandType::CloseInventory)));
    REQUIRE(g.state() == GameState::Playing);
}

TEST_CASE("bad inventory index keeps the menu open and costs no turn", "[game][focus]") {
    Game g(1);
    auto& w = prepareOpenFloor(g);
    spawn(w.reg, "goblin", Position{12, 5});
    giveItem(w, "dagger");
    g.tick(cmd(CommandType::OpenInventory));
    REQUIRE_FALSE(g.tick(use(8)));
    REQUIRE(g.state() == GameState::Inventory);
    REQUIRE_FALSE(g.tick(use(-1)));
    REQUIRE(g.state() == GameState::Inventory);
    REQUIRE(w.reg.get<Inventory>(w.player).items.size() == 1);
}

TEST_CASE("commands after death are ignored", "[game][focus]") {
    Game g(1);
    auto& w = prepareOpenFloor(g);
    const auto goblin = spawn(w.reg, "goblin", Position{6, 5});
    w.reg.get<Health>(w.player).cur = 1;
    REQUIRE(g.tick(cmd(CommandType::Wait)));
    REQUIRE(g.state() == GameState::Dead);
    REQUIRE(g.log().recent(1)[0] == str::kYouDie);

    const Position goblinPos = w.reg.get<Position>(goblin);
    const Position playerPos = w.reg.get<Position>(w.player);
    REQUIRE_FALSE(g.tick(move(0, 1)));
    REQUIRE_FALSE(g.tick(cmd(CommandType::Wait)));
    REQUIRE_FALSE(g.tick(cmd(CommandType::OpenInventory)));
    REQUIRE(g.state() == GameState::Dead);
    REQUIRE(w.reg.get<Position>(goblin) == goblinPos);
    REQUIRE(w.reg.get<Position>(w.player) == playerPos);
}

TEST_CASE("commands after victory are ignored", "[game][focus]") {
    Game g(1);
    auto& w = prepareOpenFloor(g);
    const auto dragon = spawn(w.reg, "dragon", Position{6, 5});
    w.reg.get<Health>(dragon).cur = 1;
    REQUIRE(g.tick(move(1, 0)));
    REQUIRE(g.state() == GameState::Won);
    REQUIRE(g.log().recent(1)[0] == str::kVictory);
    REQUIRE_FALSE(g.tick(move(1, 0)));
    REQUIRE_FALSE(g.tick(cmd(CommandType::Wait)));
    REQUIRE(w.reg.get<Position>(w.player) == Position{5, 5});
}

TEST_CASE("Quit and Restart are not handled by Game", "[game]") {
    Game g(1);
    REQUIRE_FALSE(g.tick(cmd(CommandType::Quit)));
    REQUIRE_FALSE(g.tick(cmd(CommandType::Restart)));
    REQUIRE(g.state() == GameState::Playing);
}
