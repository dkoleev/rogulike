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
