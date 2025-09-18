#pragma once

#include "level.h"
#include "config.h"

#include <optional>
#include <vector>

template <typename K, typename V>
class LevelStorage {
    std::vector<Level<K, V>> levels;

  public:
    explicit LevelStorage(std::size_t num_levels = CONFIG::NUM_LEVELS) {
        for (std::size_t i = 0; i < num_levels; ++i) {
            std::size_t max_ssts = get_max_ssts_for_level(i);
            levels.emplace_back(i, max_ssts);
        }
    }

    void add_sst(const SST<K, V> &sst) {
        levels[0].add_sst(sst);

        for (std::size_t i = 0; i < levels.size() - 1; ++i) {
            if (needs_compaction(i)) {
                LOG_INFO("Level {} needs compaction", i);
                compact_level(i);
            }
        }
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

  private:
    void compact_level(std::size_t level_num) {
        if (level_num >= levels.size() - 1) {
            return;
        }

        LOG_INFO("starting compaction from L{} to L{}", level_num, level_num + 1);

        auto &current_level = levels[level_num];
        auto &next_level = levels[level_num + 1];

        if (current_level.get_sst_count() == 0) {
            LOG_DEBUG("Level {} is empty, skip", level_num);
            return;
        }

        const auto &current_ssts = current_level.get_ssts();

        if (CONFIG::compact_type == CompactType::Leveling) {
            // Leveling策略: 将当前层的所有SST合并到下一层
            LOG_DEBUG("Using Leveling compaction strategy");

            // 合并当前层的所有SST
            auto merged_sst = SST<K, V>::merge(current_ssts);

            // 添加到下一层
            next_level.add_sst(merged_sst);

            // 清空当前层
            current_level.clear();

        } else {
            // Tiering策略: 将当前层的SST直接移动到下一层
            LOG_DEBUG("Using Tiering compaction strategy");

            for (const auto &sst : current_ssts) {
                next_level.add_sst(sst);
            }

            // 清空当前层
            current_level.clear();
        }

        LOG_INFO("Compaction completed: L{} -> L{}", level_num, level_num + 1);
    }

    bool needs_compaction(std::size_t level_index) const {
        if (level_index >= levels.size())
            return false;

        std::size_t max_ssts = get_max_ssts_for_level(level_index);
        return levels[level_index].get_sst_count() > max_ssts;
    }

    std::size_t get_max_ssts_for_level(std::size_t level) const {
        if (level == 0)
            return CONFIG::NUM_MAX_L0_SST;
        if (level == 1)
            return CONFIG::NUM_MAX_L1_SST;

        // L2及以后每层按倍数递增
        std::size_t multiplier = 1;
        for (std::size_t i = 2; i <= level; ++i) {
            multiplier *= CONFIG::NUM_LEVEL_MULTI;
        }
        return CONFIG::NUM_MAX_L1_SST * multiplier;
    }

    Level<K, V> &operator[](std::size_t index) { return levels[index]; }
    const Level<K, V> &operator[](std::size_t index) const { return levels[index]; }
    std::size_t size() const { return levels.size(); }
};