#include "core/game.hpp"

#include "core/events.hpp"
#include "core/generator.hpp"
#include "core/strings.hpp"
#include "core/systems/systems.hpp"

Game::Game(std::uint32_t seed) : seed_(seed), world_(std::make_unique<World>(seed)) {
    log_.attach(world_->events);
    startFloor(1);
}

void Game::startFloor(int floor) {
    World& w = *world_;
    buildFloor(w, floor);
    fovSystem(w);
    w.events.trigger(FloorChanged{floor});
}

bool Game::endTurn() {
    World& w = *world_;
    deathSystem(w);
    progressionSystem(w);
    if (w.bossKilled) {
        state_ = GameState::Won;
        w.events.trigger(Note{str::kVictory});
        return true;
    }
    fovSystem(w);
    aiSystem(w);
    deathSystem(w);
    if (w.reg.get<Health>(w.player).cur <= 0) state_ = GameState::Dead;
    return true;
}

bool Game::tick(const Command& cmd) {
    if (state_ == GameState::Dead || state_ == GameState::Won) return false;
    World& w = *world_;

    if (state_ == GameState::Inventory) {
        switch (cmd.type) {
            case CommandType::CloseInventory:
                state_ = GameState::Playing;
                return false;
            case CommandType::UseItem:
                if (!useItem(w, cmd.index)) return false;
                state_ = GameState::Playing;
                return endTurn();
            default:
                return false;
        }
    }

    switch (cmd.type) {
        case CommandType::Move:
            if (tryMove(w, w.player, cmd.dx, cmd.dy) == MoveResult::Blocked) return false;
            return endTurn();
        case CommandType::Wait:
            return endTurn();
        case CommandType::Pickup:
            if (!pickupItems(w)) return false;
            return endTurn();
        case CommandType::Descend:
            if (!isOnStairs(w)) {
                w.events.trigger(Note{str::kNoStairs});
                return false;
            }
            startFloor(w.floor + 1);
            return true;
        case CommandType::OpenInventory:
            state_ = GameState::Inventory;
            return false;
        default:
            return false;
    }
}
