#pragma once

enum class CommandType {
    None,
    Move,
    Pickup,
    Descend,
    OpenInventory,
    CloseInventory,
    UseItem,
    Wait,
    Restart,
    Quit
};

struct Command {
    CommandType type = CommandType::None;
    int dx = 0;
    int dy = 0;
    int index = 0;  // для UseItem: 0-based индекс в рюкзаке
};
