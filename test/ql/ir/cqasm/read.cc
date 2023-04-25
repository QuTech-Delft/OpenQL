#include "ql/arch/factory.h"
#include "ql/arch/none/info.h"
#include "ql/ir/ir.gen.h"
#include "ql/ir/compat/platform.h"
#include "ql/ir/cqasm/read.h"
#include "ql/rmgr/manager.h"

#include <gmock/gmock.h>
#include <gtest/gtest.h>


struct CompatPlatformFake : public ql::ir::compat::Platform {};

struct FactoryFake : public ql::rmgr::Factory {};

struct ResourceManagerFake : public ql::rmgr::Manager {
    explicit ResourceManagerFake(
        const ql::ir::compat::PlatformRef &platform,
        const ql::utils::Str &architecture = "",
        const ql::utils::Set<ql::utils::Str> &dnu = {},
        const ql::rmgr::Factory &factory = {},
        const ql::ir::Ref &ir = {}
    ) : ql::rmgr::Manager{ platform, architecture, dnu, factory, ir } {}
};

struct PlatformFake : public ql::ir::Platform {
    PlatformFake() {
        name = "platform_fake";

        auto creg_count = 1;
        auto qubit_count = 1;

        // Notice data_types and objects vectors have to be sorted
        // Thus, adding elements in order: bit, int and qubit; and creg and q, respectively
        auto bit_type = ql::utils::make<ql::ir::BitType>("bit").template as<ql::ir::DataType>();
        data_types.get_vec().push_back(bit_type);

        auto int_type = ql::utils::make<ql::ir::IntType>("int", true, 32).template as<ql::ir::DataType>();
        data_types.get_vec().push_back(int_type);
        auto physical_object = ql::utils::make<ql::ir::PhysicalObject>("creg", int_type, ql::ir::prim::UIntVec(creg_count));
        objects.get_vec().push_back(physical_object);

        auto qubit_type = ql::utils::make<ql::ir::QubitType>("qubit").template as<ql::ir::DataType>();
        data_types.get_vec().push_back(qubit_type);
        physical_object = ql::utils::make<ql::ir::PhysicalObject>("q", qubit_type, ql::ir::prim::UIntVec(qubit_count));
        objects.get_vec().push_back(physical_object);
        qubits = physical_object;

        implicit_bit_type = bit_type;
        default_bit_type = bit_type;
        default_int_type = int_type;

        auto top = ql::com::Topology(qubit_count, ql::utils::Json{ R"({
            "number_of_cores":  1,
            "connectivity": "full",
            "form": "irregular",
            "comm_qubits_per_core": 4
        })" });
        topology.populate(top);

        auto arch = ql::arch::Factory().build_from_namespace("none");
        architecture.populate(arch);

        auto rmgr_fake = ResourceManagerFake{ ql::utils::One<CompatPlatformFake>{}, "", {}, FactoryFake{}, {}};
        resources.populate(rmgr_fake);
    }
};

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
    ir->platform = ql::utils::make<PlatformFake>();
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
    ir->platform = ql::utils::make<PlatformFake>();
    ql::ir::cqasm::ReadOptions options{};
    ql::ir::cqasm::read(ir, data, fname, options);
    EXPECT_THAT(ir->program->name, "program");
}

TEST(read, no_version) {
    const ql::utils::Str data{};
    const ql::utils::Str fname{};
    ql::ir::Ref ir{};
    ir.emplace();
    ir->platform = ql::utils::make<PlatformFake>();
    ql::ir::cqasm::ReadOptions options{};
    EXPECT_THROW(ql::ir::cqasm::read(ir, data, fname, options), std::runtime_error);
}
TEST(read, version_abc) {
    const ql::utils::Str data{ "version abc" };
    const ql::utils::Str fname{};
    ql::ir::Ref ir{};
    ir.emplace();
    ir->platform = ql::utils::make<PlatformFake>();
    ql::ir::cqasm::ReadOptions options{};
    EXPECT_THROW(ql::ir::cqasm::read(ir, data, fname, options), std::runtime_error);
}
TEST(read, version_1_1_abc) {
    const ql::utils::Str data{ "version 1.1.abc" };
    const ql::utils::Str fname{};
    ql::ir::Ref ir{};
    ir.emplace();
    ir->platform = ql::utils::make<PlatformFake>();
    ql::ir::cqasm::ReadOptions options{};
    EXPECT_THROW(ql::ir::cqasm::read(ir, data, fname, options), std::runtime_error);
}
