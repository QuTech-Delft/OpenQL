#include "ql/ir/cqasm/read.h"

#include <gmock/gmock.h>
#include <gtest/gtest.h>


TEST(parse, version_1_0) {
    const ql::utils::Str data{ "version 1.0" };
    const ql::utils::Str fname{};
    auto pres = ql::ir::cqasm::parse(data, fname);
    EXPECT_TRUE(pres.errors.empty());
    auto prog = pres.root->as_program();
    EXPECT_THAT(prog->version->items, ::testing::ElementsAre(1, 0));
}
TEST(parse, version_1_1) {
    const ql::utils::Str data{ "version 1.1" };
    const ql::utils::Str fname{};
    auto pres = ql::ir::cqasm::parse(data, fname);
    EXPECT_TRUE(pres.errors.empty());
    auto prog = pres.root->as_program();
    EXPECT_THAT(prog->version->items, ::testing::ElementsAre(1, 1));
}
TEST(parse, version_1_1_1) {
    const ql::utils::Str data{ "version 1.1.1" };
    const ql::utils::Str fname{};
    auto pres = ql::ir::cqasm::parse(data, fname);
    EXPECT_TRUE(pres.errors.empty());
    auto prog = pres.root->as_program();
    EXPECT_THAT(prog->version->items, ::testing::ElementsAre(1, 1, 1));
}
TEST(parse, version_1_2) {
    const ql::utils::Str data{ "version 1.2" };
    const ql::utils::Str fname{};
    auto pres = ql::ir::cqasm::parse(data, fname);
    EXPECT_TRUE(pres.errors.empty());
    auto prog = pres.root->as_program();
    EXPECT_THAT(prog->version->items, ::testing::ElementsAre(1, 2));
}
TEST(parse, version_1_3) {
    const ql::utils::Str data{ "version 1.3" };
    const ql::utils::Str fname{};
    auto pres = ql::ir::cqasm::parse(data, fname);
    EXPECT_TRUE(pres.errors.empty());
    auto prog = pres.root->as_program();
    EXPECT_THAT(prog->version->items, ::testing::ElementsAre(1, 3));
}
TEST(parse, version_2_0) {
    const ql::utils::Str data{ "version 2.0" };
    const ql::utils::Str fname{};
    auto pres = ql::ir::cqasm::parse(data, fname);
    EXPECT_TRUE(pres.errors.empty());
    auto prog = pres.root->as_program();
    EXPECT_THAT(prog->version->items, ::testing::ElementsAre(2, 0));
}
TEST(parse, version_3_0) {
    const ql::utils::Str data{ "version 3.0" };
    const ql::utils::Str fname{};
    auto pres = ql::ir::cqasm::parse(data, fname);
    EXPECT_TRUE(pres.errors.empty());
    auto prog = pres.root->as_program();
    EXPECT_THAT(prog->version->items, ::testing::ElementsAre(3, 0));
}

TEST(parse, no_version) {
    const ql::utils::Str data{};
    const ql::utils::Str fname{};
    EXPECT_THROW(ql::ir::cqasm::parse(data, fname), std::runtime_error);
}
TEST(parse, version_abc) {
    const ql::utils::Str data{ "version abc" };
    const ql::utils::Str fname{};
    EXPECT_THROW(ql::ir::cqasm::parse(data, fname), std::runtime_error);
}
TEST(parse, version_1_1_abc) {
    const ql::utils::Str data{ "version 1.1.abc" };
    const ql::utils::Str fname{};
    EXPECT_THROW(ql::ir::cqasm::parse(data, fname), std::runtime_error);
}
