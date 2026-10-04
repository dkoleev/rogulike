#pragma once

#include <cstddef>
#include <cstdint>
#include <entt/entt.hpp>

#include "core/components.hpp"
#include "core/map.hpp"
#include "core/rng.hpp"

constexpr int kMapWidth = 80;
constexpr int kMapHeight = 24;
constexpr int kFinalFloor = 5;
constexpr std::size_t kMaxInventory = 9;

struct World {
    explicit World(std::uint32_t seed) : rng(seed) {}

    entt::registry reg;
    entt::dispatcher events;
    Map map{kMapWidth, kMapHeight};
    Rng rng;
    int floor = 1;
    entt::entity player{entt::null};
    bool bossKilled = false;
};
