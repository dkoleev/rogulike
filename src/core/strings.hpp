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
