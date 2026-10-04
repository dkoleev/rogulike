#include <cstdint>
#include <cstdlib>
#include <iostream>
#include <memory>
#include <random>
#include <string>

#include "core/game.hpp"
#include "core/strings.hpp"
#include "ui/input.hpp"
#include "ui/renderer.hpp"
#include "ui/terminal.hpp"

int main(int argc, char** argv) {
    std::random_device rd;
    const std::uint32_t firstSeed =
        argc > 1 ? static_cast<std::uint32_t>(std::strtoul(argv[1], nullptr, 10)) : rd();
    auto game = std::make_unique<Game>(firstSeed);
    Renderer renderer;

    {
        const auto terminal = makeTerminal();  // деструктор вернёт консоль в исходный режим
        const auto size = terminal->size();
        if (size.cols < Renderer::kCols || size.rows < Renderer::kRows) {
            terminal->write(std::string("\x1b[H") + str::kTooSmall + " (need " +
                            std::to_string(Renderer::kCols) + "x" + std::to_string(Renderer::kRows) +
                            ", have " + std::to_string(size.cols) + "x" + std::to_string(size.rows) +
                            ")");
            terminal->readKey();
        }

        for (;;) {
            renderer.draw(*terminal, *game);
            const Command cmd = commandFromKey(terminal->readKey(), game->state());
            if (cmd.type == CommandType::Quit) break;
            if (cmd.type == CommandType::Restart) {
                game = std::make_unique<Game>(rd());
                continue;
            }
            game->tick(cmd);
        }
    }

    std::cout << "Seed: " << game->seed() << "\n";
    return 0;
}
