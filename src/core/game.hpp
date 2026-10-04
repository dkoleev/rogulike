#pragma once

#include <cstdint>
#include <memory>

#include "core/command.hpp"
#include "core/messages.hpp"
#include "core/world.hpp"

enum class GameState { Playing, Inventory, Dead, Won };

class Game {
public:
    explicit Game(std::uint32_t seed);
    Game(const Game&) = delete;
    Game& operator=(const Game&) = delete;

    // true, если мир сделал ход. Quit и Restart обрабатывает вызывающий код.
    bool tick(const Command& cmd);

    GameState state() const { return state_; }
    World& world() { return *world_; }
    const World& world() const { return *world_; }
    const MessageLog& log() const { return log_; }
    std::uint32_t seed() const { return seed_; }

private:
    void startFloor(int floor);
    bool endTurn();

    std::uint32_t seed_;
    std::unique_ptr<World> world_;
    MessageLog log_;
    GameState state_ = GameState::Playing;
};
