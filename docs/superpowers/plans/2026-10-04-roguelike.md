# Консольный рогалик Implementation Plan

> **For agentic workers:** REQUIRED SUB-SKILL: Use superpowers:subagent-driven-development (recommended) or superpowers:executing-plans to implement this plan task-by-task. Steps use checkbox (`- [ ]`) syntax for tracking.

**Goal:** Консольный рогалик на C++20: ASCII-карта, 5 этажей, босс, инвентарь, уровни, туман войны.

**Architecture:** `core` (EnTT ECS, чистая логика, без I/O) + `ui` (терминал, рендер, ввод). `Game::tick(Command)` — единственная точка входа в логику. Платформенный код только в `ui/terminal_*.cpp`. Шаблоны врагов/предметов — таблицы в `templates.cpp`.

**Tech Stack:** C++20, CMake (FetchContent), EnTT v3.14.0, Catch2 v3.7.1, ANSI-escape вывод.

**Spec:** `docs/superpowers/specs/2026-10-04-roguelike-design.md`

## Global Constraints

- C++20, CMake, CLion. Зависимости только EnTT и Catch2 через `FetchContent`, версии закреплены.
- Платформы: Windows и POSIX. Платформенный код (сырой ввод, размер окна, режим консоли) только за интерфейсом `Terminal`.
- Язык интерфейса: английский. Фиксированные строки в `core/strings.hpp`; параметризованные строки журнала в `core/messages.cpp`.
- `core` не знает о терминале. `ui` читает мир, не меняет его.
- Случайность только через `Rng` с seed (одинаковый seed → одинаковый забег).
- Карта 80×24. 5 этажей, босс (`dragon`) на 5-м. Победа = убийство босса.
- Бой без промахов: урон = `max(1, attack − defense) + rng.range(0, 1)`.
- Неизвестный id шаблона — `assert`. Обычная игра не бросает исключений.
- Терминал восстанавливается через RAII (в т.ч. при исключении).
- Инвариант: сущность в инвентаре/экипировке не имеет `Position`.

## Review Focus

- Индекс предмета вне диапазона / пустой инвентарь → ход не тратится, без падения (test: Task 4 `useItem rejects bad index`, Task 7 `bad inventory index`).
- Зелье при полном HP → не расходуется, ход не тратится (test: Task 4 `potion at full health`).
- Подбор при полном рюкзаке (9) → предмет остаётся на полу, сообщение, ход не тратится (test: Task 4 `pickup with full pack`).
- Команды после смерти/победы игнорируются, мир не меняется (test: Task 7 `commands after death`, `commands after victory`).
- `>` вне лестницы → этаж не меняется, ход не тратится (test: Task 7 `descend off stairs`).

---

## File Structure

| Файл | Ответственность |
|---|---|
| `CMakeLists.txt` | цели `game_core`, `game_ui`, `roguelike` (exe), `game_tests` |
| `src/core/rng.hpp` | детерминированный Rng |
| `src/core/map.{hpp,cpp}` | тайлы, видимость, исследованность |
| `src/core/components.hpp` | все компоненты и enum |
| `src/core/events.hpp` | события dispatcher |
| `src/core/strings.hpp` | фиксированные тексты |
| `src/core/messages.{hpp,cpp}` | `MessageLog` |
| `src/core/world.hpp` | `World` + константы |
| `src/core/templates.{hpp,cpp}` | таблицы, `spawn*`, взвешенный выбор |
| `src/core/systems/systems.hpp` | объявления систем |
| `src/core/systems/{combat,movement,death,progression,inventory,fov,ai}.cpp` | системы |
| `src/core/generator.{hpp,cpp}` | раскладка этажа, `buildFloor` |
| `src/core/command.hpp` | `Command`, `CommandType` |
| `src/core/game.{hpp,cpp}` | `Game`, `GameState`, `tick` |
| `src/ui/terminal.{hpp,cpp}` | базовый `Terminal` (ANSI), `KeyEvent` |
| `src/ui/terminal_win.cpp`, `terminal_posix.cpp` | платформенные реализации + `makeTerminal()` |
| `src/ui/input.{hpp,cpp}` | `commandFromKey` |
| `src/ui/renderer.{hpp,cpp}` | `renderFrame`, `draw` |
| `src/main.cpp` | цикл |
| `tests/*.cpp`, `tests/test_helpers.hpp`, `tests/game_helpers.hpp` | тесты |

Сборка/запуск тестов (используется во всех задачах):
- Build: `cmake -S . -B cmake-build-debug && cmake --build cmake-build-debug --target game_tests`
- Run: `./cmake-build-debug/game_tests` (или с фильтром `"[tag]"`)

---

### Task 1: Каркас, Rng, Map

**Files:**
- Create: `.gitignore`, `src/core/rng.hpp`, `src/core/map.hpp`, `src/core/map.cpp`, `tests/test_map_rng.cpp`
- Modify: `CMakeLists.txt` (полная замена)

**Interfaces:**
- Produces: `Rng(uint32_t)`, `int range(int lo,int hi)` (включительно), `bool chance(int percent)`; `Tile{Wall,Floor}`; `Map(int w=80,int h=24)` с `width() height() inBounds tile setTile walkable opaque visible explored setVisible clearVisible`.

- [ ] **Step 1: git init + baseline**

```bash
git init
printf 'cmake-build-*/\n.idea/\n' > .gitignore
git add .gitignore CMakeLists.txt main.cpp docs
git commit -m "chore: baseline project, spec and plan"
```

- [ ] **Step 2: Заменить `CMakeLists.txt`**

```cmake
cmake_minimum_required(VERSION 3.24)
project(roguelike)

set(CMAKE_CXX_STANDARD 20)
set(CMAKE_CXX_STANDARD_REQUIRED ON)
set(CMAKE_CXX_EXTENSIONS OFF)

include(FetchContent)
FetchContent_Declare(EnTT
        GIT_REPOSITORY https://github.com/skypjack/entt.git
        GIT_TAG v3.14.0
        GIT_SHALLOW TRUE)
FetchContent_Declare(Catch2
        GIT_REPOSITORY https://github.com/catchorg/Catch2.git
        GIT_TAG v3.7.1
        GIT_SHALLOW TRUE)
FetchContent_MakeAvailable(EnTT Catch2)

file(GLOB_RECURSE CORE_SOURCES CONFIGURE_DEPENDS src/core/*.cpp)
add_library(game_core STATIC ${CORE_SOURCES})
target_include_directories(game_core PUBLIC src)
target_link_libraries(game_core PUBLIC EnTT::EnTT)

# временно, заменится в Task 9
add_executable(roguelike main.cpp)

enable_testing()
file(GLOB TEST_SOURCES CONFIGURE_DEPENDS tests/*.cpp)
add_executable(game_tests ${TEST_SOURCES})
target_include_directories(game_tests PRIVATE tests)
target_link_libraries(game_tests PRIVATE game_core Catch2::Catch2WithMain)
add_test(NAME game_tests COMMAND game_tests)
```

- [ ] **Step 3: Написать падающие тесты `tests/test_map_rng.cpp`**

```cpp
#include <catch2/catch_test_macros.hpp>

#include "core/map.hpp"
#include "core/rng.hpp"

TEST_CASE("Map starts as solid wall", "[map]") {
    Map m(10, 5);
    REQUIRE(m.width() == 10);
    REQUIRE(m.height() == 5);
    REQUIRE(m.tile(3, 3) == Tile::Wall);
    REQUIRE_FALSE(m.walkable(3, 3));
    REQUIRE(m.opaque(3, 3));
}

TEST_CASE("Map out of bounds is safe", "[map]") {
    Map m(10, 5);
    REQUIRE_FALSE(m.inBounds(-1, 0));
    REQUIRE_FALSE(m.inBounds(10, 0));
    REQUIRE_FALSE(m.inBounds(0, 5));
    REQUIRE(m.tile(-1, -1) == Tile::Wall);
    REQUIRE_FALSE(m.walkable(99, 99));
    REQUIRE(m.opaque(99, 99));
    m.setTile(99, 99, Tile::Floor);
    m.setVisible(99, 99, true);
    REQUIRE_FALSE(m.visible(99, 99));
    REQUIRE_FALSE(m.explored(99, 99));
}

TEST_CASE("Floor tiles are walkable and transparent", "[map]") {
    Map m(10, 5);
    m.setTile(2, 2, Tile::Floor);
    REQUIRE(m.walkable(2, 2));
    REQUIRE_FALSE(m.opaque(2, 2));
}

TEST_CASE("Visible cells become explored and stay explored", "[map]") {
    Map m(10, 5);
    m.setVisible(2, 2, true);
    REQUIRE(m.visible(2, 2));
    REQUIRE(m.explored(2, 2));
    m.clearVisible();
    REQUIRE_FALSE(m.visible(2, 2));
    REQUIRE(m.explored(2, 2));
}

TEST_CASE("Rng is deterministic per seed", "[rng]") {
    Rng a(42), b(42);
    for (int i = 0; i < 20; ++i) REQUIRE(a.range(0, 1000) == b.range(0, 1000));
}

TEST_CASE("Rng range is inclusive and bounded", "[rng]") {
    Rng r(1);
    bool seen[3] = {false, false, false};
    for (int i = 0; i < 1000; ++i) {
        const int v = r.range(3, 5);
        REQUIRE(v >= 3);
        REQUIRE(v <= 5);
        seen[v - 3] = true;
    }
    REQUIRE(seen[0]);
    REQUIRE(seen[1]);
    REQUIRE(seen[2]);
}

TEST_CASE("Rng range with hi <= lo returns lo", "[rng]") {
    Rng r(1);
    REQUIRE(r.range(7, 7) == 7);
    REQUIRE(r.range(7, 3) == 7);
}

TEST_CASE("Rng chance extremes", "[rng]") {
    Rng r(1);
    for (int i = 0; i < 200; ++i) {
        REQUIRE_FALSE(r.chance(0));
        REQUIRE(r.chance(100));
    }
}
```

- [ ] **Step 4: Run — ожидается FAIL (нет `core/map.hpp`)**

Run: Build. Expected: ошибка компиляции `core/map.hpp: No such file`.

- [ ] **Step 5: `src/core/rng.hpp`**

```cpp
#pragma once

#include <cstdint>
#include <random>

// mt19937 стандартизован, поэтому последовательность одинакова на всех платформах.
// uniform_int_distribution не используем: его результат зависит от реализации.
class Rng {
public:
    explicit Rng(std::uint32_t seed) : engine_(seed) {}

    // Включительно с обеих сторон. При hi <= lo возвращает lo.
    int range(int lo, int hi) {
        if (hi <= lo) return lo;
        const auto span = static_cast<std::uint32_t>(hi - lo) + 1u;
        return lo + static_cast<int>(static_cast<std::uint32_t>(engine_()) % span);
    }

    bool chance(int percent) { return range(1, 100) <= percent; }

private:
    std::mt19937 engine_;
};
```

- [ ] **Step 6: `src/core/map.hpp`**

```cpp
#pragma once

#include <vector>

enum class Tile : unsigned char { Wall, Floor };

class Map {
public:
    explicit Map(int width = 80, int height = 24);

    int width() const { return width_; }
    int height() const { return height_; }

    bool inBounds(int x, int y) const;
    Tile tile(int x, int y) const;          // вне карты -> Wall
    void setTile(int x, int y, Tile t);     // вне карты игнорируется
    bool walkable(int x, int y) const;
    bool opaque(int x, int y) const;        // вне карты -> true

    bool visible(int x, int y) const;
    bool explored(int x, int y) const;
    void setVisible(int x, int y, bool v);  // v=true также помечает explored
    void clearVisible();

private:
    int index(int x, int y) const { return y * width_ + x; }

    int width_;
    int height_;
    std::vector<Tile> tiles_;
    std::vector<char> visible_;
    std::vector<char> explored_;
};
```

- [ ] **Step 7: `src/core/map.cpp`**

```cpp
#include "core/map.hpp"

#include <algorithm>

Map::Map(int width, int height)
    : width_(width),
      height_(height),
      tiles_(static_cast<std::size_t>(width * height), Tile::Wall),
      visible_(static_cast<std::size_t>(width * height), 0),
      explored_(static_cast<std::size_t>(width * height), 0) {}

bool Map::inBounds(int x, int y) const {
    return x >= 0 && y >= 0 && x < width_ && y < height_;
}

Tile Map::tile(int x, int y) const {
    return inBounds(x, y) ? tiles_[static_cast<std::size_t>(index(x, y))] : Tile::Wall;
}

void Map::setTile(int x, int y, Tile t) {
    if (inBounds(x, y)) tiles_[static_cast<std::size_t>(index(x, y))] = t;
}

bool Map::walkable(int x, int y) const { return tile(x, y) == Tile::Floor; }

bool Map::opaque(int x, int y) const { return tile(x, y) == Tile::Wall; }

bool Map::visible(int x, int y) const {
    return inBounds(x, y) && visible_[static_cast<std::size_t>(index(x, y))] != 0;
}

bool Map::explored(int x, int y) const {
    return inBounds(x, y) && explored_[static_cast<std::size_t>(index(x, y))] != 0;
}

void Map::setVisible(int x, int y, bool v) {
    if (!inBounds(x, y)) return;
    const auto i = static_cast<std::size_t>(index(x, y));
    visible_[i] = v ? 1 : 0;
    if (v) explored_[i] = 1;
}

void Map::clearVisible() { std::fill(visible_.begin(), visible_.end(), 0); }
```

- [ ] **Step 8: Run — ожидается PASS** (Build + `./cmake-build-debug/game_tests`). Первый configure скачает EnTT и Catch2 (нужны git и сеть).

- [ ] **Step 9: Commit**

```bash
git add CMakeLists.txt src tests
git commit -m "feat: cmake scaffold with EnTT/Catch2, Rng and Map"
```

---

### Task 2: Компоненты, события, строки, MessageLog, World, шаблоны

**Files:**
- Create: `src/core/components.hpp`, `events.hpp`, `strings.hpp`, `messages.hpp`, `messages.cpp`, `world.hpp`, `templates.hpp`, `templates.cpp`
- Test: `tests/test_messages.cpp`, `tests/test_templates.cpp`

**Interfaces:**
- Consumes: `Rng`, `Map` (Task 1).
- Produces: все компоненты (см. код); `World{reg,events,map,rng,floor,player,bossKilled}`; `kMapWidth=80 kMapHeight=24 kFinalFloor=5 kMaxInventory=9`; `MessageLog::attach/add/recent`; `spawn(reg,id,Position)`, `spawnGold`, `spawnStairs`, `spawnPlayer`, `pickMonsterDef(Rng&,int floor)`, `pickItemDef(Rng&,int floor)`, `findMonsterDef`, `findItemDef`, `monsterDefs()`, `itemDefs()`.

- [ ] **Step 1: Тесты `tests/test_messages.cpp`**

```cpp
#include <catch2/catch_test_macros.hpp>

#include "core/events.hpp"
#include "core/messages.hpp"

TEST_CASE("MessageLog formats combat events", "[messages]") {
    entt::dispatcher d;
    MessageLog log;
    log.attach(d);
    d.trigger(Attacked{"You", "Goblin", 3, true});
    d.trigger(Attacked{"Goblin", "You", 2, false});
    d.trigger(Died{"Goblin", false});
    d.trigger(Died{"You", true});
    const auto lines = log.recent(10);
    REQUIRE(lines.size() == 4);
    REQUIRE(lines[0] == "You hit the Goblin for 3.");
    REQUIRE(lines[1] == "The Goblin hits you for 2.");
    REQUIRE(lines[2] == "The Goblin dies.");
    REQUIRE(lines[3] == "You die...");
}

TEST_CASE("MessageLog formats item and progress events", "[messages]") {
    entt::dispatcher d;
    MessageLog log;
    log.attach(d);
    d.trigger(ItemPickedUp{"Dagger"});
    d.trigger(GoldPicked{12});
    d.trigger(Healed{5});
    d.trigger(Equipped{"Sword"});
    d.trigger(LevelUp{2});
    d.trigger(FloorChanged{1});
    d.trigger(FloorChanged{3});
    d.trigger(Note{"hello"});
    const auto lines = log.recent(10);
    REQUIRE(lines.size() == 8);
    REQUIRE(lines[0] == "You pick up the Dagger.");
    REQUIRE(lines[1] == "You pick up 12 gold.");
    REQUIRE(lines[2] == "You heal 5 HP.");
    REQUIRE(lines[3] == "You equip the Sword.");
    REQUIRE(lines[4] == "Welcome to level 2!");
    REQUIRE(lines[5] == "You enter the dungeon.");
    REQUIRE(lines[6] == "You descend to floor 3.");
    REQUIRE(lines[7] == "hello");
}

TEST_CASE("MessageLog keeps only the most recent lines", "[messages]") {
    MessageLog log(3);
    for (const char* s : {"m1", "m2", "m3", "m4", "m5"}) log.add(s);
    REQUIRE(log.recent(10).size() == 3);
    const auto last = log.recent(2);
    REQUIRE(last.size() == 2);
    REQUIRE(last[0] == "m4");
    REQUIRE(last[1] == "m5");
}
```

- [ ] **Step 2: Тесты `tests/test_templates.cpp`**

```cpp
#include <catch2/catch_test_macros.hpp>

#include "core/templates.hpp"
#include "core/world.hpp"

TEST_CASE("spawn builds a goblin from its template", "[templates]") {
    entt::registry reg;
    const auto e = spawn(reg, "goblin", Position{3, 4});
    REQUIRE(reg.get<Position>(e) == Position{3, 4});
    REQUIRE(reg.get<Name>(e).value == "Goblin");
    REQUIRE(reg.get<Health>(e).cur == reg.get<Health>(e).max);
    REQUIRE(reg.get<AI>(e).kind == AIKind::Chase);
    REQUIRE(reg.all_of<BlocksMovement>(e));
    REQUIRE(reg.get<Faction>(e).side == Side::Monster);
}

TEST_CASE("spawn builds items from templates", "[templates]") {
    entt::registry reg;
    const auto dagger = spawn(reg, "dagger", Position{1, 1});
    REQUIRE(reg.all_of<Item>(dagger));
    REQUIRE_FALSE(reg.all_of<BlocksMovement>(dagger));
    REQUIRE(reg.get<Equippable>(dagger).slot == Slot::Weapon);
    REQUIRE(reg.get<Equippable>(dagger).atkBonus == 2);

    const auto potion = spawn(reg, "health_potion", Position{1, 1});
    REQUIRE(reg.get<Consumable>(potion).effect == Effect::Heal);
    REQUIRE(reg.get<Consumable>(potion).power == 10);

    const auto mail = spawn(reg, "chain_mail", Position{1, 1});
    REQUIRE(reg.get<Equippable>(mail).slot == Slot::Armor);
    REQUIRE(reg.get<Equippable>(mail).defBonus == 3);
}

TEST_CASE("spawnPlayer assembles the hero", "[templates]") {
    entt::registry reg;
    const auto p = spawnPlayer(reg, Position{2, 2});
    REQUIRE(reg.all_of<Player>(p));
    REQUIRE(reg.get<Health>(p).max == 30);
    REQUIRE(reg.get<Inventory>(p).items.empty());
    REQUIRE(reg.get<Equipment>(p).weapon == entt::null);
    REQUIRE(reg.get<Gold>(p).amount == 0);
    REQUIRE(reg.get<Viewshed>(p).radius > 0);
    REQUIRE(reg.get<Faction>(p).side == Side::Player);
}

TEST_CASE("spawnGold and spawnStairs", "[templates]") {
    entt::registry reg;
    const auto g = spawnGold(reg, Position{1, 1}, 25);
    REQUIRE(reg.all_of<Item>(g));
    REQUIRE(reg.get<Gold>(g).amount == 25);
    const auto s = spawnStairs(reg, Position{2, 2});
    REQUIRE(reg.all_of<Stairs>(s));
    REQUIRE(reg.get<Renderable>(s).glyph == '>');
}

TEST_CASE("pickMonsterDef respects floor range and weight", "[templates]") {
    Rng rng(7);
    for (int i = 0; i < 500; ++i) {
        const auto* d1 = pickMonsterDef(rng, 1);
        REQUIRE(d1 != nullptr);
        REQUIRE(d1->minFloor <= 1);
        REQUIRE(std::string_view(d1->id) != "dragon");
        const auto* d5 = pickMonsterDef(rng, 5);
        REQUIRE(d5 != nullptr);
        REQUIRE(std::string_view(d5->id) != "dragon");
        REQUIRE(std::string_view(d5->id) != "rat");
    }
}

TEST_CASE("pickItemDef respects minFloor", "[templates]") {
    Rng rng(7);
    for (int i = 0; i < 500; ++i) {
        const auto* d = pickItemDef(rng, 1);
        REQUIRE(d != nullptr);
        REQUIRE(d->minFloor <= 1);
    }
}

TEST_CASE("every template id resolves", "[templates]") {
    for (const auto& m : monsterDefs()) REQUIRE(findMonsterDef(m.id) == &m);
    for (const auto& i : itemDefs()) REQUIRE(findItemDef(i.id) == &i);
    REQUIRE(findMonsterDef("nope") == nullptr);
}
```

- [ ] **Step 3: Run — FAIL** (нет заголовков).

- [ ] **Step 4: `src/core/components.hpp`**

```cpp
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
```

- [ ] **Step 5: `src/core/events.hpp`**

```cpp
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
```

- [ ] **Step 6: `src/core/strings.hpp`**

```cpp
#pragma once

// Фиксированные тексты. Параметризованные строки журнала — в messages.cpp.
namespace str {
inline constexpr const char* kNothingHere = "There is nothing here.";
inline constexpr const char* kPackFull = "Your pack is full.";
inline constexpr const char* kNoSuchItem = "No such item.";
inline constexpr const char* kFullHealth = "You are already at full health.";
inline constexpr const char* kNoStairs = "There are no stairs here.";
inline constexpr const char* kYouDie = "You die...";
inline constexpr const char* kEnterDungeon = "You enter the dungeon.";
inline constexpr const char* kVictory = "The dragon falls. You have conquered the dungeon!";

inline constexpr const char* kInventoryTitle = "=== Inventory ===";
inline constexpr const char* kInventoryHint = "Press 1-9 to use or equip an item, Esc to close.";
inline constexpr const char* kWeaponLabel = "Weapon: ";
inline constexpr const char* kArmorLabel = "Armor:  ";
inline constexpr const char* kNone = "none";
inline constexpr const char* kDiedBanner = "*** You died on floor ";
inline constexpr const char* kWonBanner = "*** Victory! Gold collected: ";
inline constexpr const char* kPressRestart = ". Press R to restart, any other key to exit. ***";
inline constexpr const char* kTooSmall = "Terminal too small. Resize the window, then press any key.";
}  // namespace str
```

- [ ] **Step 7: `src/core/messages.hpp`**

```cpp
#pragma once

#include <cstddef>
#include <deque>
#include <entt/entt.hpp>
#include <string>
#include <vector>

#include "core/events.hpp"

class MessageLog {
public:
    explicit MessageLog(std::size_t capacity = 100);
    MessageLog(const MessageLog&) = delete;
    MessageLog& operator=(const MessageLog&) = delete;

    void attach(entt::dispatcher& dispatcher);
    void add(std::string text);
    // последние count строк, от старых к новым
    std::vector<std::string> recent(std::size_t count) const;

private:
    void onAttacked(const Attacked& e);
    void onDied(const Died& e);
    void onItemPickedUp(const ItemPickedUp& e);
    void onGoldPicked(const GoldPicked& e);
    void onHealed(const Healed& e);
    void onEquipped(const Equipped& e);
    void onLevelUp(const LevelUp& e);
    void onFloorChanged(const FloorChanged& e);
    void onNote(const Note& e);

    std::deque<std::string> lines_;
    std::size_t capacity_;
};
```

- [ ] **Step 8: `src/core/messages.cpp`**

```cpp
#include "core/messages.hpp"

#include <algorithm>

#include "core/strings.hpp"

MessageLog::MessageLog(std::size_t capacity) : capacity_(capacity) {}

void MessageLog::attach(entt::dispatcher& d) {
    d.sink<Attacked>().connect<&MessageLog::onAttacked>(*this);
    d.sink<Died>().connect<&MessageLog::onDied>(*this);
    d.sink<ItemPickedUp>().connect<&MessageLog::onItemPickedUp>(*this);
    d.sink<GoldPicked>().connect<&MessageLog::onGoldPicked>(*this);
    d.sink<Healed>().connect<&MessageLog::onHealed>(*this);
    d.sink<Equipped>().connect<&MessageLog::onEquipped>(*this);
    d.sink<LevelUp>().connect<&MessageLog::onLevelUp>(*this);
    d.sink<FloorChanged>().connect<&MessageLog::onFloorChanged>(*this);
    d.sink<Note>().connect<&MessageLog::onNote>(*this);
}

void MessageLog::add(std::string text) {
    lines_.push_back(std::move(text));
    while (lines_.size() > capacity_) lines_.pop_front();
}

std::vector<std::string> MessageLog::recent(std::size_t count) const {
    const auto n = static_cast<std::ptrdiff_t>(std::min(count, lines_.size()));
    return std::vector<std::string>(lines_.end() - n, lines_.end());
}

void MessageLog::onAttacked(const Attacked& e) {
    if (e.byPlayer) {
        add("You hit the " + e.target + " for " + std::to_string(e.damage) + ".");
    } else {
        add("The " + e.attacker + " hits you for " + std::to_string(e.damage) + ".");
    }
}

void MessageLog::onDied(const Died& e) {
    add(e.isPlayer ? std::string(str::kYouDie) : "The " + e.name + " dies.");
}

void MessageLog::onItemPickedUp(const ItemPickedUp& e) { add("You pick up the " + e.name + "."); }
void MessageLog::onGoldPicked(const GoldPicked& e) {
    add("You pick up " + std::to_string(e.amount) + " gold.");
}
void MessageLog::onHealed(const Healed& e) { add("You heal " + std::to_string(e.amount) + " HP."); }
void MessageLog::onEquipped(const Equipped& e) { add("You equip the " + e.name + "."); }
void MessageLog::onLevelUp(const LevelUp& e) {
    add("Welcome to level " + std::to_string(e.level) + "!");
}
void MessageLog::onFloorChanged(const FloorChanged& e) {
    add(e.floor <= 1 ? std::string(str::kEnterDungeon)
                     : "You descend to floor " + std::to_string(e.floor) + ".");
}
void MessageLog::onNote(const Note& e) { add(e.text); }
```

- [ ] **Step 9: `src/core/world.hpp`**

```cpp
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
```

- [ ] **Step 10: `src/core/templates.hpp`**

```cpp
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
```

- [ ] **Step 11: `src/core/templates.cpp`**

```cpp
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
```

- [ ] **Step 12: Run — PASS** (`./cmake-build-debug/game_tests`).

- [ ] **Step 13: Commit**

```bash
git add src tests
git commit -m "feat: components, events, message log, world and entity templates"
```

---

### Task 3: Бой, движение, смерть, прогресс

**Files:**
- Create: `src/core/systems/systems.hpp`, `combat.cpp`, `movement.cpp`, `death.cpp`, `progression.cpp`; `tests/test_helpers.hpp`, `tests/test_combat.cpp`

**Interfaces:**
- Consumes: `World`, компоненты, `spawn`, `pickItemDef`, `Attacked/Died/LevelUp`.
- Produces (`systems.hpp`, будет дополняться в Tasks 4–5):
  `enum class MoveResult{Blocked,Moved,Attacked}`; `int effectiveAttack(const entt::registry&, entt::entity)`; `int effectiveDefense(...)`; `void attack(World&, entt::entity attacker, entt::entity target)`; `MoveResult tryMove(World&, entt::entity actor, int dx, int dy)`; `void deathSystem(World&)`; `int xpToNext(int level)`; `void progressionSystem(World&)`.
- Тест-хелперы: `std::unique_ptr<World> makeOpenWorld(int w=20,int h=10)` (пол внутри рамки стен, игрок в (5,5), seed 1); `template<class... C> int countWith(const entt::registry&)`; `entt::entity giveItem(World&, std::string_view id)`.

- [ ] **Step 1: `tests/test_helpers.hpp`**

```cpp
#pragma once

#include <memory>
#include <string_view>

#include "core/templates.hpp"
#include "core/world.hpp"

// Открытая комната w×h с рамкой из стен, игрок в (5,5).
inline std::unique_ptr<World> makeOpenWorld(int w = 20, int h = 10) {
    auto world = std::make_unique<World>(1u);
    world->map = Map(w, h);
    for (int y = 1; y < h - 1; ++y) {
        for (int x = 1; x < w - 1; ++x) world->map.setTile(x, y, Tile::Floor);
    }
    world->player = spawnPlayer(world->reg, Position{5, 5});
    return world;
}

template <typename... C>
int countWith(const entt::registry& reg) {
    int n = 0;
    for ([[maybe_unused]] auto e : reg.view<const C...>()) ++n;
    return n;
}

// Кладёт предмет сразу в рюкзак игрока (без Position, с InInventory).
inline entt::entity giveItem(World& w, std::string_view id) {
    const auto e = spawn(w.reg, id, w.reg.get<Position>(w.player));
    w.reg.remove<Position>(e);
    w.reg.emplace<InInventory>(e, InInventory{w.player});
    w.reg.get<Inventory>(w.player).items.push_back(e);
    return e;
}
```

- [ ] **Step 2: `tests/test_combat.cpp`**

```cpp
#include <catch2/catch_test_macros.hpp>

#include "core/messages.hpp"
#include "core/systems/systems.hpp"
#include "core/templates.hpp"
#include "test_helpers.hpp"

TEST_CASE("tryMove walks into a free cell", "[movement]") {
    auto w = makeOpenWorld();
    REQUIRE(tryMove(*w, w->player, 1, 0) == MoveResult::Moved);
    REQUIRE(w->reg.get<Position>(w->player) == Position{6, 5});
}

TEST_CASE("tryMove is blocked by walls, map edge and zero delta", "[movement]") {
    auto w = makeOpenWorld();
    w->map.setTile(6, 5, Tile::Wall);
    REQUIRE(tryMove(*w, w->player, 1, 0) == MoveResult::Blocked);
    REQUIRE(w->reg.get<Position>(w->player) == Position{5, 5});

    w->reg.get<Position>(w->player) = Position{1, 1};
    REQUIRE(tryMove(*w, w->player, -1, 0) == MoveResult::Blocked);
    REQUIRE(tryMove(*w, w->player, 0, -1) == MoveResult::Blocked);
    REQUIRE(tryMove(*w, w->player, 0, 0) == MoveResult::Blocked);
    REQUIRE(w->reg.get<Position>(w->player) == Position{1, 1});
}

TEST_CASE("moving into a monster attacks it", "[movement][combat]") {
    auto w = makeOpenWorld();
    MessageLog log;
    log.attach(w->events);
    const auto g = spawn(w->reg, "goblin", Position{6, 5});
    REQUIRE(tryMove(*w, w->player, 1, 0) == MoveResult::Attacked);
    REQUIRE(w->reg.get<Position>(w->player) == Position{5, 5});
    const int hp = w->reg.get<Health>(g).cur;
    REQUIRE(hp < 8);
    REQUIRE(hp >= 4);  // атака 4 - защита 1 = 3, плюс 0..1
    REQUIRE(log.recent(1)[0].rfind("You hit the Goblin for ", 0) == 0);
}

TEST_CASE("monsters block each other without fighting", "[movement]") {
    auto w = makeOpenWorld();
    const auto a = spawn(w->reg, "goblin", Position{8, 5});
    const auto b = spawn(w->reg, "goblin", Position{9, 5});
    REQUIRE(tryMove(*w, a, 1, 0) == MoveResult::Blocked);
    REQUIRE(w->reg.get<Health>(b).cur == w->reg.get<Health>(b).max);
}

TEST_CASE("damage is never below 1", "[combat]") {
    auto w = makeOpenWorld();
    const auto g = spawn(w->reg, "goblin", Position{6, 5});
    w->reg.get<Stats>(g).defense = 100;
    attack(*w, w->player, g);
    const int lost = 8 - w->reg.get<Health>(g).cur;
    REQUIRE(lost >= 1);
    REQUIRE(lost <= 2);
}

TEST_CASE("equipment adds to effective stats", "[combat]") {
    auto w = makeOpenWorld();
    REQUIRE(effectiveAttack(w->reg, w->player) == 4);
    REQUIRE(effectiveDefense(w->reg, w->player) == 1);
    auto& eq = w->reg.get<Equipment>(w->player);
    eq.weapon = spawn(w->reg, "dagger", Position{0, 0});
    eq.armor = spawn(w->reg, "leather_armor", Position{0, 0});
    REQUIRE(effectiveAttack(w->reg, w->player) == 6);
    REQUIRE(effectiveDefense(w->reg, w->player) == 2);
}

TEST_CASE("deathSystem removes dead monsters and awards xp", "[death]") {
    auto w = makeOpenWorld();
    MessageLog log;
    log.attach(w->events);
    const auto g = spawn(w->reg, "goblin", Position{6, 5});
    w->reg.get<Health>(g).cur = 0;
    deathSystem(*w);
    REQUIRE_FALSE(w->reg.valid(g));
    REQUIRE(w->reg.get<Level>(w->player).xp == 10);
    REQUIRE(log.recent(10).front() == "The Goblin dies.");
}

TEST_CASE("killing the boss sets bossKilled and drops no loot", "[death]") {
    auto w = makeOpenWorld();
    const auto d = spawn(w->reg, "dragon", Position{6, 5});
    w->reg.get<Health>(d).cur = 0;
    deathSystem(*w);
    REQUIRE(w->bossKilled);
    REQUIRE(countWith<Item>(w->reg) == 0);
}

TEST_CASE("deathSystem keeps a dead player and announces it", "[death]") {
    auto w = makeOpenWorld();
    MessageLog log;
    log.attach(w->events);
    w->reg.get<Health>(w->player).cur = 0;
    deathSystem(*w);
    REQUIRE(w->reg.valid(w->player));
    REQUIRE(log.recent(1)[0] == "You die...");
}

TEST_CASE("monsters sometimes drop loot", "[death]") {
    auto w = makeOpenWorld();
    for (int i = 0; i < 200; ++i) {
        const auto r = spawn(w->reg, "rat", Position{6, 5});
        w->reg.get<Health>(r).cur = 0;
        deathSystem(*w);
    }
    const int items = countWith<Item>(w->reg);
    REQUIRE(items > 20);
    REQUIRE(items < 120);
}

TEST_CASE("progression levels up and carries over xp", "[progression]") {
    auto w = makeOpenWorld();
    w->reg.get<Level>(w->player).xp = 65;
    progressionSystem(*w);
    const auto& lvl = w->reg.get<Level>(w->player);
    REQUIRE(lvl.lvl == 3);
    REQUIRE(lvl.xp == 5);
    REQUIRE(w->reg.get<Health>(w->player).max == 40);
    REQUIRE(w->reg.get<Stats>(w->player).attack == 6);
}

TEST_CASE("progression does nothing below the threshold", "[progression]") {
    auto w = makeOpenWorld();
    w->reg.get<Level>(w->player).xp = 19;
    progressionSystem(*w);
    REQUIRE(w->reg.get<Level>(w->player).lvl == 1);
    REQUIRE(xpToNext(1) == 20);
    REQUIRE(xpToNext(2) == 40);
}
```

- [ ] **Step 3: Run — FAIL** (нет `systems.hpp`).

- [ ] **Step 4: `src/core/systems/systems.hpp`**

```cpp
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
```

- [ ] **Step 5: `combat.cpp`**

```cpp
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
```

- [ ] **Step 6: `movement.cpp`**

```cpp
#include "core/systems/systems.hpp"

MoveResult tryMove(World& w, entt::entity actor, int dx, int dy) {
    if (dx == 0 && dy == 0) return MoveResult::Blocked;
    auto& reg = w.reg;
    auto& pos = reg.get<Position>(actor);
    const int nx = pos.x + dx;
    const int ny = pos.y + dy;
    if (!w.map.walkable(nx, ny)) return MoveResult::Blocked;

    for (auto e : reg.view<Position, BlocksMovement>()) {
        if (e == actor) continue;
        const auto& p = reg.get<Position>(e);
        if (p.x != nx || p.y != ny) continue;
        const auto* mine = reg.try_get<Faction>(actor);
        const auto* theirs = reg.try_get<Faction>(e);
        if (mine && theirs && mine->side != theirs->side && reg.all_of<Health>(e)) {
            attack(w, actor, e);
            return MoveResult::Attacked;
        }
        return MoveResult::Blocked;
    }
    pos.x = nx;
    pos.y = ny;
    return MoveResult::Moved;
}
```

- [ ] **Step 7: `death.cpp`**

```cpp
#include <string>
#include <vector>

#include "core/events.hpp"
#include "core/strings.hpp"
#include "core/systems/systems.hpp"
#include "core/templates.hpp"

namespace {
constexpr int kLootChancePercent = 30;

void dropLoot(World& w, Position pos) {
    if (!w.rng.chance(kLootChancePercent)) return;
    if (const auto* def = pickItemDef(w.rng, w.floor)) spawn(w.reg, def->id, pos);
}
}  // namespace

void deathSystem(World& w) {
    auto& reg = w.reg;
    std::vector<entt::entity> dead;
    for (auto e : reg.view<Health>()) {
        if (reg.get<Health>(e).cur <= 0) dead.push_back(e);
    }
    for (auto e : dead) {
        if (e == w.player) {
            w.events.trigger(Died{"You", true});
            continue;  // игрока не удаляем: Game переводит состояние в Dead
        }
        const std::string name = reg.get<Name>(e).value;
        const Position pos = reg.get<Position>(e);
        const bool boss = reg.all_of<AI>(e) && reg.get<AI>(e).kind == AIKind::Boss;
        if (const auto* xp = reg.try_get<XpReward>(e)) reg.get<Level>(w.player).xp += xp->amount;
        w.events.trigger(Died{name, false});
        reg.destroy(e);
        if (boss) {
            w.bossKilled = true;
        } else {
            dropLoot(w, pos);
        }
    }
}
```

- [ ] **Step 8: `progression.cpp`**

```cpp
#include <algorithm>

#include "core/events.hpp"
#include "core/systems/systems.hpp"

int xpToNext(int level) { return 20 * level; }

void progressionSystem(World& w) {
    auto& lvl = w.reg.get<Level>(w.player);
    auto& hp = w.reg.get<Health>(w.player);
    auto& st = w.reg.get<Stats>(w.player);
    while (lvl.xp >= xpToNext(lvl.lvl)) {
        lvl.xp -= xpToNext(lvl.lvl);
        ++lvl.lvl;
        hp.max += 5;
        hp.cur = std::min(hp.max, hp.cur + 5);
        st.attack += 1;
        w.events.trigger(LevelUp{lvl.lvl});
    }
}
```

- [ ] **Step 9: Run — PASS**

- [ ] **Step 10: Commit**

```bash
git add src tests
git commit -m "feat: combat, movement, death and progression systems"
```

---

### Task 4: Инвентарь (подбор, использование, экипировка, лестница)

**Files:**
- Create: `src/core/systems/inventory.cpp`, `tests/test_inventory.cpp`
- Modify: `src/core/systems/systems.hpp` (дописать объявления)

**Interfaces:**
- Consumes: Task 3 хелперы и системы.
- Produces: `bool pickupItems(World&)` (true если что-то подобрано), `bool useItem(World&, int index)` (true если потрачен ход), `bool isOnStairs(const World&)`.

- [ ] **Step 1: Дописать в `systems.hpp`**

```cpp
// inventory.cpp
bool pickupItems(World& w);       // true, если что-то подобрано
bool useItem(World& w, int index);  // true, если действие выполнено (ход потрачен)
bool isOnStairs(const World& w);
```

- [ ] **Step 2: `tests/test_inventory.cpp`**

```cpp
#include <catch2/catch_test_macros.hpp>

#include "core/messages.hpp"
#include "core/strings.hpp"
#include "core/systems/systems.hpp"
#include "core/templates.hpp"
#include "test_helpers.hpp"

TEST_CASE("pickup moves an item into the pack", "[inventory]") {
    auto w = makeOpenWorld();
    MessageLog log;
    log.attach(w->events);
    const auto dagger = spawn(w->reg, "dagger", Position{5, 5});
    REQUIRE(pickupItems(*w));
    const auto& inv = w->reg.get<Inventory>(w->player);
    REQUIRE(inv.items.size() == 1);
    REQUIRE(inv.items[0] == dagger);
    REQUIRE_FALSE(w->reg.all_of<Position>(dagger));
    REQUIRE(w->reg.all_of<InInventory>(dagger));
    REQUIRE(log.recent(1)[0] == "You pick up the Dagger.");
}

TEST_CASE("pickup collects gold into the purse", "[inventory]") {
    auto w = makeOpenWorld();
    const auto pile = spawnGold(w->reg, Position{5, 5}, 25);
    REQUIRE(pickupItems(*w));
    REQUIRE(w->reg.get<Gold>(w->player).amount == 25);
    REQUIRE_FALSE(w->reg.valid(pile));
}

TEST_CASE("pickup on an empty cell reports and costs nothing", "[inventory]") {
    auto w = makeOpenWorld();
    MessageLog log;
    log.attach(w->events);
    REQUIRE_FALSE(pickupItems(*w));
    REQUIRE(log.recent(1)[0] == str::kNothingHere);
}

TEST_CASE("pickup with full pack leaves the item on the floor", "[inventory][focus]") {
    auto w = makeOpenWorld();
    MessageLog log;
    log.attach(w->events);
    for (std::size_t i = 0; i < kMaxInventory; ++i) giveItem(*w, "dagger");
    const auto potion = spawn(w->reg, "health_potion", Position{5, 5});
    REQUIRE_FALSE(pickupItems(*w));
    REQUIRE(w->reg.get<Inventory>(w->player).items.size() == kMaxInventory);
    REQUIRE(w->reg.all_of<Position>(potion));
    REQUIRE(log.recent(1)[0] == str::kPackFull);
}

TEST_CASE("a potion heals and is consumed", "[inventory]") {
    auto w = makeOpenWorld();
    MessageLog log;
    log.attach(w->events);
    auto& hp = w->reg.get<Health>(w->player);
    hp.cur = 10;
    const auto potion = giveItem(*w, "health_potion");
    REQUIRE(useItem(*w, 0));
    REQUIRE(hp.cur == 20);
    REQUIRE_FALSE(w->reg.valid(potion));
    REQUIRE(w->reg.get<Inventory>(w->player).items.empty());
    REQUIRE(log.recent(1)[0] == "You heal 10 HP.");
}

TEST_CASE("healing is capped at max health", "[inventory]") {
    auto w = makeOpenWorld();
    MessageLog log;
    log.attach(w->events);
    w->reg.get<Health>(w->player).cur = 25;
    giveItem(*w, "health_potion");
    REQUIRE(useItem(*w, 0));
    REQUIRE(w->reg.get<Health>(w->player).cur == 30);
    REQUIRE(log.recent(1)[0] == "You heal 5 HP.");
}

TEST_CASE("potion at full health is not wasted", "[inventory][focus]") {
    auto w = makeOpenWorld();
    MessageLog log;
    log.attach(w->events);
    const auto potion = giveItem(*w, "health_potion");
    REQUIRE_FALSE(useItem(*w, 0));
    REQUIRE(w->reg.valid(potion));
    REQUIRE(w->reg.get<Inventory>(w->player).items.size() == 1);
    REQUIRE(log.recent(1)[0] == str::kFullHealth);
}

TEST_CASE("useItem rejects bad index", "[inventory][focus]") {
    auto w = makeOpenWorld();
    MessageLog log;
    log.attach(w->events);
    REQUIRE_FALSE(useItem(*w, 0));    // пустой рюкзак
    REQUIRE_FALSE(useItem(*w, -1));
    giveItem(*w, "dagger");
    REQUIRE_FALSE(useItem(*w, 1));
    REQUIRE_FALSE(useItem(*w, 8));
    REQUIRE(w->reg.get<Inventory>(w->player).items.size() == 1);
    REQUIRE(log.recent(1)[0] == str::kNoSuchItem);
}

TEST_CASE("equipping swaps with the item already worn", "[inventory]") {
    auto w = makeOpenWorld();
    const auto dagger = giveItem(*w, "dagger");
    const auto sword = giveItem(*w, "sword");
    REQUIRE(useItem(*w, 0));
    const auto& eq = w->reg.get<Equipment>(w->player);
    REQUIRE(eq.weapon == dagger);
    REQUIRE(w->reg.get<Inventory>(w->player).items == std::vector<entt::entity>{sword});
    REQUIRE(useItem(*w, 0));
    REQUIRE(eq.weapon == sword);
    REQUIRE(w->reg.get<Inventory>(w->player).items == std::vector<entt::entity>{dagger});
    REQUIRE_FALSE(w->reg.all_of<Position>(dagger));
    REQUIRE_FALSE(w->reg.all_of<Position>(sword));
}

TEST_CASE("armor goes to the armor slot", "[inventory]") {
    auto w = makeOpenWorld();
    const auto mail = giveItem(*w, "chain_mail");
    REQUIRE(useItem(*w, 0));
    REQUIRE(w->reg.get<Equipment>(w->player).armor == mail);
    REQUIRE(w->reg.get<Equipment>(w->player).weapon == entt::null);
}

TEST_CASE("isOnStairs", "[inventory]") {
    auto w = makeOpenWorld();
    REQUIRE_FALSE(isOnStairs(*w));
    spawnStairs(w->reg, Position{6, 5});
    REQUIRE_FALSE(isOnStairs(*w));
    spawnStairs(w->reg, Position{5, 5});
    REQUIRE(isOnStairs(*w));
}
```

- [ ] **Step 3: Run — FAIL** (undefined reference `pickupItems` и др.).

- [ ] **Step 4: `inventory.cpp`**

```cpp
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
```

- [ ] **Step 5: Run — PASS**

- [ ] **Step 6: Commit**

```bash
git add src tests
git commit -m "feat: inventory system (pickup, use, equip, stairs check)"
```

---

### Task 5: Поле зрения и ИИ

**Files:**
- Create: `src/core/systems/fov.cpp`, `src/core/systems/ai.cpp`, `tests/test_fov_ai.cpp`
- Modify: `src/core/systems/systems.hpp`

**Interfaces:**
- Produces: `void fovSystem(World&)`, `void aiSystem(World&)`.
- Поведение ИИ: `Wander` — случайный шаг 50%, атакует соседа; `Chase` — преследует, если клетка монстра видима и манхэттен ≤ 12; `Boss` — преследует при манхэттене ≤ 15 без учёта видимости; шаги по BFS (4 направления); при мёртвом игроке монстры не действуют.

- [ ] **Step 1: Дописать в `systems.hpp`**

```cpp
// fov.cpp
void fovSystem(World& w);

// ai.cpp
void aiSystem(World& w);
```

- [ ] **Step 2: `tests/test_fov_ai.cpp`**

```cpp
#include <catch2/catch_test_macros.hpp>

#include "core/systems/systems.hpp"
#include "core/templates.hpp"
#include "test_helpers.hpp"

namespace {
void wallColumn(World& w, int x) {
    for (int y = 1; y < w.map.height() - 1; ++y) w.map.setTile(x, y, Tile::Wall);
}
}  // namespace

TEST_CASE("fov sees open cells in radius and marks the player cell", "[fov]") {
    auto w = makeOpenWorld();
    fovSystem(*w);
    REQUIRE(w->map.visible(5, 5));
    REQUIRE(w->map.visible(5, 8));
    REQUIRE(w->map.visible(9, 5));
}

TEST_CASE("fov respects radius", "[fov]") {
    auto w = makeOpenWorld();
    w->reg.get<Viewshed>(w->player).radius = 3;
    fovSystem(*w);
    REQUIRE(w->map.visible(7, 5));
    REQUIRE_FALSE(w->map.visible(10, 5));
}

TEST_CASE("walls are lit but block what is behind them", "[fov]") {
    auto w = makeOpenWorld();
    wallColumn(*w, 7);
    fovSystem(*w);
    REQUIRE(w->map.visible(7, 5));
    REQUIRE_FALSE(w->map.visible(9, 5));
}

TEST_CASE("explored cells stay explored after moving away", "[fov]") {
    auto w = makeOpenWorld();
    fovSystem(*w);
    REQUIRE(w->map.visible(9, 5));
    w->reg.get<Position>(w->player) = Position{1, 1};
    w->reg.get<Viewshed>(w->player).radius = 2;
    fovSystem(*w);
    REQUIRE_FALSE(w->map.visible(9, 5));
    REQUIRE(w->map.explored(9, 5));
}

TEST_CASE("an adjacent goblin attacks the player", "[ai]") {
    auto w = makeOpenWorld();
    spawn(w->reg, "goblin", Position{6, 5});
    fovSystem(*w);
    aiSystem(*w);
    const int hp = w->reg.get<Health>(w->player).cur;
    REQUIRE(hp < 30);
    REQUIRE(hp >= 27);  // 3 - 1 = 2, плюс 0..1
}

TEST_CASE("a goblin that sees the player steps closer", "[ai]") {
    auto w = makeOpenWorld();
    const auto g = spawn(w->reg, "goblin", Position{10, 5});
    fovSystem(*w);
    aiSystem(*w);
    REQUIRE(w->reg.get<Position>(g) == Position{9, 5});
}

TEST_CASE("a goblin behind a wall does not move", "[ai]") {
    auto w = makeOpenWorld();
    wallColumn(*w, 7);
    const auto g = spawn(w->reg, "goblin", Position{10, 5});
    fovSystem(*w);
    aiSystem(*w);
    REQUIRE(w->reg.get<Position>(g) == Position{10, 5});
}

TEST_CASE("the boss hunts without line of sight, but copes with no path", "[ai]") {
    {
        auto w = makeOpenWorld();
        const auto d = spawn(w->reg, "dragon", Position{10, 5});
        aiSystem(*w);  // fov не считали: видимости нет
        REQUIRE(w->reg.get<Position>(d) == Position{9, 5});
    }
    {
        auto w = makeOpenWorld();
        wallColumn(*w, 7);
        const auto d = spawn(w->reg, "dragon", Position{10, 5});
        aiSystem(*w);
        REQUIRE(w->reg.get<Position>(d) == Position{10, 5});
    }
}

TEST_CASE("wandering rats stay on walkable cells", "[ai]") {
    auto w = makeOpenWorld();
    w->reg.get<Health>(w->player) = Health{1000, 1000};
    const auto r = spawn(w->reg, "rat", Position{15, 5});
    for (int i = 0; i < 100; ++i) {
        fovSystem(*w);
        aiSystem(*w);
        const auto& p = w->reg.get<Position>(r);
        REQUIRE(w->map.walkable(p.x, p.y));
    }
}

TEST_CASE("monsters do nothing once the player is dead", "[ai]") {
    auto w = makeOpenWorld();
    const auto g = spawn(w->reg, "goblin", Position{6, 5});
    w->reg.get<Health>(w->player).cur = 0;
    fovSystem(*w);
    aiSystem(*w);
    REQUIRE(w->reg.get<Health>(w->player).cur == 0);
    REQUIRE(w->reg.get<Position>(g) == Position{6, 5});
}
```

- [ ] **Step 3: Run — FAIL** (undefined `fovSystem`/`aiSystem`).

- [ ] **Step 4: `fov.cpp`** (рекурсивный shadowcasting по 8 октантам)

```cpp
#include "core/systems/systems.hpp"

namespace {

constexpr int kXX[8] = {1, 0, 0, -1, -1, 0, 0, 1};
constexpr int kXY[8] = {0, 1, -1, 0, 0, -1, 1, 0};
constexpr int kYX[8] = {0, 1, 1, 0, 0, -1, -1, 0};
constexpr int kYY[8] = {1, 0, 0, 1, -1, 0, 0, -1};

void castLight(Map& map, int cx, int cy, int radius, int row, float start, float end, int xx,
               int xy, int yx, int yy) {
    if (start < end) return;
    float newStart = 0.0f;
    for (int j = row; j <= radius; ++j) {
        int dx = -j - 1;
        const int dy = -j;
        bool blocked = false;
        while (dx <= 0) {
            ++dx;
            const int mx = cx + dx * xx + dy * xy;
            const int my = cy + dx * yx + dy * yy;
            const float lSlope = (static_cast<float>(dx) - 0.5f) / (static_cast<float>(dy) + 0.5f);
            const float rSlope = (static_cast<float>(dx) + 0.5f) / (static_cast<float>(dy) - 0.5f);
            if (start < rSlope) continue;
            if (end > lSlope) break;
            if (dx * dx + dy * dy <= radius * radius) map.setVisible(mx, my, true);
            if (blocked) {
                if (map.opaque(mx, my)) {
                    newStart = rSlope;
                    continue;
                }
                blocked = false;
                start = newStart;
            } else if (map.opaque(mx, my) && j < radius) {
                blocked = true;
                castLight(map, cx, cy, radius, j + 1, start, lSlope, xx, xy, yx, yy);
                newStart = rSlope;
            }
        }
        if (blocked) break;
    }
}

}  // namespace

void fovSystem(World& w) {
    w.map.clearVisible();
    const Position pos = w.reg.get<Position>(w.player);
    const int radius = w.reg.get<Viewshed>(w.player).radius;
    w.map.setVisible(pos.x, pos.y, true);
    for (int oct = 0; oct < 8; ++oct) {
        castLight(w.map, pos.x, pos.y, radius, 1, 1.0f, 0.0f, kXX[oct], kXY[oct], kYX[oct],
                  kYY[oct]);
    }
}
```

- [ ] **Step 5: `ai.cpp`**

```cpp
#include <array>
#include <cstdlib>
#include <optional>
#include <queue>
#include <utility>
#include <vector>

#include "core/systems/systems.hpp"

namespace {

constexpr int kChaseRange = 12;
constexpr int kBossRange = 15;
constexpr std::array<std::pair<int, int>, 4> kDirs{{{1, 0}, {-1, 0}, {0, 1}, {0, -1}}};

// BFS по клеткам; блокируют только BlocksMovement-сущности (цель — исключение).
std::optional<std::pair<int, int>> stepToward(World& w, Position from, Position to, int maxDepth) {
    const Map& m = w.map;
    const int W = m.width();
    const int H = m.height();
    const auto size = static_cast<std::size_t>(W * H);
    std::vector<char> blocked(size, 0);
    for (auto e : w.reg.view<Position, BlocksMovement>()) {
        const auto& p = w.reg.get<Position>(e);
        if (m.inBounds(p.x, p.y)) blocked[static_cast<std::size_t>(p.y * W + p.x)] = 1;
    }
    const int start = from.y * W + from.x;
    const int goal = to.y * W + to.x;
    blocked[static_cast<std::size_t>(goal)] = 0;

    std::vector<int> parent(size, -1);
    std::vector<int> depth(size, 0);
    std::queue<int> q;
    parent[static_cast<std::size_t>(start)] = start;
    q.push(start);
    while (!q.empty()) {
        const int cur = q.front();
        q.pop();
        if (cur == goal) break;
        if (depth[static_cast<std::size_t>(cur)] >= maxDepth) continue;
        const int cx = cur % W;
        const int cy = cur / W;
        for (const auto& [dx, dy] : kDirs) {
            const int nx = cx + dx;
            const int ny = cy + dy;
            if (!m.walkable(nx, ny)) continue;
            const auto ni = static_cast<std::size_t>(ny * W + nx);
            if (blocked[ni] || parent[ni] != -1) continue;
            parent[ni] = cur;
            depth[ni] = depth[static_cast<std::size_t>(cur)] + 1;
            q.push(static_cast<int>(ni));
        }
    }
    if (parent[static_cast<std::size_t>(goal)] == -1) return std::nullopt;
    int cur = goal;
    while (parent[static_cast<std::size_t>(cur)] != start) cur = parent[static_cast<std::size_t>(cur)];
    return std::make_pair(cur % W - from.x, cur / W - from.y);
}

}  // namespace

void aiSystem(World& w) {
    auto& reg = w.reg;
    const Position target = reg.get<Position>(w.player);

    std::vector<entt::entity> actors;
    for (auto e : reg.view<AI, Position>()) actors.push_back(e);

    for (auto e : actors) {
        if (reg.get<Health>(w.player).cur <= 0) break;
        const AIKind kind = reg.get<AI>(e).kind;
        const Position pos = reg.get<Position>(e);
        const int dist = std::abs(pos.x - target.x) + std::abs(pos.y - target.y);

        bool aware = false;
        switch (kind) {
            case AIKind::Boss: aware = dist <= kBossRange; break;
            case AIKind::Chase: aware = w.map.visible(pos.x, pos.y) && dist <= kChaseRange; break;
            case AIKind::Wander: aware = dist == 1; break;
        }
        if (aware) {
            if (const auto step = stepToward(w, pos, target, kBossRange * 2)) {
                tryMove(w, e, step->first, step->second);
                continue;
            }
        }
        if (kind == AIKind::Wander && w.rng.chance(50)) {
            const auto& [dx, dy] = kDirs[static_cast<std::size_t>(w.rng.range(0, 3))];
            tryMove(w, e, dx, dy);
        }
    }
}
```

- [ ] **Step 6: Run — PASS**

- [ ] **Step 7: Commit**

```bash
git add src tests
git commit -m "feat: field of view (shadowcasting) and monster AI"
```

---

### Task 6: Генерация этажа

**Files:**
- Create: `src/core/generator.hpp`, `src/core/generator.cpp`, `tests/test_generator.cpp`

**Interfaces:**
- Consumes: `World`, `spawn*`, `pickMonsterDef`, `pickItemDef`, `kMapWidth/Height`, `kFinalFloor`.
- Produces: `struct Room{int x,y,w,h; Position center() const;}`; `struct Layout{Map map; std::vector<Room> rooms;}`; `Layout generateLayout(Rng&)`; `void buildFloor(World&, int floor)` — удаляет всё с `Position` кроме игрока, генерирует карту, создаёт игрока при первом вызове (`world.player == entt::null`) или переносит его на старт, ставит лестницу (этажи < 5) или босса (этаж 5), расставляет врагов/предметы/золото, выставляет `world.floor`.

- [ ] **Step 1: `tests/test_generator.cpp`**

```cpp
#include <catch2/catch_test_macros.hpp>
#include <queue>
#include <set>
#include <utility>

#include "core/generator.hpp"
#include "core/systems/systems.hpp"
#include "test_helpers.hpp"

namespace {

// Достижимые walkable-клетки от start (4 направления).
std::set<std::pair<int, int>> flood(const Map& m, Position start) {
    std::set<std::pair<int, int>> seen;
    std::queue<std::pair<int, int>> q;
    seen.insert({start.x, start.y});
    q.push({start.x, start.y});
    while (!q.empty()) {
        const auto [x, y] = q.front();
        q.pop();
        for (const auto& [dx, dy] : {std::pair{1, 0}, std::pair{-1, 0}, std::pair{0, 1}, std::pair{0, -1}}) {
            const int nx = x + dx;
            const int ny = y + dy;
            if (!m.walkable(nx, ny) || seen.count({nx, ny})) continue;
            seen.insert({nx, ny});
            q.push({nx, ny});
        }
    }
    return seen;
}

}  // namespace

TEST_CASE("layouts are connected and in bounds for many seeds", "[generator]") {
    for (std::uint32_t seed = 1; seed <= 100; ++seed) {
        Rng rng(seed);
        const Layout layout = generateLayout(rng);
        REQUIRE(layout.rooms.size() >= 2);
        REQUIRE(layout.rooms.size() <= 9);
        const auto reach = flood(layout.map, layout.rooms.front().center());
        for (const auto& r : layout.rooms) {
            REQUIRE(r.x >= 1);
            REQUIRE(r.y >= 1);
            REQUIRE(r.x + r.w <= kMapWidth - 1);
            REQUIRE(r.y + r.h <= kMapHeight - 1);
            REQUIRE(reach.count({r.center().x, r.center().y}) == 1);
        }
    }
}

TEST_CASE("the same seed gives the same layout", "[generator]") {
    Rng a(5), b(5);
    const Layout la = generateLayout(a);
    const Layout lb = generateLayout(b);
    REQUIRE(la.rooms.size() == lb.rooms.size());
    for (int y = 0; y < kMapHeight; ++y) {
        for (int x = 0; x < kMapWidth; ++x) REQUIRE(la.map.tile(x, y) == lb.map.tile(x, y));
    }
}

TEST_CASE("buildFloor places player and stairs on floors 1-4", "[generator]") {
    for (int floor = 1; floor <= 4; ++floor) {
        World w(static_cast<std::uint32_t>(floor));
        buildFloor(w, floor);
        REQUIRE(w.floor == floor);
        REQUIRE(w.reg.valid(w.player));
        const Position start = w.reg.get<Position>(w.player);
        REQUIRE(w.map.walkable(start.x, start.y));
        REQUIRE(countWith<Stairs>(w.reg) == 1);
        const auto reach = flood(w.map, start);
        for (auto e : w.reg.view<const Position, const Stairs>()) {
            const auto& p = w.reg.get<Position>(e);
            REQUIRE(reach.count({p.x, p.y}) == 1);
        }
        REQUIRE(countWith<AI>(w.reg) >= 1);
    }
}

TEST_CASE("floor 5 has the boss and no stairs", "[generator]") {
    World w(9);
    buildFloor(w, kFinalFloor);
    REQUIRE(countWith<Stairs>(w.reg) == 0);
    int bosses = 0;
    for (auto e : w.reg.view<AI>()) {
        if (w.reg.get<AI>(e).kind == AIKind::Boss) {
            ++bosses;
            const auto& p = w.reg.get<Position>(e);
            REQUIRE(flood(w.map, w.reg.get<Position>(w.player)).count({p.x, p.y}) == 1);
        }
    }
    REQUIRE(bosses == 1);
}

TEST_CASE("everything stands on walkable cells and blockers never share a cell", "[generator]") {
    for (std::uint32_t seed = 1; seed <= 30; ++seed) {
        World w(seed);
        buildFloor(w, 3);
        std::set<std::pair<int, int>> blockers;
        for (auto e : w.reg.view<Position>()) {
            const auto& p = w.reg.get<Position>(e);
            REQUIRE(w.map.walkable(p.x, p.y));
            if (w.reg.all_of<BlocksMovement>(e)) {
                REQUIRE(blockers.insert({p.x, p.y}).second);
            }
        }
    }
}

TEST_CASE("buildFloor keeps the player and pack, drops the old floor", "[generator]") {
    World w(3);
    buildFloor(w, 1);
    const auto player = w.player;
    const auto dagger = giveItem(w, "dagger");
    std::vector<entt::entity> oldStuff;
    for (auto e : w.reg.view<Position>()) {
        if (e != player) oldStuff.push_back(e);
    }
    REQUIRE_FALSE(oldStuff.empty());

    buildFloor(w, 2);
    REQUIRE(w.player == player);
    REQUIRE(w.floor == 2);
    REQUIRE(w.reg.valid(dagger));
    REQUIRE_FALSE(w.reg.all_of<Position>(dagger));
    REQUIRE(w.reg.get<Inventory>(player).items.size() == 1);
    for (auto e : oldStuff) REQUIRE_FALSE(w.reg.valid(e));
}
```

- [ ] **Step 2: Run — FAIL** (нет `generator.hpp`).

- [ ] **Step 3: `src/core/generator.hpp`**

```cpp
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
```

- [ ] **Step 4: `src/core/generator.cpp`**

```cpp
#include "core/generator.hpp"

#include <algorithm>
#include <optional>

#include "core/templates.hpp"

namespace {

bool intersects(const Room& a, const Room& b) {
    return a.x - 1 < b.x + b.w && a.x + a.w + 1 > b.x && a.y - 1 < b.y + b.h && a.y + a.h + 1 > b.y;
}

void carveRoom(Map& m, const Room& r) {
    for (int y = r.y; y < r.y + r.h; ++y) {
        for (int x = r.x; x < r.x + r.w; ++x) m.setTile(x, y, Tile::Floor);
    }
}

void carveCorridor(Map& m, Rng& rng, Position a, Position b) {
    const auto hline = [&](int x1, int x2, int y) {
        for (int x = std::min(x1, x2); x <= std::max(x1, x2); ++x) m.setTile(x, y, Tile::Floor);
    };
    const auto vline = [&](int y1, int y2, int x) {
        for (int y = std::min(y1, y2); y <= std::max(y1, y2); ++y) m.setTile(x, y, Tile::Floor);
    };
    if (rng.chance(50)) {
        hline(a.x, b.x, a.y);
        vline(a.y, b.y, b.x);
    } else {
        vline(a.y, b.y, a.x);
        hline(a.x, b.x, b.y);
    }
}

}  // namespace

Layout generateLayout(Rng& rng) {
    for (;;) {
        Layout layout{Map(kMapWidth, kMapHeight), {}};
        const int target = rng.range(6, 9);
        for (int attempt = 0; attempt < 200 && static_cast<int>(layout.rooms.size()) < target;
             ++attempt) {
            Room r{0, 0, rng.range(4, 10), rng.range(3, 6)};
            r.x = rng.range(1, kMapWidth - r.w - 2);
            r.y = rng.range(1, kMapHeight - r.h - 2);
            const bool overlaps = std::any_of(layout.rooms.begin(), layout.rooms.end(),
                                              [&](const Room& o) { return intersects(r, o); });
            if (overlaps) continue;
            carveRoom(layout.map, r);
            layout.rooms.push_back(r);
        }
        if (layout.rooms.size() < 2) continue;
        for (std::size_t i = 1; i < layout.rooms.size(); ++i) {
            carveCorridor(layout.map, rng, layout.rooms[i - 1].center(), layout.rooms[i].center());
        }
        return layout;
    }
}

void buildFloor(World& w, int floor) {
    auto& reg = w.reg;

    std::vector<entt::entity> old;
    for (auto e : reg.view<Position>()) {
        if (e != w.player) old.push_back(e);
    }
    reg.destroy(old.begin(), old.end());

    Layout layout = generateLayout(w.rng);
    w.map = std::move(layout.map);
    w.floor = floor;

    const Position start = layout.rooms.front().center();
    if (w.player == entt::null) {
        w.player = spawnPlayer(reg, start);
    } else {
        reg.get<Position>(w.player) = start;
    }

    std::vector<Position> used{start};
    const auto freeCell = [&](const Room& r) -> std::optional<Position> {
        for (int tries = 0; tries < 30; ++tries) {
            const Position p{w.rng.range(r.x, r.x + r.w - 1), w.rng.range(r.y, r.y + r.h - 1)};
            if (std::find(used.begin(), used.end(), p) != used.end()) continue;
            used.push_back(p);
            return p;
        }
        return std::nullopt;
    };

    const Room& last = layout.rooms.back();
    used.push_back(last.center());
    if (floor == kFinalFloor) {
        spawn(reg, "dragon", last.center());
    } else {
        spawnStairs(reg, last.center());
    }

    for (std::size_t i = 1; i < layout.rooms.size(); ++i) {
        const Room& r = layout.rooms[i];
        const int monsters = w.rng.range(1, 2 + floor / 3);
        for (int m = 0; m < monsters; ++m) {
            const auto p = freeCell(r);
            const auto* def = pickMonsterDef(w.rng, floor);
            if (p && def) spawn(reg, def->id, *p);
        }
        if (w.rng.chance(45)) {
            const auto p = freeCell(r);
            const auto* def = pickItemDef(w.rng, floor);
            if (p && def) spawn(reg, def->id, *p);
        }
        if (w.rng.chance(50)) {
            if (const auto p = freeCell(r)) spawnGold(reg, *p, w.rng.range(5, 15) * floor);
        }
    }
}
```

- [ ] **Step 5: Run — PASS**

- [ ] **Step 6: Commit**

```bash
git add src tests
git commit -m "feat: procedural floor generation"
```

---

### Task 7: Game (команды, состояния, ход)

**Files:**
- Create: `src/core/command.hpp`, `src/core/game.hpp`, `src/core/game.cpp`, `tests/game_helpers.hpp`, `tests/test_game.cpp`

**Interfaces:**
- Consumes: всё из Tasks 1–6.
- Produces:
  `enum class CommandType{None,Move,Pickup,Descend,OpenInventory,CloseInventory,UseItem,Wait,Restart,Quit}`; `struct Command{CommandType type=None; int dx=0, dy=0, index=0;}`;
  `enum class GameState{Playing,Inventory,Dead,Won}`;
  `class Game{ explicit Game(uint32_t seed); bool tick(const Command&); GameState state() const; World& world(); const World& world() const; const MessageLog& log() const; uint32_t seed() const; }` (копирование запрещено).
- `tick` возвращает true, если мир сделал ход. `Quit` и `Restart` обрабатывает `main` (создаёт новый `Game`), `Game` их игнорирует.
- Порядок хода (`endTurn`): `deathSystem` → `progressionSystem` → (если `bossKilled`: Won) → `fovSystem` → `aiSystem` → `deathSystem` → (если HP игрока ≤ 0: Dead).

- [ ] **Step 1: `tests/game_helpers.hpp`**

```cpp
#pragma once

#include <vector>

#include "core/game.hpp"

// Заменяет сгенерированный этаж открытой комнатой, игрок в (5,5).
inline World& prepareOpenFloor(Game& g, int w = 20, int h = 10) {
    World& world = g.world();
    std::vector<entt::entity> old;
    for (auto e : world.reg.view<Position>()) {
        if (e != world.player) old.push_back(e);
    }
    world.reg.destroy(old.begin(), old.end());
    world.map = Map(w, h);
    for (int y = 1; y < h - 1; ++y) {
        for (int x = 1; x < w - 1; ++x) world.map.setTile(x, y, Tile::Floor);
    }
    world.reg.get<Position>(world.player) = Position{5, 5};
    return world;
}
```

- [ ] **Step 2: `tests/test_game.cpp`**

```cpp
#include <catch2/catch_test_macros.hpp>

#include "core/game.hpp"
#include "core/strings.hpp"
#include "core/templates.hpp"
#include "game_helpers.hpp"
#include "test_helpers.hpp"

namespace {
Command move(int dx, int dy) { return Command{CommandType::Move, dx, dy}; }
Command cmd(CommandType t) { return Command{t}; }
Command use(int index) { return Command{CommandType::UseItem, 0, 0, index}; }
}  // namespace

TEST_CASE("a new game starts on floor 1 with a living hero", "[game]") {
    Game g(11);
    REQUIRE(g.state() == GameState::Playing);
    REQUIRE(g.world().floor == 1);
    REQUIRE(g.world().reg.get<Health>(g.world().player).cur == 30);
    REQUIRE(g.log().recent(1)[0] == str::kEnterDungeon);
}

TEST_CASE("the same seed gives the same first floor", "[game]") {
    Game a(77), b(77);
    REQUIRE(a.world().reg.get<Position>(a.world().player) ==
            b.world().reg.get<Position>(b.world().player));
    for (int y = 0; y < kMapHeight; ++y) {
        for (int x = 0; x < kMapWidth; ++x) {
            REQUIRE(a.world().map.tile(x, y) == b.world().map.tile(x, y));
        }
    }
}

TEST_CASE("moving into a wall costs no turn", "[game]") {
    Game g(1);
    auto& w = prepareOpenFloor(g);
    const auto goblin = spawn(w.reg, "goblin", Position{12, 5});
    w.map.setTile(6, 5, Tile::Wall);
    REQUIRE_FALSE(g.tick(move(1, 0)));
    REQUIRE(w.reg.get<Position>(w.player) == Position{5, 5});
    REQUIRE(w.reg.get<Position>(goblin) == Position{12, 5});
}

TEST_CASE("a free move is a turn and monsters react", "[game]") {
    Game g(1);
    auto& w = prepareOpenFloor(g);
    const auto goblin = spawn(w.reg, "goblin", Position{12, 5});
    REQUIRE(g.tick(move(1, 0)));
    REQUIRE(w.reg.get<Position>(w.player) == Position{6, 5});
    REQUIRE(w.reg.get<Position>(goblin) == Position{11, 5});
}

TEST_CASE("waiting lets an adjacent monster hit", "[game]") {
    Game g(1);
    auto& w = prepareOpenFloor(g);
    spawn(w.reg, "goblin", Position{6, 5});
    REQUIRE(g.tick(cmd(CommandType::Wait)));
    REQUIRE(w.reg.get<Health>(w.player).cur < 30);
}

TEST_CASE("killing a monster removes it and awards xp", "[game]") {
    Game g(1);
    auto& w = prepareOpenFloor(g);
    const auto rat = spawn(w.reg, "rat", Position{6, 5});
    REQUIRE(g.tick(move(1, 0)));
    REQUIRE_FALSE(w.reg.valid(rat));
    REQUIRE(w.reg.get<Level>(w.player).xp == 4);
    const auto lines = g.log().recent(5);
    bool died = false;
    for (const auto& l : lines) died = died || l == "The Rat dies.";
    REQUIRE(died);
}

TEST_CASE("pickup is a turn only when something is taken", "[game]") {
    Game g(1);
    auto& w = prepareOpenFloor(g);
    REQUIRE_FALSE(g.tick(cmd(CommandType::Pickup)));
    spawn(w.reg, "dagger", Position{5, 5});
    REQUIRE(g.tick(cmd(CommandType::Pickup)));
    REQUIRE(w.reg.get<Inventory>(w.player).items.size() == 1);
}

TEST_CASE("descend off stairs does nothing", "[game][focus]") {
    Game g(1);
    auto& w = prepareOpenFloor(g);
    REQUIRE_FALSE(g.tick(cmd(CommandType::Descend)));
    REQUIRE(w.floor == 1);
    REQUIRE(g.log().recent(1)[0] == str::kNoStairs);
}

TEST_CASE("descend on stairs builds the next floor", "[game]") {
    Game g(1);
    auto& w = prepareOpenFloor(g);
    spawnStairs(w.reg, Position{5, 5});
    giveItem(w, "dagger");
    REQUIRE(g.tick(cmd(CommandType::Descend)));
    REQUIRE(w.floor == 2);
    REQUIRE(g.state() == GameState::Playing);
    REQUIRE(w.reg.get<Inventory>(w.player).items.size() == 1);
    REQUIRE(g.log().recent(1)[0] == "You descend to floor 2.");
}

TEST_CASE("inventory flow: open, use, close", "[game]") {
    Game g(1);
    auto& w = prepareOpenFloor(g);
    w.reg.get<Health>(w.player).cur = 10;
    giveItem(w, "health_potion");

    REQUIRE_FALSE(g.tick(cmd(CommandType::OpenInventory)));
    REQUIRE(g.state() == GameState::Inventory);
    REQUIRE_FALSE(g.tick(move(1, 0)));  // движение в меню игнорируется
    REQUIRE(w.reg.get<Position>(w.player) == Position{5, 5});

    REQUIRE(g.tick(use(0)));
    REQUIRE(g.state() == GameState::Playing);
    REQUIRE(w.reg.get<Health>(w.player).cur == 20);

    REQUIRE_FALSE(g.tick(cmd(CommandType::OpenInventory)));
    REQUIRE_FALSE(g.tick(cmd(CommandType::CloseInventory)));
    REQUIRE(g.state() == GameState::Playing);
}

TEST_CASE("bad inventory index keeps the menu open and costs no turn", "[game][focus]") {
    Game g(1);
    auto& w = prepareOpenFloor(g);
    spawn(w.reg, "goblin", Position{12, 5});
    giveItem(w, "dagger");
    g.tick(cmd(CommandType::OpenInventory));
    REQUIRE_FALSE(g.tick(use(8)));
    REQUIRE(g.state() == GameState::Inventory);
    REQUIRE_FALSE(g.tick(use(-1)));
    REQUIRE(g.state() == GameState::Inventory);
    REQUIRE(w.reg.get<Inventory>(w.player).items.size() == 1);
}

TEST_CASE("commands after death are ignored", "[game][focus]") {
    Game g(1);
    auto& w = prepareOpenFloor(g);
    const auto goblin = spawn(w.reg, "goblin", Position{6, 5});
    w.reg.get<Health>(w.player).cur = 1;
    REQUIRE(g.tick(cmd(CommandType::Wait)));
    REQUIRE(g.state() == GameState::Dead);
    REQUIRE(g.log().recent(1)[0] == str::kYouDie);

    const Position goblinPos = w.reg.get<Position>(goblin);
    const Position playerPos = w.reg.get<Position>(w.player);
    REQUIRE_FALSE(g.tick(move(0, 1)));
    REQUIRE_FALSE(g.tick(cmd(CommandType::Wait)));
    REQUIRE_FALSE(g.tick(cmd(CommandType::OpenInventory)));
    REQUIRE(g.state() == GameState::Dead);
    REQUIRE(w.reg.get<Position>(goblin) == goblinPos);
    REQUIRE(w.reg.get<Position>(w.player) == playerPos);
}

TEST_CASE("commands after victory are ignored", "[game][focus]") {
    Game g(1);
    auto& w = prepareOpenFloor(g);
    const auto dragon = spawn(w.reg, "dragon", Position{6, 5});
    w.reg.get<Health>(dragon).cur = 1;
    REQUIRE(g.tick(move(1, 0)));
    REQUIRE(g.state() == GameState::Won);
    REQUIRE(g.log().recent(1)[0] == str::kVictory);
    REQUIRE_FALSE(g.tick(move(1, 0)));
    REQUIRE_FALSE(g.tick(cmd(CommandType::Wait)));
    REQUIRE(w.reg.get<Position>(w.player) == Position{5, 5});
}

TEST_CASE("Quit and Restart are not handled by Game", "[game]") {
    Game g(1);
    REQUIRE_FALSE(g.tick(cmd(CommandType::Quit)));
    REQUIRE_FALSE(g.tick(cmd(CommandType::Restart)));
    REQUIRE(g.state() == GameState::Playing);
}
```

- [ ] **Step 3: Run — FAIL** (нет `core/game.hpp`).

- [ ] **Step 4: `src/core/command.hpp`**

```cpp
#pragma once

enum class CommandType {
    None,
    Move,
    Pickup,
    Descend,
    OpenInventory,
    CloseInventory,
    UseItem,
    Wait,
    Restart,
    Quit
};

struct Command {
    CommandType type = CommandType::None;
    int dx = 0;
    int dy = 0;
    int index = 0;  // для UseItem: 0-based индекс в рюкзаке
};
```

- [ ] **Step 5: `src/core/game.hpp`**

```cpp
#pragma once

#include <cstdint>
#include <memory>

#include "core/command.hpp"
#include "core/messages.hpp"
#include "core/world.hpp"

enum class GameState { Playing, Inventory, Dead, Won };

class Game {
public:
    explicit Game(std::uint32_t seed);
    Game(const Game&) = delete;
    Game& operator=(const Game&) = delete;

    // true, если мир сделал ход. Quit и Restart обрабатывает вызывающий код.
    bool tick(const Command& cmd);

    GameState state() const { return state_; }
    World& world() { return *world_; }
    const World& world() const { return *world_; }
    const MessageLog& log() const { return log_; }
    std::uint32_t seed() const { return seed_; }

private:
    void startFloor(int floor);
    bool endTurn();

    std::uint32_t seed_;
    std::unique_ptr<World> world_;
    MessageLog log_;
    GameState state_ = GameState::Playing;
};
```

- [ ] **Step 6: `src/core/game.cpp`**

```cpp
#include "core/game.hpp"

#include "core/events.hpp"
#include "core/generator.hpp"
#include "core/strings.hpp"
#include "core/systems/systems.hpp"

Game::Game(std::uint32_t seed) : seed_(seed), world_(std::make_unique<World>(seed)) {
    log_.attach(world_->events);
    startFloor(1);
}

void Game::startFloor(int floor) {
    World& w = *world_;
    buildFloor(w, floor);
    fovSystem(w);
    w.events.trigger(FloorChanged{floor});
}

bool Game::endTurn() {
    World& w = *world_;
    deathSystem(w);
    progressionSystem(w);
    if (w.bossKilled) {
        state_ = GameState::Won;
        w.events.trigger(Note{str::kVictory});
        return true;
    }
    fovSystem(w);
    aiSystem(w);
    deathSystem(w);
    if (w.reg.get<Health>(w.player).cur <= 0) state_ = GameState::Dead;
    return true;
}

bool Game::tick(const Command& cmd) {
    if (state_ == GameState::Dead || state_ == GameState::Won) return false;
    World& w = *world_;

    if (state_ == GameState::Inventory) {
        switch (cmd.type) {
            case CommandType::CloseInventory:
                state_ = GameState::Playing;
                return false;
            case CommandType::UseItem:
                if (!useItem(w, cmd.index)) return false;
                state_ = GameState::Playing;
                return endTurn();
            default:
                return false;
        }
    }

    switch (cmd.type) {
        case CommandType::Move:
            if (tryMove(w, w.player, cmd.dx, cmd.dy) == MoveResult::Blocked) return false;
            return endTurn();
        case CommandType::Wait:
            return endTurn();
        case CommandType::Pickup:
            if (!pickupItems(w)) return false;
            return endTurn();
        case CommandType::Descend:
            if (!isOnStairs(w)) {
                w.events.trigger(Note{str::kNoStairs});
                return false;
            }
            startFloor(w.floor + 1);
            return true;
        case CommandType::OpenInventory:
            state_ = GameState::Inventory;
            return false;
        default:
            return false;
    }
}
```

- [ ] **Step 7: Run — PASS**

- [ ] **Step 8: Commit**

```bash
git add src tests
git commit -m "feat: Game with commands, states and turn order"
```

---

### Task 8: Ввод и рендер (чистые функции)

**Files:**
- Create: `src/ui/terminal.hpp`, `src/ui/terminal.cpp`, `src/ui/input.hpp`, `src/ui/input.cpp`, `src/ui/renderer.hpp`, `src/ui/renderer.cpp`, `tests/test_input_render.cpp`
- Modify: `CMakeLists.txt` (цель `game_ui`, линковка в тесты)

**Interfaces:**
- Consumes: `Game`, `GameState`, `Command`, компоненты, `effectiveAttack/Defense`, `xpToNext`, `str::*`.
- Produces: `enum class Key{None,Up,Down,Left,Right,Escape,Char}`; `struct KeyEvent{Key key=None; char ch=0;}`; `struct TerminalSize{int cols,rows;}`; `class Terminal{virtual KeyEvent readKey()=0; virtual TerminalSize size() const=0; void write(const std::string&);}` + защищённые `enterScreen()/leaveScreen()`; `std::unique_ptr<Terminal> makeTerminal()` (объявление; реализация в Task 9); `Command commandFromKey(const KeyEvent&, GameState)`; `Renderer{ static constexpr int kCols=80, kRows=29; std::string renderFrame(const Game&) const; void draw(Terminal&, const Game&) const; }`.
- Формат кадра: ровно `kRows` строк (HUD 1 + карта/панель 24 + сообщения 4), разделитель `\r\n` между строками, без хвостового.

- [ ] **Step 1: CMake — добавить `game_ui`** (после блока `game_core`, до `add_executable(roguelike ...)`), и обновить линковку тестов

```cmake
file(GLOB UI_SOURCES CONFIGURE_DEPENDS src/ui/*.cpp)
list(FILTER UI_SOURCES EXCLUDE REGEX ".*terminal_(win|posix)\\.cpp$")
add_library(game_ui STATIC ${UI_SOURCES})
target_link_libraries(game_ui PUBLIC game_core)
```

и заменить строку линковки тестов на:

```cmake
target_link_libraries(game_tests PRIVATE game_core game_ui Catch2::Catch2WithMain)
```

- [ ] **Step 2: `tests/test_input_render.cpp`**

```cpp
#include <algorithm>
#include <catch2/catch_test_macros.hpp>
#include <string>

#include "core/systems/systems.hpp"
#include "core/templates.hpp"
#include "game_helpers.hpp"
#include "test_helpers.hpp"
#include "ui/input.hpp"
#include "ui/renderer.hpp"

namespace {
KeyEvent ch(char c) { return KeyEvent{Key::Char, c}; }
KeyEvent special(Key k) { return KeyEvent{k, 0}; }
int newlines(const std::string& s) { return static_cast<int>(std::count(s.begin(), s.end(), '\n')); }
}  // namespace

TEST_CASE("playing: movement keys and arrows", "[input]") {
    const auto s = GameState::Playing;
    REQUIRE(commandFromKey(ch('w'), s).type == CommandType::Move);
    REQUIRE(commandFromKey(ch('w'), s).dy == -1);
    REQUIRE(commandFromKey(ch('S'), s).dy == 1);
    REQUIRE(commandFromKey(ch('a'), s).dx == -1);
    REQUIRE(commandFromKey(ch('d'), s).dx == 1);
    REQUIRE(commandFromKey(special(Key::Up), s).dy == -1);
    REQUIRE(commandFromKey(special(Key::Down), s).dy == 1);
    REQUIRE(commandFromKey(special(Key::Left), s).dx == -1);
    REQUIRE(commandFromKey(special(Key::Right), s).dx == 1);
}

TEST_CASE("playing: action keys", "[input]") {
    const auto s = GameState::Playing;
    REQUIRE(commandFromKey(ch('g'), s).type == CommandType::Pickup);
    REQUIRE(commandFromKey(ch('i'), s).type == CommandType::OpenInventory);
    REQUIRE(commandFromKey(ch('e'), s).type == CommandType::OpenInventory);
    REQUIRE(commandFromKey(ch('>'), s).type == CommandType::Descend);
    REQUIRE(commandFromKey(ch('z'), s).type == CommandType::Wait);
    REQUIRE(commandFromKey(ch('q'), s).type == CommandType::Quit);
}

TEST_CASE("unknown keys do nothing", "[input]") {
    REQUIRE(commandFromKey(ch('x'), GameState::Playing).type == CommandType::None);
    REQUIRE(commandFromKey(special(Key::None), GameState::Playing).type == CommandType::None);
    REQUIRE(commandFromKey(special(Key::Escape), GameState::Playing).type == CommandType::None);
}

TEST_CASE("inventory: digits use items, movement keys are ignored", "[input]") {
    const auto s = GameState::Inventory;
    const auto one = commandFromKey(ch('1'), s);
    REQUIRE(one.type == CommandType::UseItem);
    REQUIRE(one.index == 0);
    REQUIRE(commandFromKey(ch('9'), s).index == 8);
    REQUIRE(commandFromKey(ch('0'), s).type == CommandType::None);
    REQUIRE(commandFromKey(ch('a'), s).type == CommandType::None);
    REQUIRE(commandFromKey(special(Key::Escape), s).type == CommandType::CloseInventory);
    REQUIRE(commandFromKey(ch('i'), s).type == CommandType::CloseInventory);
    REQUIRE(commandFromKey(ch('q'), s).type == CommandType::Quit);
}

TEST_CASE("after the run ends R restarts and any other key quits", "[input]") {
    REQUIRE(commandFromKey(ch('r'), GameState::Dead).type == CommandType::Restart);
    REQUIRE(commandFromKey(ch('R'), GameState::Won).type == CommandType::Restart);
    REQUIRE(commandFromKey(ch('x'), GameState::Dead).type == CommandType::Quit);
    REQUIRE(commandFromKey(special(Key::Up), GameState::Won).type == CommandType::Quit);
    REQUIRE(commandFromKey(special(Key::None), GameState::Dead).type == CommandType::None);
}

TEST_CASE("frame has a fixed height and shows the HUD and hero", "[render]") {
    Game g(3);
    Renderer r;
    const auto frame = r.renderFrame(g);
    REQUIRE(newlines(frame) == Renderer::kRows - 1);
    REQUIRE(frame.find("HP 30/30") != std::string::npos);
    REQUIRE(frame.find("Floor 1") != std::string::npos);
    REQUIRE(frame.find('@') != std::string::npos);
}

TEST_CASE("fog of war hides monsters outside the view", "[render]") {
    Game g(3);
    auto& w = prepareOpenFloor(g);
    w.reg.get<Viewshed>(w.player).radius = 3;
    spawn(w.reg, "troll", Position{15, 5});  // 'T' нет ни в HUD, ни в сообщениях
    fovSystem(w);
    Renderer r;
    REQUIRE(r.renderFrame(g).find('T') == std::string::npos);

    w.reg.get<Viewshed>(w.player).radius = 12;
    fovSystem(w);
    REQUIRE(r.renderFrame(g).find('T') != std::string::npos);
}

TEST_CASE("inventory panel lists items and keeps frame height", "[render]") {
    Game g(3);
    auto& w = prepareOpenFloor(g);
    giveItem(w, "dagger");
    g.tick(Command{CommandType::OpenInventory});
    Renderer r;
    const auto frame = r.renderFrame(g);
    REQUIRE(newlines(frame) == Renderer::kRows - 1);
    REQUIRE(frame.find("1) Dagger") != std::string::npos);
    REQUIRE(frame.find("Inventory") != std::string::npos);
}

TEST_CASE("death banner is shown", "[render]") {
    Game g(3);
    auto& w = prepareOpenFloor(g);
    spawn(w.reg, "goblin", Position{6, 5});
    w.reg.get<Health>(w.player).cur = 1;
    g.tick(Command{CommandType::Wait});
    Renderer r;
    const auto frame = r.renderFrame(g);
    REQUIRE(newlines(frame) == Renderer::kRows - 1);
    REQUIRE(frame.find("You died on floor 1") != std::string::npos);
}
```

- [ ] **Step 3: Run — FAIL** (нет `ui/*.hpp`).

- [ ] **Step 4: `src/ui/terminal.hpp`**

```cpp
#pragma once

#include <memory>
#include <string>

enum class Key { None, Up, Down, Left, Right, Escape, Char };

struct KeyEvent {
    Key key = Key::None;
    char ch = 0;
};

struct TerminalSize {
    int cols;
    int rows;
};

// Вывод — ANSI-последовательности (одинаково на Windows 10+ и POSIX).
// Платформенные подклассы реализуют ввод одной клавиши и размер окна
// и вызывают enterScreen()/leaveScreen() в конструкторе/деструкторе (RAII).
class Terminal {
public:
    virtual ~Terminal() = default;
    virtual KeyEvent readKey() = 0;  // блокирующее чтение
    virtual TerminalSize size() const = 0;
    void write(const std::string& text);

protected:
    void enterScreen();
    void leaveScreen();
};

std::unique_ptr<Terminal> makeTerminal();
```

- [ ] **Step 5: `src/ui/terminal.cpp`**

```cpp
#include "ui/terminal.hpp"

#include <cstdio>

void Terminal::write(const std::string& text) {
    std::fwrite(text.data(), 1, text.size(), stdout);
    std::fflush(stdout);
}

// альтернативный экран, скрыть курсор, очистить
void Terminal::enterScreen() { write("\x1b[?1049h\x1b[?25l\x1b[2J"); }

// сброс цвета, показать курсор, вернуть основной экран
void Terminal::leaveScreen() { write("\x1b[0m\x1b[?25h\x1b[?1049l"); }
```

- [ ] **Step 6: `src/ui/input.hpp` и `input.cpp`**

```cpp
// input.hpp
#pragma once

#include "core/command.hpp"
#include "core/game.hpp"
#include "ui/terminal.hpp"

// Чистая функция: клавиша + состояние игры -> команда.
Command commandFromKey(const KeyEvent& key, GameState state);
```

```cpp
// input.cpp
#include "ui/input.hpp"

#include <cctype>

namespace {
Command move(int dx, int dy) { return Command{CommandType::Move, dx, dy}; }
}  // namespace

Command commandFromKey(const KeyEvent& key, GameState state) {
    if (key.key == Key::None) return Command{};
    const char c = key.key == Key::Char
                       ? static_cast<char>(std::tolower(static_cast<unsigned char>(key.ch)))
                       : '\0';

    if (state == GameState::Dead || state == GameState::Won) {
        return Command{c == 'r' ? CommandType::Restart : CommandType::Quit};
    }

    if (state == GameState::Inventory) {
        if (key.key == Key::Escape || c == 'i' || c == 'e') return Command{CommandType::CloseInventory};
        if (c == 'q') return Command{CommandType::Quit};
        if (c >= '1' && c <= '9') return Command{CommandType::UseItem, 0, 0, c - '1'};
        return Command{};
    }

    if (key.key == Key::Up || c == 'w') return move(0, -1);
    if (key.key == Key::Down || c == 's') return move(0, 1);
    if (key.key == Key::Left || c == 'a') return move(-1, 0);
    if (key.key == Key::Right || c == 'd') return move(1, 0);
    if (c == 'g') return Command{CommandType::Pickup};
    if (c == 'i' || c == 'e') return Command{CommandType::OpenInventory};
    if (c == '>') return Command{CommandType::Descend};
    if (c == 'z') return Command{CommandType::Wait};
    if (c == 'q') return Command{CommandType::Quit};
    return Command{};
}
```

- [ ] **Step 7: `src/ui/renderer.hpp`**

```cpp
#pragma once

#include <string>

#include "core/game.hpp"
#include "ui/terminal.hpp"

class Renderer {
public:
    static constexpr int kCols = 80;
    static constexpr int kRows = 29;  // HUD 1 + карта/панель 24 + сообщения 4

    // Чистая функция (удобна для тестов): строки разделены "\r\n", без хвостового.
    std::string renderFrame(const Game& game) const;
    void draw(Terminal& terminal, const Game& game) const;
};
```

- [ ] **Step 8: `src/ui/renderer.cpp`**

```cpp
#include "ui/renderer.hpp"

#include <algorithm>
#include <vector>

#include "core/components.hpp"
#include "core/strings.hpp"
#include "core/systems/systems.hpp"

namespace {

constexpr int kMapRows = kMapHeight;  // 24
constexpr int kMessageRows = 4;
constexpr const char* kReset = "\x1b[0m";
constexpr const char* kClearEol = "\x1b[K";

const char* ansi(Color c) {
    switch (c) {
        case Color::Gray: return "\x1b[90m";
        case Color::Red: return "\x1b[31m";
        case Color::Green: return "\x1b[32m";
        case Color::Yellow: return "\x1b[33m";
        case Color::Blue: return "\x1b[34m";
        case Color::Magenta: return "\x1b[35m";
        case Color::Cyan: return "\x1b[36m";
        case Color::White: break;
    }
    return "\x1b[37m";
}

struct Glyph {
    char ch = ' ';
    Color color = Color::White;
};

std::string hudLine(const World& w) {
    const auto& reg = w.reg;
    const auto& hp = reg.get<Health>(w.player);
    const auto& lvl = reg.get<Level>(w.player);
    return "HP " + std::to_string(hp.cur) + "/" + std::to_string(hp.max) +
           "  Lv " + std::to_string(lvl.lvl) +
           "  XP " + std::to_string(lvl.xp) + "/" + std::to_string(xpToNext(lvl.lvl)) +
           "  Floor " + std::to_string(w.floor) +
           "  Gold " + std::to_string(reg.get<Gold>(w.player).amount) +
           "  Atk " + std::to_string(effectiveAttack(reg, w.player)) +
           "  Def " + std::to_string(effectiveDefense(reg, w.player));
}

std::vector<std::string> mapLines(const World& w) {
    const Map& m = w.map;
    const auto cells = static_cast<std::size_t>(m.width() * m.height());
    std::vector<Glyph> top(cells);
    std::vector<int> layer(cells, -1);
    for (auto e : w.reg.view<const Position, const Renderable>()) {
        const auto& p = w.reg.get<Position>(e);
        if (!m.inBounds(p.x, p.y) || !m.visible(p.x, p.y)) continue;
        const auto& r = w.reg.get<Renderable>(e);
        const auto i = static_cast<std::size_t>(p.y * m.width() + p.x);
        if (r.layer > layer[i]) {
            layer[i] = r.layer;
            top[i] = Glyph{r.glyph, r.color};
        }
    }

    std::vector<std::string> lines;
    for (int y = 0; y < m.height(); ++y) {
        std::string line;
        bool haveColor = false;
        Color current = Color::White;
        for (int x = 0; x < m.width(); ++x) {
            const auto i = static_cast<std::size_t>(y * m.width() + x);
            Glyph g;
            if (m.visible(x, y)) {
                g = layer[i] >= 0 ? top[i]
                                  : Glyph{m.walkable(x, y) ? '.' : '#', Color::White};
            } else if (m.explored(x, y)) {
                g = Glyph{m.walkable(x, y) ? '.' : '#', Color::Gray};
            }
            if (!haveColor || g.color != current) {
                line += ansi(g.color);
                current = g.color;
                haveColor = true;
            }
            line += g.ch;
        }
        line += kReset;
        lines.push_back(std::move(line));
    }
    return lines;
}

std::vector<std::string> inventoryLines(const World& w) {
    const auto& reg = w.reg;
    std::vector<std::string> lines;
    lines.push_back(str::kInventoryTitle);
    const auto& eq = reg.get<Equipment>(w.player);
    lines.push_back(std::string(str::kWeaponLabel) +
                    (eq.weapon != entt::null ? reg.get<Name>(eq.weapon).value : std::string(str::kNone)));
    lines.push_back(std::string(str::kArmorLabel) +
                    (eq.armor != entt::null ? reg.get<Name>(eq.armor).value : std::string(str::kNone)));
    lines.push_back("");
    const auto& inv = reg.get<Inventory>(w.player);
    for (std::size_t i = 0; i < inv.items.size(); ++i) {
        lines.push_back(std::to_string(i + 1) + ") " + reg.get<Name>(inv.items[i]).value);
    }
    lines.push_back("");
    lines.push_back(str::kInventoryHint);
    return lines;
}

void fit(std::vector<std::string>& lines, std::size_t count) {
    lines.resize(count);  // обрезает лишнее, добавляет пустые строки
}

std::string clip(const std::string& s) {
    return s.size() > static_cast<std::size_t>(Renderer::kCols)
               ? s.substr(0, static_cast<std::size_t>(Renderer::kCols))
               : s;
}

}  // namespace

std::string Renderer::renderFrame(const Game& game) const {
    const World& w = game.world();
    std::vector<std::string> lines;
    lines.push_back(hudLine(w));

    auto body = game.state() == GameState::Inventory ? inventoryLines(w) : mapLines(w);
    fit(body, kMapRows);
    lines.insert(lines.end(), body.begin(), body.end());

    const bool over = game.state() == GameState::Dead || game.state() == GameState::Won;
    auto messages = game.log().recent(over ? kMessageRows - 1 : kMessageRows);
    for (auto& m : messages) m = clip(m);
    if (over) {
        fit(messages, kMessageRows - 1);
        const std::string banner =
            game.state() == GameState::Dead
                ? std::string(str::kDiedBanner) + std::to_string(w.floor)
                : std::string(str::kWonBanner) + std::to_string(w.reg.get<Gold>(w.player).amount);
        messages.push_back(banner + str::kPressRestart);
    }
    fit(messages, kMessageRows);
    lines.insert(lines.end(), messages.begin(), messages.end());

    std::string out;
    for (std::size_t i = 0; i < lines.size(); ++i) {
        out += lines[i];
        out += kClearEol;
        if (i + 1 < lines.size()) out += "\r\n";
    }
    return out;
}

void Renderer::draw(Terminal& terminal, const Game& game) const {
    terminal.write("\x1b[H" + renderFrame(game));
}
```

Примечание: `recent()` возвращает сообщения от старых к новым; при нехватке сообщений `fit` дополняет пустыми строками снизу.

- [ ] **Step 9: Run — PASS**

- [ ] **Step 10: Commit**

```bash
git add CMakeLists.txt src tests
git commit -m "feat: input mapping and frame renderer"
```

---

### Task 9: Платформенный терминал, main, ручная проверка

**Files:**
- Create: `src/ui/terminal_win.cpp`, `src/ui/terminal_posix.cpp`, `src/main.cpp`
- Delete: `main.cpp` (корневой шаблон CLion)
- Modify: `CMakeLists.txt`

**Interfaces:**
- Consumes: `Terminal`, `makeTerminal()` (объявлено), `Game`, `Renderer`, `commandFromKey`.
- Produces: исполняемый `roguelike`; `./roguelike [seed]`.

- [ ] **Step 1: `src/ui/terminal_win.cpp`**

```cpp
#ifdef _WIN32

#define NOMINMAX
#define WIN32_LEAN_AND_MEAN
#include <conio.h>
#include <windows.h>

#include "ui/terminal.hpp"

namespace {

class WinTerminal final : public Terminal {
public:
    WinTerminal() {
        out_ = GetStdHandle(STD_OUTPUT_HANDLE);
        GetConsoleMode(out_, &savedMode_);
        SetConsoleMode(out_, savedMode_ | ENABLE_VIRTUAL_TERMINAL_PROCESSING);
        enterScreen();
    }

    ~WinTerminal() override {
        leaveScreen();
        SetConsoleMode(out_, savedMode_);
    }

    KeyEvent readKey() override {
        const int c = _getch();
        if (c == 0 || c == 0xE0) {
            switch (_getch()) {
                case 72: return KeyEvent{Key::Up, 0};
                case 80: return KeyEvent{Key::Down, 0};
                case 75: return KeyEvent{Key::Left, 0};
                case 77: return KeyEvent{Key::Right, 0};
                default: return KeyEvent{};
            }
        }
        if (c == 27) return KeyEvent{Key::Escape, 0};
        return KeyEvent{Key::Char, static_cast<char>(c)};
    }

    TerminalSize size() const override {
        CONSOLE_SCREEN_BUFFER_INFO info;
        if (!GetConsoleScreenBufferInfo(out_, &info)) return TerminalSize{80, 24};
        return TerminalSize{info.srWindow.Right - info.srWindow.Left + 1,
                            info.srWindow.Bottom - info.srWindow.Top + 1};
    }

private:
    HANDLE out_ = nullptr;
    DWORD savedMode_ = 0;
};

}  // namespace

std::unique_ptr<Terminal> makeTerminal() { return std::make_unique<WinTerminal>(); }

#endif  // _WIN32
```

- [ ] **Step 2: `src/ui/terminal_posix.cpp`**

```cpp
#ifndef _WIN32

#include <poll.h>
#include <sys/ioctl.h>
#include <termios.h>
#include <unistd.h>

#include "ui/terminal.hpp"

namespace {

class PosixTerminal final : public Terminal {
public:
    PosixTerminal() {
        tcgetattr(STDIN_FILENO, &saved_);
        termios raw = saved_;
        raw.c_lflag &= ~static_cast<tcflag_t>(ICANON | ECHO);
        raw.c_cc[VMIN] = 1;
        raw.c_cc[VTIME] = 0;
        tcsetattr(STDIN_FILENO, TCSAFLUSH, &raw);
        enterScreen();
    }

    ~PosixTerminal() override {
        leaveScreen();
        tcsetattr(STDIN_FILENO, TCSAFLUSH, &saved_);
    }

    KeyEvent readKey() override {
        char c = 0;
        if (read(STDIN_FILENO, &c, 1) != 1) return KeyEvent{Key::Char, 'q'};  // EOF -> выход
        if (c != 27) return KeyEvent{Key::Char, c};
        if (!byteReady(50)) return KeyEvent{Key::Escape, 0};
        char seq[2] = {0, 0};
        if (read(STDIN_FILENO, &seq[0], 1) != 1) return KeyEvent{Key::Escape, 0};
        if (seq[0] != '[') return KeyEvent{};
        if (read(STDIN_FILENO, &seq[1], 1) != 1) return KeyEvent{};
        switch (seq[1]) {
            case 'A': return KeyEvent{Key::Up, 0};
            case 'B': return KeyEvent{Key::Down, 0};
            case 'C': return KeyEvent{Key::Right, 0};
            case 'D': return KeyEvent{Key::Left, 0};
            default: return KeyEvent{};
        }
    }

    TerminalSize size() const override {
        winsize ws{};
        if (ioctl(STDOUT_FILENO, TIOCGWINSZ, &ws) != 0 || ws.ws_col == 0) return TerminalSize{80, 24};
        return TerminalSize{ws.ws_col, ws.ws_row};
    }

private:
    static bool byteReady(int timeoutMs) {
        pollfd p{STDIN_FILENO, POLLIN, 0};
        return poll(&p, 1, timeoutMs) > 0;
    }

    termios saved_{};
};

}  // namespace

std::unique_ptr<Terminal> makeTerminal() { return std::make_unique<PosixTerminal>(); }

#endif  // !_WIN32
```

- [ ] **Step 3: `src/main.cpp`**

```cpp
#include <cstdint>
#include <cstdlib>
#include <iostream>
#include <memory>
#include <random>
#include <string>

#include "core/game.hpp"
#include "core/strings.hpp"
#include "ui/input.hpp"
#include "ui/renderer.hpp"
#include "ui/terminal.hpp"

int main(int argc, char** argv) {
    std::random_device rd;
    const std::uint32_t firstSeed =
        argc > 1 ? static_cast<std::uint32_t>(std::strtoul(argv[1], nullptr, 10)) : rd();
    auto game = std::make_unique<Game>(firstSeed);
    Renderer renderer;

    {
        const auto terminal = makeTerminal();  // деструктор вернёт консоль в исходный режим
        const auto size = terminal->size();
        if (size.cols < Renderer::kCols || size.rows < Renderer::kRows) {
            terminal->write(std::string("\x1b[H") + str::kTooSmall + " (need " +
                            std::to_string(Renderer::kCols) + "x" + std::to_string(Renderer::kRows) +
                            ", have " + std::to_string(size.cols) + "x" + std::to_string(size.rows) +
                            ")");
            terminal->readKey();
        }

        for (;;) {
            renderer.draw(*terminal, *game);
            const Command cmd = commandFromKey(terminal->readKey(), game->state());
            if (cmd.type == CommandType::Quit) break;
            if (cmd.type == CommandType::Restart) {
                game = std::make_unique<Game>(rd());
                continue;
            }
            game->tick(cmd);
        }
    }

    std::cout << "Seed: " << game->seed() << "\n";
    return 0;
}
```

- [ ] **Step 4: CMake — заменить временный exe и добавить платформенный файл**

Удалить блок `# временно, заменится в Task 9` с `add_executable(roguelike main.cpp)`. После `add_library(game_ui ...)` добавить:

```cmake
if(WIN32)
    target_sources(game_ui PRIVATE src/ui/terminal_win.cpp)
else()
    target_sources(game_ui PRIVATE src/ui/terminal_posix.cpp)
endif()

add_executable(roguelike src/main.cpp)
target_link_libraries(roguelike PRIVATE game_ui)
```

Удалить корневой `main.cpp`: `git rm -f main.cpp` (если уже в индексе) — иначе `rm main.cpp`.

- [ ] **Step 5: Build всё**

Run: `cmake -S . -B cmake-build-debug && cmake --build cmake-build-debug`
Expected: собираются `roguelike` и `game_tests` без ошибок.

- [ ] **Step 6: Прогнать тесты**

Run: `./cmake-build-debug/game_tests`
Expected: все PASS.

- [ ] **Step 7: Ручная проверка** (в Windows Terminal / обычном терминале; консоль CLion «Run» не подходит — нет raw-ввода и ANSI; включить «Emulate terminal in output console» или запускать из терминала)

Run: `./cmake-build-debug/roguelike 42`

Чек-лист:
- виден `@`, комната вокруг, остальное чёрное (туман)
- WASD и стрелки двигают; в стену не идёт и ход не тратится (враги стоят)
- `g` на предмете подбирает; `g` на пустой клетке пишет «There is nothing here.»
- `i` открывает рюкзак; цифра использует/надевает; Esc закрывает; HUD обновляет Atk/Def
- встреча с врагом: сообщения «You hit…/The … hits you…»; XP растёт, Lv повышается
- `>` на лестнице → Floor 2; `>` вне лестницы → «There are no stairs here.»
- смерть → баннер «You died on floor N…», `R` начинает новый забег со случайным seed, любая другая клавиша выходит, консоль восстановлена (курсор виден, экран прежний), печатается `Seed: 42`
- `q` в любой момент выходит корректно
- окно меньше 80×29 → предупреждение, затем игра продолжается после клавиши
- один и тот же seed даёт ту же первую карту

- [ ] **Step 8: Баланс (по результатам 2–3 забегов)**

Если этаж 1 слишком смертелен/скучен — править только числа в таблицах `src/core/templates.cpp` (hp, атака, вес, шанс лута в `death.cpp`). Логику не менять. Прогнать `game_tests` после правок: тесты зависят от конкретных значений (`goblin` hp 8 / atk 3 / def 1, `rat` hp 4, `dagger` +2, `chain_mail` +3, `health_potion` 10, игрок 30 hp / atk 4 / def 1, `xpToNext = 20*lvl`) — при изменении этих чисел обновить соответствующие тесты.

- [ ] **Step 9: Commit**

```bash
git add -A
git commit -m "feat: platform terminals and main loop; playable roguelike"
```

---

### Task 10: CI (GitHub Actions) и push

**Files:**
- Create: `.github/workflows/build.yml`

**Interfaces:**
- Consumes: цели `roguelike` и `game_tests`, `add_test(NAME game_tests ...)` из CMake.
- Produces: workflow `build` — сборка и тесты на Linux, Windows, macOS при каждом push (любая ветка) и pull request; артефакты бинарников.

- [ ] **Step 1: `.github/workflows/build.yml`**

```yaml
name: build

on:
  push:
  pull_request:

jobs:
  build:
    name: ${{ matrix.os }}
    runs-on: ${{ matrix.os }}
    strategy:
      fail-fast: false
      matrix:
        os: [ubuntu-latest, windows-latest, macos-latest]
    steps:
      - uses: actions/checkout@v4

      - name: Configure
        run: cmake -S . -B build -DCMAKE_BUILD_TYPE=Release

      - name: Build
        run: cmake --build build --config Release --parallel

      - name: Test
        run: ctest --test-dir build -C Release --output-on-failure

      - name: Upload binary
        uses: actions/upload-artifact@v4
        with:
          name: roguelike-${{ matrix.os }}
          path: |
            build/roguelike
            build/Release/roguelike.exe
          if-no-files-found: error
```

Примечания: Windows-раннер использует MSVC (мульти-конфиг, бинарник в `build/Release/`), Linux/macOS — однокофиг (`build/`). Ветки не фильтруются: сборка идёт при push в любую ветку.

- [ ] **Step 2: Проверить YAML локально**

Run: `python -c "import yaml,sys; yaml.safe_load(open('.github/workflows/build.yml')); print('yaml ok')"`
Expected: `yaml ok` (если нет модуля `yaml` — пропустить, проверит GitHub).

- [ ] **Step 3: Commit**

```bash
git add .github
git commit -m "ci: build and test on Linux, Windows and macOS"
```

- [ ] **Step 4: Remote и push**

Получить URL удалённого репозитория у пользователя (пустой репозиторий на GitHub). Если URL ещё не дан — спросить и остановиться; без подтверждения не пушить.

```bash
git branch -M main
git remote add origin <URL от пользователя>
git push -u origin main
```

- [ ] **Step 5: Проверить CI**

Run: `gh run watch` (или `gh run list --limit 1`). Expected: все три job зелёные. Падение — разобрать лог (`gh run view --log-failed`), исправить причину, закоммитить, запушить. Типичный риск: ошибки компиляции под MSVC/clang, которых нет у основного компилятора.

---

## Self-Review

**Spec coverage:** цель/рамки → Tasks 1,9; структура → File Structure; компоненты/шаблоны/системы → Tasks 2–5; генерация → 6; цикл/состояния/управление/экран → 7,8,9; RAII терминал → 9; тесты Catch2 (урон, связность, подбор/использование/экипировка, уровень, смерть+XP) → 3–7; решения (без промахов, 5 этажей, шаблоны в коде) → 2,3,6.

**Отклонения от спецификации (осознанные уточнения):**
- Клавиша `e` открывает тот же инвентарь, что `i`; цифра в меню использует зелье или надевает снаряжение (отдельного режима «надеть» нет).
- После смерти/победы `R` перезапускает игру (команда `Restart`), любая другая клавиша выходит.
- Предупреждение о размере окна проверяет 80×29 (HUD + карта 24 + 4 строки сообщений), не 80×24.
- Добавлены события `GoldPicked`, `Healed`, `Equipped`, `Note`; кошелёк игрока — компонент `Gold` на сущности игрока.
- `Died`/`Attacked` и др. несут строки (имена), так как сущность к моменту форматирования может быть удалена.
- Лестница — сущность `Stairs` (как в таблице компонентов), а не тип тайла.

**Placeholders:** нет. **Типы:** сигнатуры систем (`tryMove`, `useItem`, `pickupItems`, `isOnStairs`, `fovSystem`, `aiSystem`, `buildFloor`, `generateLayout`) совпадают между объявлением, реализацией и тестами. **Review Focus:** все 5 пунктов имеют тесты (`[focus]`).

## Решения пользователя (учтены в плане)

- Проект и exe: `roguelike`. `cmake_minimum_required` понижен до 3.24 (совместимость с раннерами CI).
- Баланс — первое приближение, правка после игры (Task 9, Step 8).
- `git init` разрешён. Push на удалённый сервер и CI на всех платформах — Task 10.
- После смерти/победы: `R` — новый забег, любая другая клавиша — выход.

## Unresolved questions

- URL удалённого репозитория (нужен в Task 10, Step 4).
