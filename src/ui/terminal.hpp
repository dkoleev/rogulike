#pragma once

#include <memory>
#include <string>

enum class Key { None, Up, Down, Left, Right, Escape, Char };

struct KeyEvent {
    Key key = Key::None;
    char ch = 0;
};

struct TerminalSize {
    int cols;
    int rows;
};

// Вывод — ANSI-последовательности (одинаково на Windows 10+ и POSIX).
// Платформенные подклассы реализуют ввод одной клавиши и размер окна
// и вызывают enterScreen()/leaveScreen() в конструкторе/деструкторе (RAII).
class Terminal {
public:
    virtual ~Terminal() = default;
    virtual KeyEvent readKey() = 0;  // блокирующее чтение
    virtual TerminalSize size() const = 0;
    void write(const std::string& text);

protected:
    void enterScreen();
    void leaveScreen();
};

std::unique_ptr<Terminal> makeTerminal();
