#include <gtest/gtest.h>
#include "log.h"
#include "lsm.h"
#include <config.h>
#include <string>

TEST(LSMTest, Basic) {
    LSM<int, std::string> lsm;

    lsm.set(1, "value1");
    lsm.set(2, "value2");
    lsm.set(3, "value3");

    auto result1 = lsm.get(1);
    auto result2 = lsm.get(2);
    auto result3 = lsm.get(3);
    auto result4 = lsm.get(999);

    ASSERT_TRUE(result1.has_value());
    ASSERT_TRUE(result2.has_value());
    ASSERT_TRUE(result3.has_value());
    ASSERT_FALSE(result4.has_value());

    EXPECT_EQ(result1.value(), "value1");
    EXPECT_EQ(result2.value(), "value2");
    EXPECT_EQ(result3.value(), "value3");
}

TEST(LSMTest, MemTableFlushBig) {
    LSM<int, int> lsm;

    for (int i = 1; i <= 100000; ++i) {
        lsm.set(i, i * 100);
    }

    for (int i = 1; i <= 100000; ++i) {
        auto result = lsm.get(i);
        ASSERT_TRUE(result.has_value());
        EXPECT_EQ(result.value(), i * 100);
    }
}

TEST(LSMTest, CompactionBasic) {
    LSM<int, int> lsm;

    for (int i = 1; i <= 20; ++i) {
        lsm.set(i, i * 10);
    }

    auto r1 = lsm.get(1);
    auto r2 = lsm.get(20);
    ASSERT_TRUE(r1.has_value());
    ASSERT_TRUE(r2.has_value());
    EXPECT_EQ(r1.value(), 10);
    EXPECT_EQ(r2.value(), 200);
}

TEST(LSMTest, UpdateBasic) {
    LSM<int, std::string> lsm;

    lsm.set(1, "initial");
    lsm.set(2, "value2");
    lsm.set(3, "value3");

    for (int i = 4; i <= 8; ++i) {
        lsm.set(i, "dummy");
    }

    lsm.set(1, "updated");
    lsm.set(2, "updated2");

    auto r1 = lsm.get(1);
    auto r2 = lsm.get(2);
    auto r3 = lsm.get(3);

    ASSERT_TRUE(r1.has_value());
    ASSERT_TRUE(r2.has_value());
    ASSERT_TRUE(r3.has_value());

    EXPECT_EQ(r1.value(), "updated");
    EXPECT_EQ(r2.value(), "updated2");
    EXPECT_EQ(r3.value(), "value3");
}

int main(int argc, char **argv) {
    ::testing::InitGoogleTest(&argc, argv);
    INIT_LOGGER();
    init_all_config();
    return RUN_ALL_TESTS();
}