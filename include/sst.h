#pragma once

#include "log.h"
#include "mem_table.h"

#include <cstddef>
#include <list>
#include <map>
#include <queue>
#include <vector>
#include <optional>

// 满足能够支持范围查询的结构
template <typename K, typename V>
class SST {
    // 删除操作是插入一个std::nullopt
    std::map<K, std::optional<V>> table;
    std::size_t max_size;

  public:
    explicit SST(std::size_t max_size = 4) : max_size(max_size) {}

    explicit SST(const MemTable<K, V>& memtable, std::size_t max_size = 4) : table(memtable.get_table()), max_size(max_size) {}

    void set(const K &key, const std::optional<V> &value) {
        LOG_DEBUG("key={}, value={}", key, value);
        table[key] = value;
    }

    std::optional<V> get(const K &key) const {
        auto it = table.find(key);
        if (it != table.end()) {
            LOG_DEBUG("key={}, found value={}", key, it->second);
            return it->second;
        }
        LOG_DEBUG("key={}, not found", key);
        return std::nullopt;
    }

    std::size_t size() const { return table.size(); }
    bool is_full() const { return table.size() >= max_size; }
    bool empty() const { return table.empty(); }

    std::pair<K, K> get_key_range() const {
        return {table.begin()->first, table.rbegin()->first};
    }

    bool contains_key(const K& key) const {
        return table.find(key) != table.end();
    }

    static SST<K, V> merge(const std::list<SST<K, V>>& ssts) {
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

