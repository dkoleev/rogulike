#include <array>
#include <cstdlib>
#include <optional>
#include <queue>
#include <utility>
#include <vector>

#include "core/systems/systems.hpp"

namespace {

constexpr int kChaseRange = 12;
constexpr int kBossRange = 15;
constexpr std::array<std::pair<int, int>, 4> kDirs{{{1, 0}, {-1, 0}, {0, 1}, {0, -1}}};

// BFS по клеткам; блокируют только BlocksMovement-сущности (цель — исключение).
std::optional<std::pair<int, int>> stepToward(World& w, Position from, Position to, int maxDepth) {
    const Map& m = w.map;
    const int W = m.width();
    const int H = m.height();
    const auto size = static_cast<std::size_t>(W * H);
    std::vector<char> blocked(size, 0);
    for (auto e : w.reg.view<Position, BlocksMovement>()) {
        const auto& p = w.reg.get<Position>(e);
        if (m.inBounds(p.x, p.y)) blocked[static_cast<std::size_t>(p.y * W + p.x)] = 1;
    }
    const int start = from.y * W + from.x;
    const int goal = to.y * W + to.x;
    blocked[static_cast<std::size_t>(goal)] = 0;

    std::vector<int> parent(size, -1);
    std::vector<int> depth(size, 0);
    std::queue<int> q;
    parent[static_cast<std::size_t>(start)] = start;
    q.push(start);
    while (!q.empty()) {
        const int cur = q.front();
        q.pop();
        if (cur == goal) break;
        if (depth[static_cast<std::size_t>(cur)] >= maxDepth) continue;
        const int cx = cur % W;
        const int cy = cur / W;
        for (const auto& [dx, dy] : kDirs) {
            const int nx = cx + dx;
            const int ny = cy + dy;
            if (!m.walkable(nx, ny)) continue;
            const auto ni = static_cast<std::size_t>(ny * W + nx);
            if (blocked[ni] || parent[ni] != -1) continue;
            parent[ni] = cur;
            depth[ni] = depth[static_cast<std::size_t>(cur)] + 1;
            q.push(static_cast<int>(ni));
        }
    }
    if (parent[static_cast<std::size_t>(goal)] == -1) return std::nullopt;
    int cur = goal;
    while (parent[static_cast<std::size_t>(cur)] != start) cur = parent[static_cast<std::size_t>(cur)];
    return std::make_pair(cur % W - from.x, cur / W - from.y);
}

}  // namespace

void aiSystem(World& w) {
    auto& reg = w.reg;
    const Position target = reg.get<Position>(w.player);

    std::vector<entt::entity> actors;
    for (auto e : reg.view<AI, Position>()) actors.push_back(e);

    for (auto e : actors) {
        if (reg.get<Health>(w.player).cur <= 0) break;
        const AIKind kind = reg.get<AI>(e).kind;
        const Position pos = reg.get<Position>(e);
        const int dist = std::abs(pos.x - target.x) + std::abs(pos.y - target.y);

        bool aware = false;
        switch (kind) {
            case AIKind::Boss: aware = dist <= kBossRange; break;
            case AIKind::Chase: aware = w.map.visible(pos.x, pos.y) && dist <= kChaseRange; break;
            case AIKind::Wander: aware = dist == 1; break;
        }
        if (aware) {
            if (const auto step = stepToward(w, pos, target, kBossRange * 2)) {
                tryMove(w, e, step->first, step->second);
                continue;
            }
        }
        if (kind == AIKind::Wander && w.rng.chance(50)) {
            const auto& [dx, dy] = kDirs[static_cast<std::size_t>(w.rng.range(0, 3))];
            tryMove(w, e, dx, dy);
        }
    }
}
