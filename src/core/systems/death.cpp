#include <string>
#include <vector>

#include "core/events.hpp"
#include "core/strings.hpp"
#include "core/systems/systems.hpp"
#include "core/templates.hpp"

namespace {
constexpr int kLootChancePercent = 30;

void dropLoot(World& w, Position pos) {
    if (!w.rng.chance(kLootChancePercent)) return;
    if (const auto* def = pickItemDef(w.rng, w.floor)) spawn(w.reg, def->id, pos);
}
}  // namespace

void deathSystem(World& w) {
    auto& reg = w.reg;
    std::vector<entt::entity> dead;
    for (auto e : reg.view<Health>()) {
        if (reg.get<Health>(e).cur <= 0) dead.push_back(e);
    }
    for (auto e : dead) {
        if (e == w.player) {
            w.events.trigger(Died{"You", true});
            continue;  // игрока не удаляем: Game переводит состояние в Dead
        }
        const std::string name = reg.get<Name>(e).value;
        const Position pos = reg.get<Position>(e);
        const bool boss = reg.all_of<AI>(e) && reg.get<AI>(e).kind == AIKind::Boss;
        if (const auto* xp = reg.try_get<XpReward>(e)) reg.get<Level>(w.player).xp += xp->amount;
        w.events.trigger(Died{name, false});
        reg.destroy(e);
        if (boss) {
            w.bossKilled = true;
        } else {
            dropLoot(w, pos);
        }
    }
}
