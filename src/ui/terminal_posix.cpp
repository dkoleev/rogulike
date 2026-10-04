#ifndef _WIN32

#include <cerrno>
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
        raw.c_lflag &= ~static_cast<tcflag_t>(ICANON | ECHO | ISIG);
        raw.c_iflag &= ~static_cast<tcflag_t>(IXON);
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
        if (!readByte(c)) return KeyEvent{Key::Char, 'q'};  // EOF/ошибка -> выход
        if (c == 3) return KeyEvent{Key::Char, 'q'};        // Ctrl-C (ISIG выключен)
        if (c != 27) return KeyEvent{Key::Char, c};
        if (!byteReady(50)) return KeyEvent{Key::Escape, 0};
        char intro = 0;
        if (!readByte(intro)) return KeyEvent{Key::Escape, 0};
        if (intro != '[' && intro != 'O') return KeyEvent{};
        // Читаем параметры до финального байта 0x40-0x7E.
        bool hasParams = false;
        for (;;) {
            if (!byteReady(50)) return KeyEvent{};
            char b = 0;
            if (!readByte(b)) return KeyEvent{};
            const auto u = static_cast<unsigned char>(b);
            if (u >= 0x40 && u <= 0x7E) {
                if (hasParams) return KeyEvent{};
                switch (b) {
                    case 'A': return KeyEvent{Key::Up, 0};
                    case 'B': return KeyEvent{Key::Down, 0};
                    case 'C': return KeyEvent{Key::Right, 0};
                    case 'D': return KeyEvent{Key::Left, 0};
                    default: return KeyEvent{};
                }
            }
            hasParams = true;
        }
    }

    TerminalSize size() const override {
        winsize ws{};
        if (ioctl(STDOUT_FILENO, TIOCGWINSZ, &ws) != 0 || ws.ws_col == 0) return TerminalSize{80, 24};
        return TerminalSize{ws.ws_col, ws.ws_row};
    }

private:
    static bool readByte(char& c) {
        for (;;) {
            const ssize_t n = read(STDIN_FILENO, &c, 1);
            if (n == 1) return true;
            if (n < 0 && errno == EINTR) continue;
            return false;
        }
    }

    static bool byteReady(int timeoutMs) {
        pollfd p{STDIN_FILENO, POLLIN, 0};
        for (;;) {
            const int r = poll(&p, 1, timeoutMs);
            if (r < 0 && errno == EINTR) continue;
            return r > 0;
        }
    }

    termios saved_{};
};

}  // namespace

std::unique_ptr<Terminal> makeTerminal() { return std::make_unique<PosixTerminal>(); }

#endif  // !_WIN32
