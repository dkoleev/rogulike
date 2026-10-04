#include "ui/input.hpp"

#include <cctype>

namespace {
Command move(int dx, int dy) { return Command{CommandType::Move, dx, dy}; }
}  // namespace

Command commandFromKey(const KeyEvent& key, GameState state) {
    if (key.key == Key::None) return Command{};
    const char c = key.key == Key::Char
                       ? static_cast<char>(std::tolower(static_cast<unsigned char>(key.ch)))
                       : '\0';

    if (state == GameState::Dead || state == GameState::Won) {
        if (c == 'r') return Command{CommandType::Restart};
        if (c == 'q' || key.key == Key::Escape) return Command{CommandType::Quit};
        return Command{};
    }

    if (state == GameState::Inventory) {
        if (key.key == Key::Escape || c == 'i' || c == 'e') return Command{CommandType::CloseInventory};
        if (c == 'q') return Command{CommandType::Quit};
        if (c >= '1' && c <= '9') return Command{CommandType::UseItem, 0, 0, c - '1'};
        return Command{};
    }

    if (key.key == Key::Up || c == 'w') return move(0, -1);
    if (key.key == Key::Down || c == 's') return move(0, 1);
    if (key.key == Key::Left || c == 'a') return move(-1, 0);
    if (key.key == Key::Right || c == 'd') return move(1, 0);
    if (c == 'g') return Command{CommandType::Pickup};
    if (c == 'i' || c == 'e') return Command{CommandType::OpenInventory};
    if (c == '>') return Command{CommandType::Descend};
    if (c == 'z') return Command{CommandType::Wait};
    if (c == 'q') return Command{CommandType::Quit};
    return Command{};
}
