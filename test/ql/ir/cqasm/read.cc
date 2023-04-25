#include "ql/ir/ir.h"
#include "ql/ir/compat/platform.h"
#include "ql/ir/cqasm/read.h"
#include "ql/ir/old_to_new.h"

#include <gmock/gmock.h>
#include <gtest/gtest.h>


/*
TEST(read, version_1_0) {
    const ql::utils::Str data{ "version 1.0" };
    const ql::utils::Str fname{};
    ql::ir::Ref ir{};
    ir.emplace();
    ir->platform.emplace();
    ql::ir::cqasm::ReadOptions options{};
    ql::ir::cqasm::read(ir, data, fname, options);
    EXPECT_THAT(ir->program->name, "program");
}
TEST(read, version_1_1) {
    const ql::utils::Str data{ "version 1.1" };
    const ql::utils::Str fname{};
    auto pres = ql::ir::cqasm::read(data, fname);
    EXPECT_TRUE(pres.errors.empty());
    auto prog = pres.root->as_program();
    EXPECT_THAT(prog->version->items, ::testing::ElementsAre(1, 1));
}
TEST(read, version_1_1_1) {
    const ql::utils::Str data{ "version 1.1.1" };
    const ql::utils::Str fname{};
    auto pres = ql::ir::cqasm::read(data, fname);
    EXPECT_TRUE(pres.errors.empty());
    auto prog = pres.root->as_program();
    EXPECT_THAT(prog->version->items, ::testing::ElementsAre(1, 1, 1));
}
*/

TEST(read, version_1_2) {
    const ql::utils::Str data{ "version 1.2" };
    const ql::utils::Str fname{};
    ql::ir::Ref ir{};
    ir.emplace();
    auto platform = ql::ir::compat::Platform::build(ql::utils::Str("none"), ql::utils::Str("none"));
    // TODO:
    // convert_old_to_new causes read to be called twice
    // It doesn't make sense to test a function and call it twice during the test setup
    // Have a look into using a platform mock
    ir->platform = ql::ir::convert_old_to_new(platform)->platform;
    ql::ir::cqasm::ReadOptions options{};
    ql::ir::cqasm::read(ir, data, fname, options);
    EXPECT_THAT(ir->program->name, "program");
}
/*
TEST(read, version_1_3) {
    const ql::utils::Str data{ "version 1.3" };
    const ql::utils::Str fname{};
    auto pres = ql::ir::cqasm::read(data, fname);
    EXPECT_TRUE(pres.errors.empty());
    auto prog = pres.root->as_program();
    EXPECT_THAT(prog->version->items, ::testing::ElementsAre(1, 3));
}
TEST(read, version_2_0) {
    const ql::utils::Str data{ "version 2.0" };
    const ql::utils::Str fname{};
    auto pres = ql::ir::cqasm::read(data, fname);
    EXPECT_TRUE(pres.errors.empty());
    auto prog = pres.root->as_program();
    EXPECT_THAT(prog->version->items, ::testing::ElementsAre(2, 0));
}
*/
TEST(read, version_3_0) {
    const ql::utils::Str data{ "version 3.0" };
    const ql::utils::Str fname{};
    ql::ir::Ref ir{};
    ir.emplace();
    auto platform = ql::ir::compat::Platform::build(ql::utils::Str("none"), ql::utils::Str("none"));
    ir->platform = ql::ir::convert_old_to_new(platform)->platform;
    ql::ir::cqasm::ReadOptions options{};
    ql::ir::cqasm::read(ir, data, fname, options);
    EXPECT_THAT(ir->program->name, "program");
}

TEST(read, no_version) {
    const ql::utils::Str data{};
    const ql::utils::Str fname{};
    ql::ir::Ref ir{};
    ir.emplace();
    auto platform = ql::ir::compat::Platform::build(ql::utils::Str("none"), ql::utils::Str("none"));
    ir->platform = ql::ir::convert_old_to_new(platform)->platform;
    ql::ir::cqasm::ReadOptions options{};
    EXPECT_THROW(ql::ir::cqasm::read(ir, data, fname, options), std::runtime_error);
}
TEST(read, version_abc) {
    const ql::utils::Str data{ "version abc" };
    const ql::utils::Str fname{};
    ql::ir::Ref ir{};
    ir.emplace();
    auto platform = ql::ir::compat::Platform::build(ql::utils::Str("none"), ql::utils::Str("none"));
    ir->platform = ql::ir::convert_old_to_new(platform)->platform;
    ql::ir::cqasm::ReadOptions options{};
    EXPECT_THROW(ql::ir::cqasm::read(ir, data, fname, options), std::runtime_error);
}
TEST(read, version_1_1_abc) {
    const ql::utils::Str data{ "version 1.1.abc" };
    const ql::utils::Str fname{};
    ql::ir::Ref ir{};
    ir.emplace();
    auto platform = ql::ir::compat::Platform::build(ql::utils::Str("none"), ql::utils::Str("none"));
    ir->platform = ql::ir::convert_old_to_new(platform)->platform;
    ql::ir::cqasm::ReadOptions options{};
    EXPECT_THROW(ql::ir::cqasm::read(ir, data, fname, options), std::runtime_error);
}
