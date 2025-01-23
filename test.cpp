#include <gtest/gtest.h>

#include "common.h"


// 示例函数
int Add(int a, int b) {
    return a + b;
}



// 测试用例
TEST(AddTest, PositiveNos) {
    EXPECT_EQ(Add(1, 2), 3);
    EXPECT_EQ(Add(100, 200), 300);
    Rect r1{98, 276, 98, 2275};
    Rect r2{97, 276, 97, 2275};
    EXPECT_TRUE(r1.nearby(r2, 1));
}

int main(int argc, char **argv) {
    ::testing::InitGoogleTest(&argc, argv);
    return RUN_ALL_TESTS();
}