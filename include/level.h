#pragma once

#include "log.h"
#include "sst.h"
#include "config.h"

#include <algorithm>
#include <cmath>
#include <cstddef>
#include <list>

template <typename K, typename V>
class Level {
    std::size_t level_num;
    // 新的在后面
    std::list<SST<K, V>> ssts;
    std::size_t max_ssts;
    Level<K, V> *next_level;

    friend class fmt::formatter<Level<K, V>>;

  public:
    explicit Level(std::size_t level_num, std::size_t max_ssts, Level<K, V> *next_level = nullptr)
        : level_num(level_num), max_ssts(max_ssts), next_level(next_level) {}

    void add_sst(SST<K, V> sst) {
        // 如果是merge的SST, 则大小不定
        LOG_DEBUG("adding SST {} to level {}", sst, level_num);
        sst.set_max_size(CONFIG::NUM_SST_ENTRY * std::pow(CONFIG::NUM_LEVEL_MULTI, level_num));
        LOG_DEBUG("SST {} set max size to {}", sst, sst.get_max_size());
        ssts.push_back(std::move(sst));
        if (needs_compaction()) {
            compact();
        }
    }

    void set_next_level(Level<K, V> *next_level) {
        this->next_level = next_level;
    }

    std::optional<V> get(const K &key) const {
        // 从新到旧遍历SST
        for (auto it = ssts.rbegin(); it != ssts.rend(); ++it) {
            auto result = it->get(key);
            if (result.has_value()) {
                LOG_DEBUG("key={}, found in level {}", key, level_num);
                return result;
            }
        }
        LOG_DEBUG("key={}, not found in level {}", key, level_num);
        return std::nullopt;
    }

    const std::list<SST<K, V>>& get_ssts() const { return ssts; }
    std::size_t get_sst_count() const { return ssts.size(); }
    std::size_t get_level_num() const { return level_num; }
    bool needs_compaction() const {
        LOG_DEBUG("level {}", *this);
        if (CONFIG::compact_type == CompactType::Leveling && level_num != 0) {
            return ssts.front().is_full() || ssts.size() > max_ssts;
        } else {
            return ssts.size() > max_ssts;
        }
    }

    void clear() { ssts.clear(); }

    void compact() {
        ASSERT_FATAL(needs_compaction() == true);
        ASSERT_FATAL(next_level);
        LOG_INFO("Compacting L{} with L{}:", level_num, next_level->level_num);
        LOG_DEBUG("\t{}",*this);
        LOG_DEBUG("\t{}", *next_level);
        if (CONFIG::compact_type == CompactType::Leveling && level_num != 0) {
            // L_{i-1}+L_{i}->L_i)
            if (next_level == nullptr) {
                // TODO
                LOG_FATAL("Compacting Lmax!");
                return;
            }
            ASSERT_FATAL(ssts.size() <= 2);
            ASSERT_FATAL(next_level->ssts.size() <= 1);
            std::list<SST<K, V>> to_be_merged_ssts = std::move(ssts);
            to_be_merged_ssts.splice(to_be_merged_ssts.end(), next_level->ssts);

            ASSERT_FATAL(next_level->ssts.size() == 0);

            auto merged_sst = SST<K, V>::merge(std::move(to_be_merged_ssts));

            // 添加到下一层
            next_level->add_sst(std::move(merged_sst));

            ASSERT_FATAL(ssts.size() == 0);
            ASSERT_FATAL(next_level->ssts.size() <= 1);
        } else {
            // 合并当前层的所有SST
            auto merged_sst = SST<K, V>::merge(std::move(ssts));
            next_level->add_sst(std::move(merged_sst));
            ASSERT_FATAL(ssts.size() == 0);
        }
        LOG_INFO("Now L{} & L{}:", level_num, next_level->level_num);
        LOG_DEBUG("\t{}",*this);
        LOG_DEBUG("\t{}", *next_level);
    }
};

template <typename K, typename V>
struct fmt::formatter<Level<K, V>> : fmt::formatter<std::string_view> {
    template <typename FormatContext>
    auto format(const Level<K, V>& level, FormatContext& ctx) const {
        auto out = ctx.out();
        out = fmt::format_to(out, "Level{{level_num: {}, sst_num: {}, max_ssts: {}, next_level: {}}}", level.level_num, level.get_sst_count(), level.max_ssts, level.next_level ? static_cast<int>(level.next_level->level_num) : -1);
        return out;
    }
};