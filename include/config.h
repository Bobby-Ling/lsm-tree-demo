#pragma once

#include <string>
#include "level.h"
#include "log.h"

struct CONFIG {
    // MemTable的大小
    static inline std::size_t NUM_MEM_ENTRY = 4;
    // SST的大小
    static inline std::size_t NUM_SST_ENTRY = 3;
    // MemTable的最大数量(一些immutable_memtables和一个mem_table, 达到后阻塞前台进行写入)
    static inline std::size_t NUM_MAX_MEM_TABLE = 2;
    // LO最大大小(达到后阻塞MemTable进行flush)
    static inline std::size_t NUM_MAX_L0_SST = 3;
    // L1最大大小达到后阻塞L0往L1进行Compact)
    static inline std::size_t NUM_MAX_L1_SST = 5;
    // 从L1开始每一层放大(下一层的文件数量为上一层的倍数)
    static inline std::size_t NUM_LEVEL_MULTI = 10;
    // 最大层数
    static inline std::size_t NUM_LEVELS = 7;

    static inline CompactType compact_type = CompactType::Leveling;

    template <typename T>
    inline static void init_config(T &config, std::string_view config_name) {
        std::string_view config_name_sv = config_name.substr(config_name.find("::") + 2);
        std::string env_name = "CONFIG_" + std::string(config_name_sv.data());
        char *env = getenv(env_name.c_str());
        if (env) {
            config = static_cast<T>(std::stoi(env));
        }
        LOG_PRINT("{} = {}", env_name, config);
    }
};

#define INIT_CONFIG(name)            \
    {                                               \
        constexpr std::string_view config_name = #name;               \
        CONFIG::init_config(name, config_name.data()); \
    }

inline void init_all_config() {
}