#pragma once

#include <vector>

#include "core/components.hpp"
#include "core/map.hpp"
#include "core/rng.hpp"
#include "core/world.hpp"

struct Room {
    int x;
    int y;
    int w;
    int h;
    Position center() const { return Position{x + w / 2, y + h / 2}; }
};

struct Layout {
    Map map;
    std::vector<Room> rooms;
};

// Комнаты + L-образные коридоры; все комнаты связаны.
Layout generateLayout(Rng& rng);

// Новый этаж: чистит старый (кроме игрока и его вещей), строит карту и наполняет её.
void buildFloor(World& world, int floor);
