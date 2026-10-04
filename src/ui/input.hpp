#pragma once

#include "core/command.hpp"
#include "core/game.hpp"
#include "ui/terminal.hpp"

// Чистая функция: клавиша + состояние игры -> команда.
Command commandFromKey(const KeyEvent& key, GameState state);
