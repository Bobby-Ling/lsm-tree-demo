#pragma once

#include <string>
#include "log.h"

/*
- Leveling(层内有序、key不重叠)
  - 一层一个文件(SST)
  - 文件有序
  - 层之间的每个文件大小倍率为T
  - L-1层满了，就整个合并入L层（L_{i-1}+L_{i}->L_i)）
- Tiering(可以重叠)
  - 每层T个文件
  - 文件内部有序，文件之间key可以重叠
  - 层之间的每个文件大小倍率为T
  - L-1层满了，则T个文件整个合并作为L层的一个文件
*/

enum class CompactType {
    // 一个Level只有一个SST, 不同Level的SST之间大小倍率为NUM_LEVEL_MULTI
    Leveling,
    // 一个Level有T个SST, 不同Level的SST之间大小倍率为NUM_LEVEL_MULTI=T
    // 当L被填满时(该Level出现了T个component), 该层的T个component会合并为一个新的component(所以是T倍), 进入L+1
    Tiering
};

struct CONFIG {
    // MemTable的大小
    static inline std::size_t NUM_MEM_ENTRY = 4;
    // SST的大小
    static inline std::size_t NUM_SST_ENTRY = 4;
    // MemTable的最大数量(一些immutable_memtables和一个mem_table, 达到后阻塞前台进行写入)
    static inline std::size_t NUM_MAX_MEM_TABLE = 2;
    // LO最大大小(达到后阻塞MemTable进行flush)
    static inline std::size_t NUM_MAX_L0_SST = 3;
    // L1最大大小达到后阻塞L0往L1进行Compact)
    // static inline std::size_t NUM_MAX_L1_SST = 5;
    // 从L1开始每一层放大(下一层的文件数量为上一层的倍数)
    static inline std::size_t NUM_LEVEL_MULTI = 5;
    // 最大层数
    static inline std::size_t NUM_LEVELS = 7;

    static inline CompactType compact_type = CompactType::Tiering;

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