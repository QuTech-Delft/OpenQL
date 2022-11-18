/*
This test checks the correctness of the circuit transformation done by the router pass.

It does not test whether routing is done optimally, based on the various options.
*/

#define DOCTEST_CONFIG_IMPLEMENT_WITH_MAIN
#include "doctest/doctest.h"

#include "ql/pass/map/qubits/map/map.h"
#include "ql/ir/ops.h"
#include "ql/ir/cqasm/read.h"
#include "ql/ir/cqasm/write.h"
#include "ql/ir/old_to_new.h"
#include "ql/com/ddg/ops.h"
#include "ql/com/ddg/build.h"
#include "ql/com/ddg/dot.h"
#include "ql/com/map/qubit_mapping.h"
#include "ql/com/map/reference_updater.h"
#include "ql/pmgr/factory.h"

namespace ql {
namespace pass {
namespace map {
namespace qubits {
namespace map {

class MapTest {
protected:
    MapTest() {
        factory.emplace();
        mapperPass = std::unique_ptr<MapQubitsPass>(new MapQubitsPass(factory, "instance", "type"));
    };

    // Checks two DDGS for equality, assuming that no node contains 2 identical children (based on the value they contain).
    static bool ddgs_are_equal(const ir::BlockBaseRef &block1, const ir::BlockBaseRef &block2) {
        // Only works when DDGs are built with commutation rules disabled!

        std::list<ir::StatementRef> nextBlock1, nextBlock2;    

        nextBlock1.push_back(com::ddg::get_source(block1));
        nextBlock2.push_back(com::ddg::get_source(block2));

        while(!nextBlock1.empty()) {
            const auto next1 = nextBlock1.front();
            nextBlock1.pop_front();
            const auto next2 = nextBlock2.front();
            nextBlock2.pop_front();

            QL_ASSERT(next1->equals(*next2));

            const auto &succ1 = com::ddg::get_node(next1)->successors;

            const auto &succ2 = com::ddg::get_node(next2)->successors;

            if (succ1.size() != succ2.size()) {
                return false;
            }

            for (const auto& node_edge1: succ1) {
                nextBlock1.push_back(node_edge1.first);

                bool found = false;
                for (const auto& node_edge2: succ2) {
                    if (node_edge2.first->equals(*node_edge1.first)) {
                        QL_ASSERT(!found); // This fails when commutation rules are enabled
                        found = true;
                        nextBlock2.push_back(node_edge2.first);
                    }
                }

                if (!found) {
                    return false;
                }
            }
        }

        return true;
    };

    /*
     * Check whether two single-block programs have the same gates modulo a permutation that
     * respect topological order. That is, the two blocks have the same DDG. Removes cycle numbers.
     */
    static void checkCircuitSemanticsAreTheSame(const ir::Ref &ir1, const ir::Ref &ir2) {
        REQUIRE(!ir1.empty());
        REQUIRE(!ir2.empty());
        REQUIRE(!ir1->platform.empty());
        REQUIRE(!ir2->platform.empty());
        CHECK(ir1->platform->equals(*ir2->platform));
        REQUIRE_EQ(ir1->program->blocks.size(), 1);
        REQUIRE_EQ(ir2->program->blocks.size(), 1);

        com::ddg::clear(ir1->program->blocks[0]);
        com::ddg::clear(ir2->program->blocks[0]);

        removeAssignedCycles(ir1->program->blocks[0]);
        removeAssignedCycles(ir2->program->blocks[0]);

        com::ddg::build(ir1->platform, ir1->program->blocks[0], false, false);
        com::ddg::build(ir2->platform, ir2->program->blocks[0], false, false);

        std::stringstream ir1dot;
        com::ddg::dump_dot(ir1->program->blocks[0], ir1dot);

        CAPTURE(ir1dot.str());

        std::stringstream ir2dot;
        com::ddg::dump_dot(ir2->program->blocks[0], ir2dot);
        
        CAPTURE(ir2dot.str());

        CHECK(ddgs_are_equal(ir1->program->blocks[0], ir2->program->blocks[0]));
    }

    static void deswapCircuit(const ir::Ref &ir) {
        REQUIRE(ir->program->blocks.size() == 1);

        std::vector<utils::UInt> r2v;
        r2v.resize(ir::get_num_qubits(ir->platform));

        for (utils::UInt i = 0; i < r2v.size(); ++i) {
            r2v[i] = i;
        }

        utils::Any<ir::Statement> output_statements;

        for (const auto& st: ir->program->blocks[0]->statements) {
            auto custom_instr = st.as<ir::CustomInstruction>();

            REQUIRE(!custom_instr.empty());
            
            if (custom_instr->instruction_type->name == "swap") { // or tswap
                ir::OperandsHelper ops(ir->platform, *custom_instr);
                auto qops = ops.get2QGateOperands();

                std::swap(r2v[qops.first], r2v[qops.second]);

                continue;
            }

            if (custom_instr->instruction_type->name == "move") { // or tmove
                ir::OperandsHelper ops(ir->platform, *custom_instr);
                auto qops = ops.get2QGateOperands();

                std::swap(r2v[qops.first], r2v[qops.second]);

                continue;
            }

            auto custom_instr_cloned = custom_instr.clone();

            com::map::mapInstruction(ir->platform, r2v, custom_instr_cloned);

            output_statements.add(custom_instr_cloned);
        }

        ir->program->blocks[0]->statements = output_statements;
    }

    // This test does not work when there is some initial permutation of the qubits performed by the pass.
    // This can occur when some 2q gate uses qubits which were unused so far (state not "live").
    void runAndCheck(const std::string &circuit) {
        INFO("Input cqasm circuit: \n", circuit);
        auto input = read(circuit);

        auto output = run(input);

        const auto mapped = ir::cqasm::to_string(output, output);
        INFO("Mapped cqasm circuit: \n", mapped);

        deswapCircuit(output);

        const auto deswapped = ir::cqasm::to_string(output, output);
        INFO("Deswapped cqasm circuit: \n", deswapped);

        checkCircuitSemanticsAreTheSame(input, output);
    }

    ir::Ref read(const std::string &circuit) {
                auto platform = ir::cqasm::read_platform(circuit);
        auto input = ir::convert_old_to_new(platform);

        ir::cqasm::read(input, circuit);

        return input;
    }

    ir::Ref run(ir::Ref input) {
        utils::List<pmgr::pass_types::Ref> passes;
        pmgr::condition::Ref cond;
        mapperPass->on_construct(factory, passes, cond);

        auto output = input.clone();
        const utils::Options opts;
        pmgr::pass_types::Context ctx{"myPass", "outputPrefix", opts};
        mapperPass->run(output, ctx);
        
        return output;
    }

    void set_option(std::string opt, std::string value) {
        mapperPass->set_option(opt, value);
    }

 private:
    static void removeAssignedCycles(const ir::BlockBaseRef &block) {
        for (const auto &st: block->statements) {
            st->cycle = 0;
        }
    }

    pmgr::CFactoryRef factory;
    std::unique_ptr<MapQubitsPass> mapperPass;
};

TEST_CASE_FIXTURE(MapTest, "3q gate throws") {
    auto circuit = R"(
version 1.2

pragma @ql.platform("cc_light.s7")

toffoli q[1], q[2], q[3]
)";

    auto input = read(circuit);

    std::string exceptionMessage = "";
    try {
        run(input);
    } catch (utils::Exception const& e) {
        exceptionMessage = e.what();
    }
    CAPTURE(exceptionMessage);
    CHECK_EQ(exceptionMessage.rfind("Internal compiler error: Mapper/router does not handle gates with more than 2 qubits operands.", 0), 0);
}

TEST_CASE_FIXTURE(MapTest, "This test class does not handle swaps that don't end up in the circuit") {
    auto circuit = R"(
version 1.2

pragma @ql.platform("cc_light.s7")

cnot q[4], q[3]
)";

    auto input = read(circuit);
    auto output = run(input);

    REQUIRE_EQ(output->program->blocks[0]->statements.size(), 1);

    auto instr = output->program->blocks[0]->statements[0].as<ir::CustomInstruction>();
    REQUIRE(!instr.empty());

    CHECK_EQ(instr->instruction_type->name, "cnot");

    ir::OperandsHelper ops(output->platform, *instr);
    auto qops = ops.get2QGateOperands();

    CHECK_EQ(qops.first, 4);
    CHECK_EQ(qops.second, 1);
}

TEST_CASE_FIXTURE(MapTest, "Simple S7") {
    auto circuit = R"(
version 1.2

pragma @ql.platform("cc_light.s7")

h q[3]
h q[4]
h q[6]
cnot q[4], q[3]
cnot q[3], q[4]
cnot q[3], q[6]
h q[3]
)";

    runAndCheck(circuit);
}

TEST_CASE_FIXTURE(MapTest, "Simple S7 bis") {
    auto circuit = R"(
version 1.2

pragma @ql.platform("cc_light.s7")

x q[3]
x q[4]
cnot q[3], q[4]
cnot q[3], q[4]
)";

    runAndCheck(circuit);
}

TEST_CASE_FIXTURE(MapTest, "A lot of czs on S7") {
    SUBCASE("") {
        set_option("route_heuristic", "base");
    }
    SUBCASE("") {
        set_option("route_heuristic", "minextend");
    }

    std::stringstream circuit;
    circuit << R"(
version 1.2

pragma @ql.platform("cc_light.s7")

x q[0]
x q[1]
x q[2]
x q[3]
x q[4]
x q[6]
)";

    for (auto i = 0; i < 7; ++i) {
        for (auto j = 0; j < 7; ++j) {
            if (i == j) {
                continue;
            }

            circuit << std::endl << "cz q[" << i << "], q[" << j << "]";
        }
    }

    runAndCheck(circuit.str());
}

TEST_CASE_FIXTURE(MapTest, "A lot of cnots on S17") {
    SUBCASE("") {
        set_option("route_heuristic", "base");
    }
    SUBCASE("") {
        set_option("route_heuristic", "minextend");
    }

    std::stringstream circuit;
    circuit << R"(
version 1.2

pragma @ql.platform("cc_light.s17")

)";

    // Make all qubits live from the start by applying a 1q gate, to avoid "silent" swaps.
    for (auto i = 0; i < 17; ++i) {
        circuit << std::endl << "x q[" << i << "]";
    }

    for (auto c = 0; c < 2; ++c) {
        for (utils::Int i = 5; i >= 0; --i) {
            for (auto j = 0; j < 3; ++j) {
                if (i == j) {
                    continue;
                }

                circuit << std::endl << "cnot q[" << i << "], q[" << j << "]";
            }
        }
    }

    runAndCheck(circuit.str());
}

// TEST_CASE_FIXTURE(MapTest, "Criticality") {
//     auto circuit = R"(
// version 1.2

// pragma @ql.platform("cc_light.s7")

// x q[0]
// x q[1]
// x q[2]
// x q[3]
// x q[4]
// x q[5]
// x q[6]
// cnot q[0], q[5]
// cnot q[3], q[4]
// x q[3]
// x q[3]
// x q[3]
// )";

//     auto expected = R"(# Generated by OpenQL 0.11.1 for program program
// version 1.2

// pragma @ql.name("program")

// .entry @ql.entry()
//     goto __1

// .__2 @ql.name("")
//     x q[0]
//     x q[1]
//     x q[2]
//     x q[3]
//     x q[4]
//     x q[5]
//     x q[6]
//     swap q[4], q[6]
//     cnot q[3], q[6]
//     x q[3]
//     x q[3]
//     x q[3]
//     swap q[5], q[2]
//     cnot q[0], q[2]
//     skip 8)";
//     auto input = read(circuit);
    
//     set_option("route_heuristic", "base");

//     auto output = run(input);

//     const auto mapped = ir::cqasm::to_string(output, output);
//     CHECK_EQ(mapped, expected);
// }


}
}
}
}
}