#pragma once

#include <string>

struct Attacked { std::string attacker; std::string target; int damage; bool byPlayer; };
struct Died { std::string name; bool isPlayer; };
struct ItemPickedUp { std::string name; };
struct GoldPicked { int amount; };
struct Healed { int amount; };
struct Equipped { std::string name; };
struct LevelUp { int level; };
struct FloorChanged { int floor; };
struct Note { std::string text; };
