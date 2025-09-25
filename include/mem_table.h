#pragma once

#include <map>
#include <optional>
#include <cstddef>

template <typename K, typename V>
class MemTable {
    // 删除操作是插入一个std::nullopt
    std::map<K, std::optional<V>> table;
    std::size_t max_size;

  public:
    explicit MemTable(std::size_t max_size = 4) : max_size(max_size) {}

    void set(const K &key, const std::optional<V> &value) {
        LOG_DEBUG("MemTable::set key={}, value={}", key, value.value());
        table[key] = value;
    }

    std::optional<V> get(const K &key) const {
        auto it = table.find(key);
        if (it != table.end()) {
            LOG_DEBUG("MemTable::get key={}, found value={}", key, it->second.value());
            return it->second;
        }
        LOG_DEBUG("MemTable::get key={}, not found", key);
        return std::nullopt;
    }

    std::size_t size() const { return table.size(); }
    bool is_full() const { return table.size() >= max_size; }
    bool empty() const { return table.empty(); }

    const std::map<K, std::optional<V>>& get_table() const { return table; }
};