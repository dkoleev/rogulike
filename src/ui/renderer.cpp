#include "ui/renderer.hpp"

#include <algorithm>
#include <vector>

#include "core/components.hpp"
#include "core/strings.hpp"
#include "core/systems/systems.hpp"

namespace {

constexpr int kMapRows = kMapHeight;  // 24
constexpr int kMessageRows = 4;
constexpr const char* kReset = "\x1b[0m";
constexpr const char* kClearEol = "\x1b[K";

const char* ansi(Color c) {
    switch (c) {
        case Color::Gray: return "\x1b[90m";
        case Color::Red: return "\x1b[31m";
        case Color::Green: return "\x1b[32m";
        case Color::Yellow: return "\x1b[33m";
        case Color::Blue: return "\x1b[34m";
        case Color::Magenta: return "\x1b[35m";
        case Color::Cyan: return "\x1b[36m";
        case Color::White: break;
    }
    return "\x1b[37m";
}

struct Glyph {
    char ch = ' ';
    Color color = Color::White;
};

std::string hudLine(const World& w) {
    const auto& reg = w.reg;
    const auto& hp = reg.get<Health>(w.player);
    const auto& lvl = reg.get<Level>(w.player);
    return "HP " + std::to_string(hp.cur) + "/" + std::to_string(hp.max) +
           "  Lv " + std::to_string(lvl.lvl) +
           "  XP " + std::to_string(lvl.xp) + "/" + std::to_string(xpToNext(lvl.lvl)) +
           "  Floor " + std::to_string(w.floor) +
           "  Gold " + std::to_string(reg.get<Gold>(w.player).amount) +
           "  Atk " + std::to_string(effectiveAttack(reg, w.player)) +
           "  Def " + std::to_string(effectiveDefense(reg, w.player));
}

std::vector<std::string> mapLines(const World& w) {
    const Map& m = w.map;
    const auto cells = static_cast<std::size_t>(m.width() * m.height());
    std::vector<Glyph> top(cells);
    std::vector<int> layer(cells, -1);
    for (auto e : w.reg.view<const Position, const Renderable>()) {
        const auto& p = w.reg.get<Position>(e);
        if (!m.inBounds(p.x, p.y) || !m.visible(p.x, p.y)) continue;
        const auto& r = w.reg.get<Renderable>(e);
        const auto i = static_cast<std::size_t>(p.y * m.width() + p.x);
        if (r.layer > layer[i]) {
            layer[i] = r.layer;
            top[i] = Glyph{r.glyph, r.color};
        }
    }

    std::vector<std::string> lines;
    for (int y = 0; y < m.height(); ++y) {
        std::string line;
        bool haveColor = false;
        Color current = Color::White;
        for (int x = 0; x < m.width(); ++x) {
            const auto i = static_cast<std::size_t>(y * m.width() + x);
            Glyph g;
            if (m.visible(x, y)) {
                g = layer[i] >= 0 ? top[i]
                                  : Glyph{m.walkable(x, y) ? '.' : '#', Color::White};
            } else if (m.explored(x, y)) {
                g = Glyph{m.walkable(x, y) ? '.' : '#', Color::Gray};
            }
            if (!haveColor || g.color != current) {
                line += ansi(g.color);
                current = g.color;
                haveColor = true;
            }
            line += g.ch;
        }
        line += kReset;
        lines.push_back(std::move(line));
    }
    return lines;
}

std::vector<std::string> inventoryLines(const World& w) {
    const auto& reg = w.reg;
    std::vector<std::string> lines;
    lines.push_back(str::kInventoryTitle);
    const auto& eq = reg.get<Equipment>(w.player);
    lines.push_back(std::string(str::kWeaponLabel) +
                    (eq.weapon != entt::null ? reg.get<Name>(eq.weapon).value : std::string(str::kNone)));
    lines.push_back(std::string(str::kArmorLabel) +
                    (eq.armor != entt::null ? reg.get<Name>(eq.armor).value : std::string(str::kNone)));
    lines.push_back("");
    const auto& inv = reg.get<Inventory>(w.player);
    for (std::size_t i = 0; i < inv.items.size(); ++i) {
        lines.push_back(std::to_string(i + 1) + ") " + reg.get<Name>(inv.items[i]).value);
    }
    lines.push_back("");
    lines.push_back(str::kInventoryHint);
    return lines;
}

void fit(std::vector<std::string>& lines, std::size_t count) {
    lines.resize(count);  // обрезает лишнее, добавляет пустые строки
}

std::string clip(const std::string& s) {
    return s.size() > static_cast<std::size_t>(Renderer::kCols)
               ? s.substr(0, static_cast<std::size_t>(Renderer::kCols))
               : s;
}

}  // namespace

std::string Renderer::renderFrame(const Game& game) const {
    const World& w = game.world();
    std::vector<std::string> lines;
    lines.push_back(hudLine(w));

    auto body = game.state() == GameState::Inventory ? inventoryLines(w) : mapLines(w);
    fit(body, kMapRows);
    lines.insert(lines.end(), body.begin(), body.end());

    const bool over = game.state() == GameState::Dead || game.state() == GameState::Won;
    auto messages = game.log().recent(over ? kMessageRows - 1 : kMessageRows);
    for (auto& m : messages) m = clip(m);
    if (over) {
        fit(messages, kMessageRows - 1);
        const std::string banner =
            game.state() == GameState::Dead
                ? std::string(str::kDiedBanner) + std::to_string(w.floor)
                : std::string(str::kWonBanner) + std::to_string(w.reg.get<Gold>(w.player).amount);
        messages.push_back(banner + str::kPressRestart);
    }
    fit(messages, kMessageRows);
    lines.insert(lines.end(), messages.begin(), messages.end());

    std::string out;
    for (std::size_t i = 0; i < lines.size(); ++i) {
        out += lines[i];
        out += kClearEol;
        if (i + 1 < lines.size()) out += "\r\n";
    }
    return out;
}

void Renderer::draw(Terminal& terminal, const Game& game) const {
    terminal.write("\x1b[H" + renderFrame(game));
}
