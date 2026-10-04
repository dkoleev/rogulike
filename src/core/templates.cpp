#include "core/templates.hpp"

#include <cassert>
#include <string>

namespace {

// id, name, glyph, color, hp, atk, def, xp, ai, minFloor, maxFloor, weight
const MonsterDef kMonsters[] = {
    {"rat", "Rat", 'r', Color::Gray, 4, 2, 0, 4, AIKind::Wander, 1, 2, 10},
    {"goblin", "Goblin", 'g', Color::Green, 8, 3, 1, 10, AIKind::Chase, 1, 5, 8},
    {"orc", "Orc", 'o', Color::Yellow, 16, 5, 2, 25, AIKind::Chase, 3, 5, 6},
    {"troll", "Troll", 'T', Color::Red, 28, 7, 3, 50, AIKind::Chase, 4, 5, 4},
    {"dragon", "Dragon", 'D', Color::Magenta, 60, 9, 4, 200, AIKind::Boss, 5, 5, 0},
};

// id, name, glyph, color, kind, power, minFloor, weight
const ItemDef kItems[] = {
    {"health_potion", "Health Potion", '!', Color::Red, ItemKind::Potion, 10, 1, 10},
    {"greater_health_potion", "Greater Health Potion", '!', Color::Magenta, ItemKind::Potion, 25, 3, 4},
    {"dagger", "Dagger", '/', Color::Cyan, ItemKind::Weapon, 2, 1, 6},
    {"sword", "Sword", '/', Color::White, ItemKind::Weapon, 4, 3, 4},
    {"leather_armor", "Leather Armor", '[', Color::Yellow, ItemKind::Armor, 1, 1, 6},
    {"chain_mail", "Chain Mail", '[', Color::Cyan, ItemKind::Armor, 3, 3, 3},
};

template <typename Def, typename Pred>
const Def* pickWeighted(std::span<const Def> defs, Rng& rng, Pred eligible) {
    int total = 0;
    for (const auto& d : defs) {
        if (d.weight > 0 && eligible(d)) total += d.weight;
    }
    if (total == 0) return nullptr;
    int roll = rng.range(1, total);
    for (const auto& d : defs) {
        if (d.weight <= 0 || !eligible(d)) continue;
        roll -= d.weight;
        if (roll <= 0) return &d;
    }
    return nullptr;
}

}  // namespace

std::span<const MonsterDef> monsterDefs() { return std::span<const MonsterDef>(kMonsters); }
std::span<const ItemDef> itemDefs() { return std::span<const ItemDef>(kItems); }

const MonsterDef* findMonsterDef(std::string_view id) {
    for (const auto& d : kMonsters) {
        if (std::string_view(d.id) == id) return &d;
    }
    return nullptr;
}

const ItemDef* findItemDef(std::string_view id) {
    for (const auto& d : kItems) {
        if (std::string_view(d.id) == id) return &d;
    }
    return nullptr;
}

const MonsterDef* pickMonsterDef(Rng& rng, int floor) {
    return pickWeighted<MonsterDef>(monsterDefs(), rng, [floor](const MonsterDef& d) {
        return d.minFloor <= floor && floor <= d.maxFloor;
    });
}

const ItemDef* pickItemDef(Rng& rng, int floor) {
    return pickWeighted<ItemDef>(itemDefs(), rng,
                                 [floor](const ItemDef& d) { return d.minFloor <= floor; });
}

entt::entity spawn(entt::registry& reg, std::string_view id, Position pos) {
    if (const auto* m = findMonsterDef(id)) {
        const auto e = reg.create();
        reg.emplace<Position>(e, pos);
        reg.emplace<Renderable>(e, Renderable{m->glyph, m->color, 2});
        reg.emplace<Name>(e, Name{m->name});
        reg.emplace<BlocksMovement>(e);
        reg.emplace<Health>(e, Health{m->hp, m->hp});
        reg.emplace<Stats>(e, Stats{m->attack, m->defense});
        reg.emplace<XpReward>(e, XpReward{m->xp});
        reg.emplace<AI>(e, AI{m->ai});
        reg.emplace<Faction>(e, Faction{Side::Monster});
        return e;
    }
    if (const auto* i = findItemDef(id)) {
        const auto e = reg.create();
        reg.emplace<Position>(e, pos);
        reg.emplace<Renderable>(e, Renderable{i->glyph, i->color, 1});
        reg.emplace<Name>(e, Name{i->name});
        reg.emplace<Item>(e);
        switch (i->kind) {
            case ItemKind::Potion:
                reg.emplace<Consumable>(e, Consumable{Effect::Heal, i->power});
                break;
            case ItemKind::Weapon:
                reg.emplace<Equippable>(e, Equippable{Slot::Weapon, i->power, 0});
                break;
            case ItemKind::Armor:
                reg.emplace<Equippable>(e, Equippable{Slot::Armor, 0, i->power});
                break;
        }
        return e;
    }
    assert(false && "unknown template id");
    return entt::null;
}

entt::entity spawnGold(entt::registry& reg, Position pos, int amount) {
    const auto e = reg.create();
    reg.emplace<Position>(e, pos);
    reg.emplace<Renderable>(e, Renderable{'$', Color::Yellow, 1});
    reg.emplace<Name>(e, Name{"Gold"});
    reg.emplace<Item>(e);
    reg.emplace<Gold>(e, Gold{amount});
    return e;
}

entt::entity spawnStairs(entt::registry& reg, Position pos) {
    const auto e = reg.create();
    reg.emplace<Position>(e, pos);
    reg.emplace<Renderable>(e, Renderable{'>', Color::White, 0});
    reg.emplace<Name>(e, Name{"Stairs"});
    reg.emplace<Stairs>(e);
    return e;
}

entt::entity spawnPlayer(entt::registry& reg, Position pos) {
    const auto e = reg.create();
    reg.emplace<Player>(e);
    reg.emplace<Position>(e, pos);
    reg.emplace<Renderable>(e, Renderable{'@', Color::White, 3});
    reg.emplace<Name>(e, Name{"You"});
    reg.emplace<BlocksMovement>(e);
    reg.emplace<Health>(e, Health{30, 30});
    reg.emplace<Stats>(e, Stats{4, 1});
    reg.emplace<Level>(e, Level{1, 0});
    reg.emplace<Faction>(e, Faction{Side::Player});
    reg.emplace<Inventory>(e);
    reg.emplace<Equipment>(e);
    reg.emplace<Gold>(e, Gold{0});
    reg.emplace<Viewshed>(e, Viewshed{8});
    return e;
}
