#pragma once

#include <string>

#include "core/game.hpp"
#include "ui/terminal.hpp"

class Renderer {
public:
    static constexpr int kCols = 80;
    static constexpr int kRows = 29;  // HUD 1 + карта/панель 24 + сообщения 4

    // Чистая функция (удобна для тестов): строки разделены "\r\n", без хвостового.
    std::string renderFrame(const Game& game) const;
    void draw(Terminal& terminal, const Game& game) const;
};
