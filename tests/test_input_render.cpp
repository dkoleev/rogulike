#include <algorithm>
#include <catch2/catch_test_macros.hpp>
#include <string>

#include "core/systems/systems.hpp"
#include "core/templates.hpp"
#include "game_helpers.hpp"
#include "test_helpers.hpp"
#include "ui/input.hpp"
#include "ui/renderer.hpp"

namespace {
KeyEvent ch(char c) { return KeyEvent{Key::Char, c}; }
KeyEvent special(Key k) { return KeyEvent{k, 0}; }
int newlines(const std::string& s) { return static_cast<int>(std::count(s.begin(), s.end(), '\n')); }
}  // namespace

TEST_CASE("playing: movement keys and arrows", "[input]") {
    const auto s = GameState::Playing;
    REQUIRE(commandFromKey(ch('w'), s).type == CommandType::Move);
    REQUIRE(commandFromKey(ch('w'), s).dy == -1);
    REQUIRE(commandFromKey(ch('S'), s).dy == 1);
    REQUIRE(commandFromKey(ch('a'), s).dx == -1);
    REQUIRE(commandFromKey(ch('d'), s).dx == 1);
    REQUIRE(commandFromKey(special(Key::Up), s).dy == -1);
    REQUIRE(commandFromKey(special(Key::Down), s).dy == 1);
    REQUIRE(commandFromKey(special(Key::Left), s).dx == -1);
    REQUIRE(commandFromKey(special(Key::Right), s).dx == 1);
}

TEST_CASE("playing: action keys", "[input]") {
    const auto s = GameState::Playing;
    REQUIRE(commandFromKey(ch('g'), s).type == CommandType::Pickup);
    REQUIRE(commandFromKey(ch('i'), s).type == CommandType::OpenInventory);
    REQUIRE(commandFromKey(ch('e'), s).type == CommandType::OpenInventory);
    REQUIRE(commandFromKey(ch('>'), s).type == CommandType::Descend);
    REQUIRE(commandFromKey(ch('z'), s).type == CommandType::Wait);
    REQUIRE(commandFromKey(ch('q'), s).type == CommandType::Quit);
}

TEST_CASE("unknown keys do nothing", "[input]") {
    REQUIRE(commandFromKey(ch('x'), GameState::Playing).type == CommandType::None);
    REQUIRE(commandFromKey(special(Key::None), GameState::Playing).type == CommandType::None);
    REQUIRE(commandFromKey(special(Key::Escape), GameState::Playing).type == CommandType::None);
}

TEST_CASE("inventory: digits use items, movement keys are ignored", "[input]") {
    const auto s = GameState::Inventory;
    const auto one = commandFromKey(ch('1'), s);
    REQUIRE(one.type == CommandType::UseItem);
    REQUIRE(one.index == 0);
    REQUIRE(commandFromKey(ch('9'), s).index == 8);
    REQUIRE(commandFromKey(ch('0'), s).type == CommandType::None);
    REQUIRE(commandFromKey(ch('a'), s).type == CommandType::None);
    REQUIRE(commandFromKey(special(Key::Escape), s).type == CommandType::CloseInventory);
    REQUIRE(commandFromKey(ch('i'), s).type == CommandType::CloseInventory);
    REQUIRE(commandFromKey(ch('q'), s).type == CommandType::Quit);
}

TEST_CASE("after the run ends R restarts and any other key quits", "[input]") {
    REQUIRE(commandFromKey(ch('r'), GameState::Dead).type == CommandType::Restart);
    REQUIRE(commandFromKey(ch('R'), GameState::Won).type == CommandType::Restart);
    REQUIRE(commandFromKey(ch('x'), GameState::Dead).type == CommandType::Quit);
    REQUIRE(commandFromKey(special(Key::Up), GameState::Won).type == CommandType::Quit);
    REQUIRE(commandFromKey(special(Key::None), GameState::Dead).type == CommandType::None);
}

TEST_CASE("frame has a fixed height and shows the HUD and hero", "[render]") {
    Game g(3);
    Renderer r;
    const auto frame = r.renderFrame(g);
    REQUIRE(newlines(frame) == Renderer::kRows - 1);
    REQUIRE(frame.find("HP 30/30") != std::string::npos);
    REQUIRE(frame.find("Floor 1") != std::string::npos);
    REQUIRE(frame.find('@') != std::string::npos);
}

TEST_CASE("fog of war hides monsters outside the view", "[render]") {
    Game g(3);
    auto& w = prepareOpenFloor(g);
    w.reg.get<Viewshed>(w.player).radius = 3;
    spawn(w.reg, "troll", Position{15, 5});  // 'T' нет ни в HUD, ни в сообщениях
    fovSystem(w);
    Renderer r;
    REQUIRE(r.renderFrame(g).find('T') == std::string::npos);

    w.reg.get<Viewshed>(w.player).radius = 12;
    fovSystem(w);
    REQUIRE(r.renderFrame(g).find('T') != std::string::npos);
}

TEST_CASE("inventory panel lists items and keeps frame height", "[render]") {
    Game g(3);
    auto& w = prepareOpenFloor(g);
    giveItem(w, "dagger");
    g.tick(Command{CommandType::OpenInventory});
    Renderer r;
    const auto frame = r.renderFrame(g);
    REQUIRE(newlines(frame) == Renderer::kRows - 1);
    REQUIRE(frame.find("1) Dagger") != std::string::npos);
    REQUIRE(frame.find("Inventory") != std::string::npos);
}

TEST_CASE("death banner is shown", "[render]") {
    Game g(3);
    auto& w = prepareOpenFloor(g);
    spawn(w.reg, "goblin", Position{6, 5});
    w.reg.get<Health>(w.player).cur = 1;
    g.tick(Command{CommandType::Wait});
    Renderer r;
    const auto frame = r.renderFrame(g);
    REQUIRE(newlines(frame) == Renderer::kRows - 1);
    REQUIRE(frame.find("You died on floor 1") != std::string::npos);
}
