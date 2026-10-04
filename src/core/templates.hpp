#pragma once

#include <span>
#include <string_view>

#include "core/components.hpp"
#include "core/rng.hpp"

enum class ItemKind { Potion, Weapon, Armor };

struct MonsterDef {
    const char* id;
    const char* name;
    char glyph;
    Color color;
    int hp;
    int attack;
    int defense;
    int xp;
    AIKind ai;
    int minFloor;
    int maxFloor;
    int weight;  // 0 = случайно не выбирается
};

struct ItemDef {
    const char* id;
    const char* name;
    char glyph;
    Color color;
    ItemKind kind;
    int power;  // Potion: лечение, Weapon: бонус атаки, Armor: бонус защиты
    int minFloor;
    int weight;
};

std::span<const MonsterDef> monsterDefs();
std::span<const ItemDef> itemDefs();
const MonsterDef* findMonsterDef(std::string_view id);
const ItemDef* findItemDef(std::string_view id);
const MonsterDef* pickMonsterDef(Rng& rng, int floor);
const ItemDef* pickItemDef(Rng& rng, int floor);

// Монстр или предмет по id. Неизвестный id — assert.
entt::entity spawn(entt::registry& reg, std::string_view id, Position pos);
entt::entity spawnGold(entt::registry& reg, Position pos, int amount);
entt::entity spawnStairs(entt::registry& reg, Position pos);
entt::entity spawnPlayer(entt::registry& reg, Position pos);
