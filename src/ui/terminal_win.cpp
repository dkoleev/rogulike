#ifdef _WIN32

#define NOMINMAX
#define WIN32_LEAN_AND_MEAN
#include <conio.h>
#include <windows.h>

#include "ui/terminal.hpp"

namespace {

class WinTerminal final : public Terminal {
public:
    WinTerminal() {
        out_ = GetStdHandle(STD_OUTPUT_HANDLE);
        GetConsoleMode(out_, &savedMode_);
        SetConsoleMode(out_, savedMode_ | ENABLE_VIRTUAL_TERMINAL_PROCESSING);
        in_ = GetStdHandle(STD_INPUT_HANDLE);
        GetConsoleMode(in_, &savedInMode_);
        SetConsoleMode(in_, savedInMode_ & ~static_cast<DWORD>(ENABLE_PROCESSED_INPUT));
        enterScreen();
    }

    ~WinTerminal() override {
        leaveScreen();
        SetConsoleMode(out_, savedMode_);
        SetConsoleMode(in_, savedInMode_);
    }

    KeyEvent readKey() override {
        const int c = _getch();
        if (c == 0 || c == 0xE0) {
            switch (_getch()) {
                case 72: return KeyEvent{Key::Up, 0};
                case 80: return KeyEvent{Key::Down, 0};
                case 75: return KeyEvent{Key::Left, 0};
                case 77: return KeyEvent{Key::Right, 0};
                default: return KeyEvent{};
            }
        }
        if (c == 3) return KeyEvent{Key::Char, 'q'};  // Ctrl-C (processed input выключен)
        if (c == 27) return KeyEvent{Key::Escape, 0};
        return KeyEvent{Key::Char, static_cast<char>(c)};
    }

    TerminalSize size() const override {
        CONSOLE_SCREEN_BUFFER_INFO info;
        if (!GetConsoleScreenBufferInfo(out_, &info)) return TerminalSize{80, 24};
        return TerminalSize{info.srWindow.Right - info.srWindow.Left + 1,
                            info.srWindow.Bottom - info.srWindow.Top + 1};
    }

private:
    HANDLE out_ = nullptr;
    DWORD savedMode_ = 0;
    HANDLE in_ = nullptr;
    DWORD savedInMode_ = 0;
};

}  // namespace

std::unique_ptr<Terminal> makeTerminal() { return std::make_unique<WinTerminal>(); }

#endif  // _WIN32
