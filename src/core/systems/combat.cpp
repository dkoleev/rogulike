#include <algorithm>

#include "core/events.hpp"
#include "core/systems/systems.hpp"

int effectiveAttack(const entt::registry& reg, entt::entity e) {
    int value = reg.get<Stats>(e).attack;
    if (const auto* eq = reg.try_get<Equipment>(e); eq && eq->weapon != entt::null) {
        value += reg.get<Equippable>(eq->weapon).atkBonus;
    }
    return value;
}

int effectiveDefense(const entt::registry& reg, entt::entity e) {
    int value = reg.get<Stats>(e).defense;
    if (const auto* eq = reg.try_get<Equipment>(e); eq && eq->armor != entt::null) {
        value += reg.get<Equippable>(eq->armor).defBonus;
    }
    return value;
}

void attack(World& w, entt::entity attacker, entt::entity target) {
    auto& reg = w.reg;
    const int base = std::max(1, effectiveAttack(reg, attacker) - effectiveDefense(reg, target));
    const int damage = base + w.rng.range(0, 1);
    reg.get<Health>(target).cur -= damage;
    w.events.trigger(
        Attacked{reg.get<Name>(attacker).value, reg.get<Name>(target).value, damage, attacker == w.player});
}
