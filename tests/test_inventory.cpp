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
    REQUIRE((w->reg.get<Equipment>(w->player).weapon == entt::null));
}

TEST_CASE("isOnStairs", "[inventory]") {
    auto w = makeOpenWorld();
    REQUIRE_FALSE(isOnStairs(*w));
    spawnStairs(w->reg, Position{6, 5});
    REQUIRE_FALSE(isOnStairs(*w));
    spawnStairs(w->reg, Position{5, 5});
    REQUIRE(isOnStairs(*w));
}
