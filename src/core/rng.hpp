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
