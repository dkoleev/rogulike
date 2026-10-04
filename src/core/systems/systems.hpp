#pragma once

#include <entt/entt.hpp>

#include "core/world.hpp"

enum class MoveResult { Blocked, Moved, Attacked };

// combat.cpp
int effectiveAttack(const entt::registry& reg, entt::entity e);
int effectiveDefense(const entt::registry& reg, entt::entity e);
void attack(World& w, entt::entity attacker, entt::entity target);

// movement.cpp
MoveResult tryMove(World& w, entt::entity actor, int dx, int dy);

// death.cpp
void deathSystem(World& w);

// progression.cpp
int xpToNext(int level);
void progressionSystem(World& w);

// inventory.cpp
bool pickupItems(World& w);       // true, если что-то подобрано
bool useItem(World& w, int index);  // true, если действие выполнено (ход потрачен)
bool isOnStairs(const World& w);

// fov.cpp
void fovSystem(World& w);

// ai.cpp
void aiSystem(World& w);
