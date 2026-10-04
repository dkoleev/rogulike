#include "ui/terminal.hpp"

#include <cstdio>

void Terminal::write(const std::string& text) {
    std::fwrite(text.data(), 1, text.size(), stdout);
    std::fflush(stdout);
}

// альтернативный экран, скрыть курсор, очистить
void Terminal::enterScreen() { write("\x1b[?1049h\x1b[?25l\x1b[2J"); }

// сброс цвета, показать курсор, вернуть основной экран
void Terminal::leaveScreen() { write("\x1b[0m\x1b[?25h\x1b[?1049l"); }
