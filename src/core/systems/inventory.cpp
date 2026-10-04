#include <algorithm>
#include <vector>

#include "core/events.hpp"
#include "core/strings.hpp"
#include "core/systems/systems.hpp"

bool pickupItems(World& w) {
    auto& reg = w.reg;
    const Position pp = reg.get<Position>(w.player);

    std::vector<entt::entity> here;
    for (auto e : reg.view<Position, Item>()) {
        const auto& p = reg.get<Position>(e);
        if (p.x == pp.x && p.y == pp.y) here.push_back(e);
    }
    if (here.empty()) {
        w.events.trigger(Note{str::kNothingHere});
        return false;
    }

    bool tookAny = false;
    for (auto e : here) {
        if (const auto* gold = reg.try_get<Gold>(e)) {
            const int amount = gold->amount;
            reg.get<Gold>(w.player).amount += amount;
            w.events.trigger(GoldPicked{amount});
            reg.destroy(e);
            tookAny = true;
            continue;
        }
        auto& inv = reg.get<Inventory>(w.player);
        if (inv.items.size() >= kMaxInventory) {
            w.events.trigger(Note{str::kPackFull});
            continue;
        }
        inv.items.push_back(e);
        reg.remove<Position>(e);  // инвариант: предмет в рюкзаке без Position
        reg.emplace<InInventory>(e, InInventory{w.player});
        w.events.trigger(ItemPickedUp{reg.get<Name>(e).value});
        tookAny = true;
    }
    return tookAny;
}

bool useItem(World& w, int index) {
    auto& reg = w.reg;
    auto& inv = reg.get<Inventory>(w.player);
    if (index < 0 || index >= static_cast<int>(inv.items.size())) {
        w.events.trigger(Note{str::kNoSuchItem});
        return false;
    }
    const entt::entity item = inv.items[static_cast<std::size_t>(index)];

    if (const auto* c = reg.try_get<Consumable>(item)) {
        auto& hp = reg.get<Health>(w.player);
        if (hp.cur >= hp.max) {
            w.events.trigger(Note{str::kFullHealth});
            return false;
        }
        const int healed = std::min(c->power, hp.max - hp.cur);
        hp.cur += healed;
        inv.items.erase(inv.items.begin() + index);
        reg.destroy(item);
        w.events.trigger(Healed{healed});
        return true;
    }

    if (const auto* eq = reg.try_get<Equippable>(item)) {
        auto& worn = reg.get<Equipment>(w.player);
        entt::entity& slot = (eq->slot == Slot::Weapon) ? worn.weapon : worn.armor;
        const entt::entity previous = slot;
        slot = item;
        inv.items.erase(inv.items.begin() + index);
        if (previous != entt::null) inv.items.push_back(previous);
        w.events.trigger(Equipped{reg.get<Name>(item).value});
        return true;
    }
    return false;
}

bool isOnStairs(const World& w) {
    const auto& pp = w.reg.get<Position>(w.player);
    for (auto e : w.reg.view<const Position, const Stairs>()) {
        const auto& p = w.reg.get<Position>(e);
        if (p.x == pp.x && p.y == pp.y) return true;
    }
    return false;
}
