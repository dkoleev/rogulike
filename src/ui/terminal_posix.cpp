#ifndef _WIN32

#include <poll.h>
#include <sys/ioctl.h>
#include <termios.h>
#include <unistd.h>

#include "ui/terminal.hpp"

namespace {

class PosixTerminal final : public Terminal {
public:
    PosixTerminal() {
        tcgetattr(STDIN_FILENO, &saved_);
        termios raw = saved_;
        raw.c_lflag &= ~static_cast<tcflag_t>(ICANON | ECHO);
        raw.c_cc[VMIN] = 1;
        raw.c_cc[VTIME] = 0;
        tcsetattr(STDIN_FILENO, TCSAFLUSH, &raw);
        enterScreen();
    }

    ~PosixTerminal() override {
        leaveScreen();
        tcsetattr(STDIN_FILENO, TCSAFLUSH, &saved_);
    }

    KeyEvent readKey() override {
        char c = 0;
        if (read(STDIN_FILENO, &c, 1) != 1) return KeyEvent{Key::Char, 'q'};  // EOF -> выход
        if (c != 27) return KeyEvent{Key::Char, c};
        if (!byteReady(50)) return KeyEvent{Key::Escape, 0};
        char seq[2] = {0, 0};
        if (read(STDIN_FILENO, &seq[0], 1) != 1) return KeyEvent{Key::Escape, 0};
        if (seq[0] != '[') return KeyEvent{};
        if (read(STDIN_FILENO, &seq[1], 1) != 1) return KeyEvent{};
        switch (seq[1]) {
            case 'A': return KeyEvent{Key::Up, 0};
            case 'B': return KeyEvent{Key::Down, 0};
            case 'C': return KeyEvent{Key::Right, 0};
            case 'D': return KeyEvent{Key::Left, 0};
            default: return KeyEvent{};
        }
    }

    TerminalSize size() const override {
        winsize ws{};
        if (ioctl(STDOUT_FILENO, TIOCGWINSZ, &ws) != 0 || ws.ws_col == 0) return TerminalSize{80, 24};
        return TerminalSize{ws.ws_col, ws.ws_row};
    }

private:
    static bool byteReady(int timeoutMs) {
        pollfd p{STDIN_FILENO, POLLIN, 0};
        return poll(&p, 1, timeoutMs) > 0;
    }

    termios saved_{};
};

}  // namespace

std::unique_ptr<Terminal> makeTerminal() { return std::make_unique<PosixTerminal>(); }

#endif  // !_WIN32
