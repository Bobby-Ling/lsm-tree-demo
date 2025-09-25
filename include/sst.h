#pragma once

#include "config.h"
#include "log.h"
#include "mem_table.h"

#include <cstddef>
#include <list>
#include <map>
#include <queue>
#include <vector>
#include <optional>

/*
SST要满足:
- 支持范围查询
- 能够快速得知是否包含某个Key
*/
template <typename K, typename V>
class SST {
    // 删除操作是插入一个std::nullopt
    std::map<K, std::optional<V>> table;
    std::size_t max_size;

    SST(const SST&) = delete;
    SST& operator=(const SST&) = delete;

    friend class fmt::formatter<SST<K, V>>;

  public:
    explicit SST(std::size_t max_size = CONFIG::NUM_SST_ENTRY) : max_size(max_size) {}

    explicit SST(const MemTable<K, V>& memtable, std::size_t max_size = CONFIG::NUM_SST_ENTRY) : table(memtable.get_table()), max_size(max_size) {}

    SST(SST&&) = default;
    SST& operator=(SST&&) = default;

    void set_max_size(std::size_t max_size) {
        this->max_size = max_size;
    }

    void set(const K &key, const std::optional<V> &value) {
        LOG_TRACE("key={}, value={}", key, value);
        table[key] = value;
    }

    std::optional<V> get(const K &key) const {
        auto it = table.find(key);
        if (it != table.end()) {
            LOG_TRACE("key={}, found value={}", key, it->second);
            return it->second;
        }
        LOG_TRACE("key={}, not found", key);
        return std::nullopt;
    }

    std::size_t size() const { return table.size(); }
    std::size_t get_max_size() const { return max_size; }
    bool is_full() const { return table.size() >= max_size; }
    bool empty() const { return table.empty(); }

    std::pair<K, K> get_key_range() const {
        return {table.begin()->first, table.rbegin()->first};
    }

    bool contains_key(const K& key) const {
        return table.find(key) != table.end();
    }

    static SST<K, V> merge(std::list<SST<K, V>> ssts) {
        std::priority_queue<std::pair<K, std::optional<V>>, std::vector<std::pair<K, std::optional<V>>>, std::greater<>> min_heap;
        for (const auto& sst : ssts) {
            for (const auto& [key, value] : sst.table) {
                min_heap.emplace(key, value);
            }
        }
        // TODO
        SST<K, V> merged(min_heap.size());
        while (!min_heap.empty()) {
            const auto& top = min_heap.top();
            const K& key = top.first;
            const std::optional<V>& value = top.second;
            merged.set(key, value);
            min_heap.pop();
        }
        return merged;
    }
};


template <typename K, typename V>
struct fmt::formatter<SST<K, V>> : fmt::formatter<std::string_view> {
    template <typename FormatContext>
    auto format(const SST<K, V>& sst, FormatContext& ctx) const {
        auto out = ctx.out();
        out = fmt::format_to(out, "SST{{size: {}, max_size: {}}}", sst.size(), sst.get_max_size());
        return out;
    }
};