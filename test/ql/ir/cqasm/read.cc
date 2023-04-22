#include "ql/ir/cqasm/read.h"

#include <gtest/gtest.h>


TEST(read, version_1_0) {}
TEST(read, version_1_1) {}
TEST(read, version_1_2) {}
TEST(read, version_3_0) {}

TEST(read, no_version) {}
TEST(read, version_1_3) {}
TEST(read, version_2_0) {}
TEST(read, version_abc) {}
TEST(read, version_1_1_1) {}
TEST(read, version_1_1_abc) {}
