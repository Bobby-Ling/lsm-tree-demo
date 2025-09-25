#pragma once

#include "config.h"
#include "log.h"
#include "mem_table.h"
#include "sst.h"
#include "storage.h"

#include <list>
#include <memory>
#include <optional>

template <typename K, typename V>
class LSM {
    std::unique_ptr<MemTable<K, V>> mem_table;
    // 最新的在最后
    std::list<std::unique_ptr<MemTable<K, V>>> immutable_memtables;
    LevelStorage<K, V> levels;

    void flush_memtable() {
        LOG_INFO("MemTable is full, MemTable->Immutable MemTable");
        // MemTable full, 则MemTable->Immutable MemTable, 然后新建一个MemTable;
        immutable_memtables.push_back(std::move(mem_table));
        mem_table = std::make_unique<MemTable<K, V>>(CONFIG::NUM_MEM_ENTRY);

        // 如果Immutable MemTable也full, 则刷入L0(L0不保证不重叠)
        if (immutable_memtables.size() > CONFIG::NUM_MAX_MEM_TABLE) {
            LOG_INFO("Too many ImmutableMemTables, flushing oldest to SST");

            // 将最老的ImmutableMemTable转换为SST并添加到L0
            std::unique_ptr<MemTable<K, V>> oldest_memtable = std::move(immutable_memtables.front());
            immutable_memtables.pop_front();

            ASSERT_FATAL(!oldest_memtable->empty());
            SST<K, V> new_sst(*oldest_memtable);

            levels.add_sst_to_l0(std::move(new_sst));
            LOG_INFO("Added new SST to L0, now has {} SSTs", levels[0].get_sst_count());
        }
    }

  public:
    LSM() : mem_table(std::make_unique<MemTable<K, V>>(CONFIG::NUM_MEM_ENTRY)),
            levels(CONFIG::NUM_LEVELS) {
        LOG_INFO("LevelStorage: {}", this->levels);
    }

    void set(const K &key, const V &value) {
        LOG_DEBUG("key={}, value={}", key, value);

        // MemTable是否full
        if (mem_table->is_full()) {
            // MemTable full, 则MemTable->Immutable MemTable, 然后新建一个MemTable;
            // 如果Immutable MemTable也full, 则刷入L0(L0不保证不重叠)
            // 如果L0 full, 则将找到L1中与L0的SST的范围重叠的SST, 一起合并入L1
            flush_memtable();
        }

        // (Update/Delete)直接写入MemTable
        mem_table->set(key, value);

        LOG_DEBUG("completed for key={}", key);
    }

    std::optional<V> get(const K &key) const {
        LOG_DEBUG("key={}", key);

        // 1. MemTable
        auto result = mem_table->get(key);
        if (result.has_value()) {
            LOG_DEBUG("key={}, found in MemTable", key);
            return result;
        }

        // 2. Immutable MemTable(较新的在后面)
        for (auto it = immutable_memtables.rbegin(); it != immutable_memtables.rend(); ++it) {
            result = (*it)->get(key);
            if (result.has_value()) {
                LOG_DEBUG("key={}, found in ImmutableMemTable", key);
                return result;
            }
        }

        // 3. SST(从L0->Lmax)
        result = levels.get(key);
        if (result.has_value()) {
            return result;
        }

        LOG_DEBUG("key={}, not found", key);
        return std::nullopt;
    }
};
