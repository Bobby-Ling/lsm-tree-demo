#pragma once

#include "log.h"
#include "sst.h"

#include <cstddef>
#include <list>

enum class CompactType {
    // 一个Level只有一个SST, 不同Level的SST之间大小倍率为NUM_LEVEL_MULTI
    Leveling,
    // 一个Level有T个SST, 不同Level的SST之间大小倍率为NUM_LEVEL_MULTI=T
    // 当L被填满时(该Level出现了T个component), 该层的T个component会合并为一个新的component(所以是T倍), 进入L+1
    Tiering
};

template <typename K, typename V>
class Level {
    std::size_t level_num;
    // 新的在后面
    std::list<SST<K, V>> ssts;
    std::size_t max_ssts;

  public:
    explicit Level(std::size_t level_num, std::size_t max_ssts = 10)
        : level_num(level_num), max_ssts(max_ssts) {}

    void add_sst(const SST<K, V> &sst) {
        LOG_DEBUG("adding SST to level {}", level_num);
        ssts.push_back(sst);
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
    bool needs_compaction() const { return ssts.size() > max_ssts; }

    void clear() { ssts.clear(); }
};