#pragma once

#include <cstddef>
#include <deque>
#include <entt/entt.hpp>
#include <string>
#include <vector>

#include "core/events.hpp"

class MessageLog {
public:
    explicit MessageLog(std::size_t capacity = 100);
    MessageLog(const MessageLog&) = delete;
    MessageLog& operator=(const MessageLog&) = delete;

    void attach(entt::dispatcher& dispatcher);
    void add(std::string text);
    // последние count строк, от старых к новым
    std::vector<std::string> recent(std::size_t count) const;

private:
    void onAttacked(const Attacked& e);
    void onDied(const Died& e);
    void onItemPickedUp(const ItemPickedUp& e);
    void onGoldPicked(const GoldPicked& e);
    void onHealed(const Healed& e);
    void onEquipped(const Equipped& e);
    void onLevelUp(const LevelUp& e);
    void onFloorChanged(const FloorChanged& e);
    void onNote(const Note& e);

    std::deque<std::string> lines_;
    std::size_t capacity_;
};
