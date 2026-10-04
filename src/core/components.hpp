#pragma once

#include <entt/entt.hpp>
#include <string>
#include <vector>

enum class Color { White, Gray, Red, Green, Yellow, Blue, Magenta, Cyan };
enum class AIKind { Wander, Chase, Boss };
enum class Side { Player, Monster };
enum class Slot { Weapon, Armor };
enum class Effect { Heal };

struct Position {
    int x = 0;
    int y = 0;
    bool operator==(const Position&) const = default;
};
struct Renderable {
    char glyph = '?';
    Color color = Color::White;
    int layer = 0;  // больше = рисуется поверх
};
struct Name { std::string value; };
struct BlocksMovement {};
struct Health { int cur = 1; int max = 1; };
struct Stats { int attack = 0; int defense = 0; };
struct Level { int lvl = 1; int xp = 0; };
struct XpReward { int amount = 0; };
struct Player {};
struct AI { AIKind kind = AIKind::Wander; };
struct Faction { Side side = Side::Monster; };
struct Item {};
struct Consumable { Effect effect = Effect::Heal; int power = 0; };
struct Equippable { Slot slot = Slot::Weapon; int atkBonus = 0; int defBonus = 0; };
struct Gold { int amount = 0; };  // на полу — куча, на игроке — кошелёк
struct Inventory { std::vector<entt::entity> items; };
struct Equipment {
    entt::entity weapon = entt::null;
    entt::entity armor = entt::null;
};
struct InInventory { entt::entity owner = entt::null; };
struct Stairs {};
struct Viewshed { int radius = 8; };
