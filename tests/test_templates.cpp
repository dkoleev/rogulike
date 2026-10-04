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
    REQUIRE((reg.get<Equipment>(p).weapon == entt::null));
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
