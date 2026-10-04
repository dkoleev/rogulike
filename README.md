# Roguelike

A turn-based roguelike for the command line. Explore five procedurally generated dungeon floors, collect gear, level up, and defeat the dragon waiting on the last floor. Death is permanent: when you die, the run is over.

```
HP 24/30  Lv 2  XP 18/40  Floor 3  Gold 45  Atk 6  Def 2
 ################
 #..g....@..!...#
 ...
You hit the Goblin for 3.
The Goblin hits you for 2.
```

---

# Part 1: Playing the game

## Goal

Descend through floors 1–5 and kill the dragon (`D`) on floor 5. Killing it wins the run. If your HP reaches 0 you die, and every run starts from scratch on a new, randomly generated dungeon.

## Running

Build the game (see [Building](#building)) and start the executable:

```
./roguelike          # random dungeon
./roguelike 42       # fixed seed: the same number always gives the same dungeon
```

The game prints its seed when you quit, so you can replay an interesting dungeon.

**Terminal requirements:** at least 80 columns by 29 rows, with ANSI color support (Windows Terminal, any modern Linux/macOS terminal). If the window is too small, the game asks you to resize it. Run it in a real terminal, not in an IDE's output pane.

## Controls

| Key | Action |
|---|---|
| `W` `A` `S` `D` or arrow keys | Move. Walking into a monster attacks it |
| `g` | Pick up the item(s) on your tile |
| `i` or `e` | Open or close the inventory |
| `1`–`9` (in the inventory) | Use a potion, or equip a weapon/armor |
| `Esc` (in the inventory) | Close the inventory |
| `>` | Descend (you must stand on the stairs) |
| `z` | Wait a turn |
| `q` | Quit |
| `r` (after death or victory) | Start a new run |

Moving into a wall, picking up nothing, or pressing an invalid key does not cost a turn. The world only moves when you act.

## What you see

| Symbol | Meaning |
|---|---|
| `@` | You |
| `#` `.` | Wall, floor |
| `>` | Stairs down |
| `$` | Gold |
| `!` | Potion |
| `/` | Weapon |
| `[` | Armor |
| `r` `g` `o` `T` | Rat, Goblin, Orc, Troll |
| `D` | Dragon (final boss) |

You only see what is in your line of sight (radius 8). Areas you have explored stay on the map in gray, but monsters and items outside your view are not shown.

The top line shows your status: HP, level, XP, current floor, gold, and your effective attack and defense (equipment included). The last four lines are the message log.

## Monsters

| Monster | Floors | Behavior |
|---|---|---|
| Rat `r` | 1–2 | Wanders; attacks if you stand next to it |
| Goblin `g` | 1–5 | Hunts you once it sees you |
| Orc `o` | 3–5 | Hunts you once it sees you; tougher |
| Troll `T` | 4–5 | Hunts you once it sees you; very tough |
| Dragon `D` | 5 | Always hunts you; waits in the stairs-less final room |

Deeper floors have more monsters and better loot. Monsters sometimes drop items when they die.

## Items

- **Health Potion / Greater Health Potion `!`**: restore HP. They are not used up if you are already at full health.
- **Dagger, Sword `/`**: add to your attack. Only one weapon is equipped at a time.
- **Leather Armor, Chain Mail `[`**: add to your defense. Only one armor is equipped at a time.
- **Gold `$`**: collected automatically into your purse and shown in the status line.

Equipping an item puts the one you were wearing back into your pack. The pack holds 9 items; if it is full you cannot pick up more.

## Combat and progression

- Damage is `max(1, attack − defense)` plus 0–1 random bonus. There are no misses.
- You start with 30 HP, attack 4, defense 1.
- Killing monsters gives XP. You level up at `20 × level` XP. Each level adds +5 max HP, heals 5 HP, and adds +1 attack.

## Tips

- Fight in corridors so only one monster can reach you at a time.
- Drink potions when you are low. The pack is small, so do not hoard.
- Do not rush the stairs: the dragon is much easier with a Sword, Chain Mail and a few levels.

---

# Part 2: Architecture

## Overview

The project is a C++20 application built with CMake. It is split into three layers with strict dependency direction:

```
main.cpp ──► ui (terminal, input, renderer) ──► core (game logic)
```

- **`core`** contains all game rules and state. It has no I/O and knows nothing about the terminal.
- **`ui`** reads the game state to draw it and turns key presses into commands. It never mutates the world directly.
- **`main.cpp`** wires them together in a loop: draw → read key → map to a command → `Game::tick(command)`.

The only entry point into the game logic is `Game::tick(const Command&)`. This makes the whole game testable without a terminal.

## Tech stack

| Component | Choice |
|---|---|
| Language | C++20 |
| Build | CMake ≥ 3.24 |
| ECS | [EnTT](https://github.com/skypjack/entt) v3.14.0 |
| Tests | [Catch2](https://github.com/catchorg/Catch2) v3.7.1 |
| Output | ANSI escape sequences (identical on all platforms) |

Both dependencies are fetched by CMake `FetchContent` with pinned versions, so the first configure needs network access and `git`.

## Directory layout

```
CMakeLists.txt
src/
  main.cpp                  # game loop
  core/                     # game logic, no I/O
    components.hpp          # all ECS components and enums
    events.hpp              # events sent through entt::dispatcher
    strings.hpp             # fixed UI/log texts
    messages.{hpp,cpp}      # MessageLog: turns events into log lines
    world.hpp               # World: registry, dispatcher, map, rng, floor, player
    rng.hpp                 # deterministic, seedable Rng
    map.{hpp,cpp}           # tiles, visibility, explored flags
    templates.{hpp,cpp}     # monster/item tables and spawn functions
    generator.{hpp,cpp}     # floor layout and population
    command.hpp             # Command / CommandType
    game.{hpp,cpp}          # Game: state machine and turn order
    systems/                # one file per system, declarations in systems.hpp
      combat.cpp  movement.cpp  death.cpp  progression.cpp
      inventory.cpp  fov.cpp  ai.cpp
  ui/
    terminal.{hpp,cpp}      # Terminal base class (ANSI output)
    terminal_win.cpp        # Windows: raw input, VT mode, window size
    terminal_posix.cpp      # POSIX: termios, poll, ioctl
    input.{hpp,cpp}         # commandFromKey(): key + state -> Command
    renderer.{hpp,cpp}      # renderFrame(): Game -> string
tests/                      # Catch2 tests for core, input mapping and renderer
.github/workflows/build.yml # CI: build and test on Linux, Windows, macOS
```

## Core design

### Entity Component System

Game objects are EnTT entities assembled from plain-struct components (`Position`, `Health`, `Stats`, `AI`, `Inventory`, …). Components hold data only; behavior lives in systems. The hero, monsters, items, gold and stairs are all entities, so there is no class hierarchy: a new kind of object is a new combination of components.

### Data-driven templates

Monsters and items are described by rows in two tables in `templates.cpp` (`MonsterDef`, `ItemDef`): glyph, color, stats, floor range and spawn weight. `spawn(registry, id, position)` builds an entity from a row. Adding a monster or an item means adding a row to the table, with no new code. An unknown id triggers an `assert`.

### Systems

Systems are free functions over `World&`, declared in `core/systems/systems.hpp`:

| System | Responsibility |
|---|---|
| `tryMove` / `attack` | Movement; moving into an enemy becomes an attack. Damage = `max(1, atk − def) + rng(0,1)` |
| `deathSystem` | Removes dead monsters, awards XP, rolls loot (30%), flags a killed boss |
| `progressionSystem` | Level-ups |
| `pickupItems` / `useItem` / `isOnStairs` | Inventory, equipment, stairs check |
| `fovSystem` | Recursive shadowcasting over 8 octants; updates `visible` / `explored` on the map |
| `aiSystem` | Wander / Chase / Boss behavior; BFS pathfinding on a 4-connected grid |

### Turn order

`Game::tick` first interprets the command for the current state. If the action costs a turn, `endTurn` runs the world:

```
deathSystem → progressionSystem → (boss killed? → Won)
            → fovSystem → aiSystem → deathSystem → (player HP ≤ 0? → Dead)
```

Actions that do not cost a turn (bumping a wall, an invalid inventory index, descending away from stairs, any command after the run has ended) return `false` and leave the world untouched.

### Game states

`GameState` is one of `Playing`, `Inventory`, `Dead`, `Won`. The input layer picks the meaning of a key from the current state, and `Game::tick` ignores commands that do not make sense in it. `Quit` and `Restart` are handled by `main` (a restart creates a new `Game`).

### Events and the message log

Systems report what happened through `entt::dispatcher` events (`Attacked`, `Died`, `ItemPickedUp`, `Healed`, `LevelUp`, `FloorChanged`, …). `MessageLog` subscribes to them and formats the text shown in the UI. Fixed texts live in `strings.hpp`, formatted lines in `messages.cpp`, which keeps all English text in two places and makes localization straightforward.

### Procedural generation

`generateLayout` places 6–9 non-overlapping rooms on an 80×24 grid and joins consecutive rooms with L-shaped corridors, so every floor is connected. `buildFloor` then clears the previous floor, places the player in the first room, the stairs (or the dragon on floor 5) in the last room, and fills the other rooms with monsters, items and gold chosen by floor depth and spawn weight.

### Determinism

All randomness goes through `Rng`, a thin wrapper over `std::mt19937` with its own range arithmetic (not `std::uniform_int_distribution`, whose output differs between standard libraries). The same seed therefore produces the same dungeon and the same run on every platform.

### Invariants worth knowing

- An entity in the pack or equipped has **no `Position`**. `buildFloor` relies on this: it destroys every entity with a `Position` except the player, and everything the player carries survives.
- A dead player is not destroyed; `Game` switches to `Dead` instead.
- `MessageLog` stays connected to the dispatcher, so `Game` declares its members in the order `world_`, `log_`: the log is destroyed first.

## UI layer

- **`Terminal`**: an abstract class with ANSI output (`write`, alternate screen, cursor hiding) in the base and platform code in two subclasses: `terminal_win.cpp` (`_getch`, VT mode, console size) and `terminal_posix.cpp` (`termios` raw mode, `poll`-based escape-sequence parsing, `ioctl` size). `makeTerminal()` is defined once per platform and CMake compiles only the right file. The constructor sets up raw mode and the alternate screen; the destructor restores the console, so the terminal is repaired on any normal exit, including exceptions caught in `main`. Ctrl-C is read as a normal key and mapped to quit.
- **`commandFromKey`**: a pure function from a key event and the game state to a `Command`.
- **`Renderer::renderFrame`**: a pure function from `Game` to a string of exactly 29 lines (status line, 24 map rows, 4 message rows). It draws entities only on currently visible cells, remembered terrain in gray, and the inventory as an overlay panel. `draw` just homes the cursor and writes the frame.

Because input mapping and rendering are pure, both are covered by unit tests without a real terminal.

## Building

Requirements: a C++20 compiler, CMake ≥ 3.24, `git` and network access for the first configure.

```
cmake -S . -B build -DCMAKE_BUILD_TYPE=Release
cmake --build build --parallel
./build/roguelike
```

With a multi-config generator such as Visual Studio, add `--config Release` to the build command; the executable ends up in `build/Release/`.

**MinGW note:** `libstdc++` and `libgcc` are linked statically, and `libwinpthread-1.dll` is copied next to the executable after the build, so the game runs outside the IDE. Keep that DLL next to `roguelike.exe` if you move it.

## Testing

```
cmake --build build --target game_tests
ctest --test-dir build -C Release --output-on-failure
# or run the binary directly
./build/game_tests            # all tests
./build/game_tests "[combat]" # a tag
```

Tests (Catch2) cover the whole `core` library, the input mapping and the renderer. Shared helpers are in `tests/test_helpers.hpp` (an open test room, item helpers) and `tests/game_helpers.hpp` (replace a generated floor with a controlled one). Seeds are fixed, so tests are deterministic.

## Continuous integration

`.github/workflows/build.yml` builds and runs the tests on Ubuntu, Windows and macOS for every pull request, and uploads the executable as a build artifact.

## Extending the game

| To add… | Do this |
|---|---|
| A monster or item | Add a row to the table in `core/templates.cpp`; add a test if it needs special behavior |
| A new action | Add a `CommandType`, map a key in `ui/input.cpp`, handle it in `Game::tick`, add tests |
| A new system | Add a function to `core/systems/`, declare it in `systems.hpp`, call it from `Game::endTurn` in the right place |
| A new event/message | Add a struct to `events.hpp`, a handler in `MessageLog`, trigger it from a system |
| A new platform | Implement `Terminal` (`readKey`, `size`) and `makeTerminal()` in a new `terminal_*.cpp` and select it in CMake |

## Design documents

The original design spec and the implementation plan are in [`docs/superpowers/`](docs/superpowers/).
