#include "core/messages.hpp"

#include <algorithm>

#include "core/strings.hpp"

MessageLog::MessageLog(std::size_t capacity) : capacity_(capacity) {}

void MessageLog::attach(entt::dispatcher& d) {
    d.sink<Attacked>().connect<&MessageLog::onAttacked>(*this);
    d.sink<Died>().connect<&MessageLog::onDied>(*this);
    d.sink<ItemPickedUp>().connect<&MessageLog::onItemPickedUp>(*this);
    d.sink<GoldPicked>().connect<&MessageLog::onGoldPicked>(*this);
    d.sink<Healed>().connect<&MessageLog::onHealed>(*this);
    d.sink<Equipped>().connect<&MessageLog::onEquipped>(*this);
    d.sink<LevelUp>().connect<&MessageLog::onLevelUp>(*this);
    d.sink<FloorChanged>().connect<&MessageLog::onFloorChanged>(*this);
    d.sink<Note>().connect<&MessageLog::onNote>(*this);
}

void MessageLog::add(std::string text) {
    lines_.push_back(std::move(text));
    while (lines_.size() > capacity_) lines_.pop_front();
}

std::vector<std::string> MessageLog::recent(std::size_t count) const {
    const auto n = static_cast<std::ptrdiff_t>(std::min(count, lines_.size()));
    return std::vector<std::string>(lines_.end() - n, lines_.end());
}

void MessageLog::onAttacked(const Attacked& e) {
    if (e.byPlayer) {
        add("You hit the " + e.target + " for " + std::to_string(e.damage) + ".");
    } else {
        add("The " + e.attacker + " hits you for " + std::to_string(e.damage) + ".");
    }
}

void MessageLog::onDied(const Died& e) {
    add(e.isPlayer ? std::string(str::kYouDie) : "The " + e.name + " dies.");
}

void MessageLog::onItemPickedUp(const ItemPickedUp& e) { add("You pick up the " + e.name + "."); }
void MessageLog::onGoldPicked(const GoldPicked& e) {
    add("You pick up " + std::to_string(e.amount) + " gold.");
}
void MessageLog::onHealed(const Healed& e) { add("You heal " + std::to_string(e.amount) + " HP."); }
void MessageLog::onEquipped(const Equipped& e) { add("You equip the " + e.name + "."); }
void MessageLog::onLevelUp(const LevelUp& e) {
    add("Welcome to level " + std::to_string(e.level) + "!");
}
void MessageLog::onFloorChanged(const FloorChanged& e) {
    add(e.floor <= 1 ? std::string(str::kEnterDungeon)
                     : "You descend to floor " + std::to_string(e.floor) + ".");
}
void MessageLog::onNote(const Note& e) { add(e.text); }
