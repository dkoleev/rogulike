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
