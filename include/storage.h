#pragma once

#include "level.h"
#include "config.h"

#include "fmt/format.h"
#include <cmath>
#include <optional>
#include <vector>

template <typename K, typename V>
class LevelStorage {
    std::vector<Level<K, V>> levels;

    friend class fmt::formatter<LevelStorage<K, V>>;

  public:
    explicit LevelStorage(std::size_t num_levels = CONFIG::NUM_LEVELS) {
        for (ssize_t i = num_levels - 1; i >= 0; --i) {
            ssize_t max_ssts = get_max_ssts_for_level(i);
            Level<K, V> level(i, max_ssts);
            levels.insert(levels.begin(), std::move(level));
        }
        for (size_t i = 0; i < levels.size() - 1; ++i) {
            levels[i].set_next_level(&levels[i + 1]);
        }
        // LOG_INFO("LevelStorage initialized: {}", *this);
        for (const auto &level : levels) {
            LOG_INFO("LevelStorage initialized: {}", level);
        }
    }

    void add_sst_to_l0(SST<K, V> sst) {
        levels[0].add_sst(std::move(sst));
    }

    std::optional<V> get(const K &key) const {
        std::optional<V> result;
        for (const auto &level : levels) {
            result = level.get(key);
            if (result.has_value()) {
                LOG_DEBUG("key={}, found in level {}", key, level.get_level_num());
                return result;
            }
        }
        return std::nullopt;
    }

    Level<K, V> &operator[](std::size_t index) { return levels[index]; }
    const Level<K, V> &operator[](std::size_t index) const { return levels[index]; }
    std::size_t size() const { return levels.size(); }

    friend std::ostream& operator<<(std::ostream& os, const LevelStorage<K, V>& level_storage) {
        return os << fmt::format("{}", level_storage);
    }

  private:
    std::size_t get_max_ssts_for_level(std::size_t level) const {
        if (level == 0)
            return CONFIG::NUM_MAX_L0_SST;

        switch (CONFIG::compact_type) {
            case CompactType::Leveling:
                return 1;
            case CompactType::Tiering:
                return CONFIG::NUM_MAX_L0_SST * std::pow(CONFIG::NUM_LEVEL_MULTI, level);
            default:
                return 0;
        }
    }
};

template <typename K, typename V>
struct fmt::formatter<LevelStorage<K, V>> : fmt::formatter<std::string_view> {
    template <typename FormatContext>
    auto format(const LevelStorage<K, V>& level_storage, FormatContext& ctx) const {
        return fmt::format_to(ctx.out(), "LevelStorage{{levels: [{}]}}", fmt::join(level_storage.levels, ", "));
    }
};

// template <typename K, typename V>
// LevelStorage<K, V>::LevelStorage(std::size_t num_levels) {
//     Level<K, V> *next_level = nullptr;
//     for (std::size_t i = num_levels; i-- > 0;) {
//         std::size_t max_ssts = get_max_ssts_for_level(i);
//         Level<K, V> level(i, max_ssts, next_level);
//         levels.insert(levels.begin(), std::move(level));
//         next_level = &levels.front();
//     }
//     LOG_INFO("LevelStorage initialized: {}", *this);
//     for (const auto &level : levels) {
//         LOG_INFO("LevelStorage initialized: {}", level);
//     }
// }