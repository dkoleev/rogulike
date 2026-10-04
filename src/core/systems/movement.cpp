#include "core/systems/systems.hpp"

MoveResult tryMove(World& w, entt::entity actor, int dx, int dy) {
    if (dx == 0 && dy == 0) return MoveResult::Blocked;
    auto& reg = w.reg;
    auto& pos = reg.get<Position>(actor);
    const int nx = pos.x + dx;
    const int ny = pos.y + dy;
    if (!w.map.walkable(nx, ny)) return MoveResult::Blocked;

    for (auto e : reg.view<Position, BlocksMovement>()) {
        if (e == actor) continue;
        const auto& p = reg.get<Position>(e);
        if (p.x != nx || p.y != ny) continue;
        const auto* mine = reg.try_get<Faction>(actor);
        const auto* theirs = reg.try_get<Faction>(e);
        if (mine && theirs && mine->side != theirs->side && reg.all_of<Health>(e)) {
            attack(w, actor, e);
            return MoveResult::Attacked;
        }
        return MoveResult::Blocked;
    }
    pos.x = nx;
    pos.y = ny;
    return MoveResult::Moved;
}
