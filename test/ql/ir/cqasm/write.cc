#include "ql/ir/cqasm/write.h"

#include <gmock/gmock.h>
#include <compare>  // strong_ordering

using namespace ql::ir::cqasm;


TEST(version_compare, v12_equals_v12) { EXPECT_EQ(version_compare({1, 2}, {1, 2}), std::strong_ordering::equal); }
TEST(version_compare, v1_equals_v10) { EXPECT_EQ(version_compare({1}, {1, 0}), std::strong_ordering::equal); }
TEST(version_compare, v10_equals_v1) { EXPECT_EQ(version_compare({1, 0}, {1}), std::strong_ordering::equal); }

TEST(version_compare, v1_less_than_v12) { EXPECT_EQ(version_compare({1}, {1, 2}), std::strong_ordering::less); }
TEST(version_compare, v1_less_than_v3) { EXPECT_EQ(version_compare({1}, {3}), std::strong_ordering::less); }
TEST(version_compare, v1_less_than_v30) { EXPECT_EQ(version_compare({1}, {3, 0}), std::strong_ordering::less); }
TEST(version_compare, v10_less_than_v12) { EXPECT_EQ(version_compare({1, 0}, {1, 2}), std::strong_ordering::less); }
TEST(version_compare, v10_less_than_v3) { EXPECT_EQ(version_compare({1, 0}, {3}), std::strong_ordering::less); }
TEST(version_compare, v10_less_than_v30) { EXPECT_EQ(version_compare({1, 0}, {3, 0}), std::strong_ordering::less); }

TEST(version_compare, v12_more_than_v1) { EXPECT_EQ(version_compare({1, 2}, {1}), std::strong_ordering::greater); }
TEST(version_compare, v3_more_than_v1) { EXPECT_EQ(version_compare({3}, {1}), std::strong_ordering::greater); }
TEST(version_compare, v30_more_than_v1) { EXPECT_EQ(version_compare({3, 0}, {1}), std::strong_ordering::greater); }
TEST(version_compare, v12_more_than_v10) { EXPECT_EQ(version_compare({1, 2}, {1, 0}), std::strong_ordering::greater); }
TEST(version_compare, v3_more_than_v10) { EXPECT_EQ(version_compare({3}, {1, 0}), std::strong_ordering::greater); }
TEST(version_compare, v30_more_than_v10) { EXPECT_EQ(version_compare({3, 0}, {1, 0}), std::strong_ordering::greater); }
