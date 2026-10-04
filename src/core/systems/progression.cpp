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
