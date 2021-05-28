/** \file
 * Defines the pass for generation the microcode for the Fujitsu project quantum
 * computer
 */

#include "ql/arch/diamond/pass/gen/microcode/microcode.h"
#include "ql/arch/diamond/pass/gen/microcode/detail/functions.h"
#include "ql/arch/diamond/annotations.h"

#include "ql/pmgr/pass_types/base.h"

#include "ql/utils/str.h"
#include "ql/utils/filesystem.h"
#include "ql/plat/platform.h"
#include "ql/com/options.h"

namespace ql {
namespace arch {
namespace diamond {
namespace pass {
namespace gen {
namespace microcode {

using namespace utils;

/**
 * Dumps docs for the code generator
 */
void GenerateMicrocodePass::dump_docs(
    std::ostream &os,
    const utils::Str &line_prefix
) const {
    utils::dump_str(os, line_prefix, R"(
    Generates the microcode from the algorithm (cQASM/C++/Python) description
    for quantum computing in diamond.
    )");
}

utils::Str GenerateMicrocodePass::get_friendly_type() const {
    return "Diamond microcode generator";
}

GenerateMicrocodePass::GenerateMicrocodePass(
    const utils::Ptr<const pmgr::Factory> &pass_factory,
    const utils::Str &instance_name,
    const utils::Str &type_name
) : pmgr::pass_types::ProgramTransformation(pass_factory, instance_name, type_name) {

}

utils::Int GenerateMicrocodePass::run(
    const ir::ProgramRef &program,
    const pmgr::pass_types::Context &context
) const {
    // General Idea: Make a big case statement with all the different options that
    // cQASM provides. Then, decide for each option what to write to the output file.

    // Specify output file name
    Str file_name(context.output_prefix + ".dqasm");

    // Specify the outfile name
    OutFile outfile{file_name};

    // Copy the kernel into a new kernel, add the necessary gates before and in between the existing gates.
    for (const auto &kernel : program->kernels) {
        ir::Kernel temp_kernel(
            "dummy",
            program->platform,
            kernel->qubit_count,
            kernel->creg_count,
            kernel->breg_count
        );

        // For every qubit, insert a magnetic bias check
        for (UInt q = 0; q < kernel->qubit_count; q++) {

            temp_kernel.gate("sweep_bias", q);
            temp_kernel.gates.back()->set_annotation<ql::arch::diamond::annotations::SweepBiasParameters>(
                {10, q, 0, 10, 100, 0});
            temp_kernel.gate("calculate_current", q);
        }

        // For every qubit, inset a rabi check
        for (UInt q = 0; q < kernel->qubit_count; q++) {
            // rabi check

        }

        // For every qubit, insert a CRC
        for (UInt q = 0; q < kernel->qubit_count; q++) {

            temp_kernel.gate("crc", q);
            temp_kernel.gates.back()->set_annotation<ql::arch::diamond::annotations::CRCParameters>({5, q});
        }

        // For every qubit, initialize
        for (UInt q = 0; q < kernel->qubit_count; q++) {

            temp_kernel.gate("initialize", q);
        }

        UInt number_gates = 0;
        for (const auto &gate : kernel->gates) {
            if (number_gates > 9) {
                for (UInt q = 0; q < kernel->qubit_count; q++) {

                    temp_kernel.gate("crc", q);
                    temp_kernel.gates.back()->set_annotation<ql::arch::diamond::annotations::CRCParameters>({5, q});
                }
                number_gates = 0;
            }

            temp_kernel.gates.add(gate);
            number_gates++;
        }

        kernel->gates = std::move(temp_kernel.gates);
        kernel->cycles_valid = false;
    }

    // Make global variables for keeping track of registers, labels etc.
    int labelcount = 0;

    for (const ir::KernelRef &kernel : program->kernels) {
        for (const ir::GateRef &gate : kernel->gates) {
                const auto &data = program->platform->find_instruction(gate->name);

            // Determine gate type.
            utils::Str type = "unknown";
            auto iterator = data.find("diamond_type");
            if (iterator != data.end() && iterator->is_string()) {
                type = iterator->get<utils::Str>();
            }

            // Determine the microcode output for the given gate-type
            if (type == "qgate") {
                outfile << detail::qgate(gate->name, gate->operands);
            } else if (type == "qgate2") {
                Str op_1 = "q" + to_string(gate->operands[0]);
                Str op_2 = "q" + to_string(gate->operands[1]);
                outfile << detail::qgate2(gate->name, op_1, op_2);
            } else if (type == "classical") {
                 if (gate->name == "calculate_current"){
                     outfile << "calculate_current()" << "\n";
                 }
                 else if (gate->name == "calculate_voltage") {
                     outfile << "calculate_voltage()" << "\n";
                 }
            } else if (type == "initial_checks") {
                if (gate->name == "mag_bias") {
                    // Code for magnetic biasing
                    // Not added because it is decomposed into sweep_bias and calculate_current()
                    // at lines 76-79 of microcode.cc
                } else if (gate->name == "rabi_check") {
                    // Code for rabi check
                } else if (gate->name == "crc") {
                    const auto &params = gate->get_annotation<annotations::CRCParameters>();

                    Str qubit_number = to_string(gate->operands[0]);
                    Str count = to_string(labelcount);
                    Str count2 = to_string(labelcount+1);

                    outfile << detail::loadimm(to_string(params.threshold), "treshReg", qubit_number) << "\n";
                    outfile << detail::loadimm(to_string(params.value), "dacReg", qubit_number) << "\n";

                    outfile << detail::label(count) << "\n";
                    outfile << detail::loadimm("0", "photon Reg", qubit_number) << "\n";
                    outfile << detail::switchOn(gate->operands[0]) << "\n";
                    outfile << detail::excite_mw("1", "100", "200", "0", gate->operands[0]) << "\n";
                    outfile << detail::mov("photonReg", qubit_number, "R",qubit_number) << "\n";
                    outfile << detail::switchOff(gate->operands[0]) << "\n";
                    outfile << detail::branch("R", qubit_number, ">", "treshReg",
                                              qubit_number, "LAB",
                                              count2) << "\n";
                    outfile << "calculateVoltage()" << "\n";
                    outfile << detail::jump(count) << "\n";
                    outfile << detail::label(count2);

                    // Add 2 to labelcount because label was used twice.
                    labelcount++;
                    labelcount++;
                }
            } else {
                if (gate->name == "measure") {
                    Str qubit_number = to_string(gate->operands[0]);
                    const Str threshold = "33";

                    outfile << detail::switchOn(gate->operands[0]) << "\n";
                    outfile << detail::loadimm("0", "photonReg", qubit_number) << "\n";
                    outfile << detail::excite_mw("1", "100", "200", "0", gate->operands[0]) << "\n";
                    outfile << detail::mov("photonReg", qubit_number, "R",qubit_number) << "\n";
                    outfile << detail::switchOff(gate->operands[0]) << "\n";
                    outfile << detail::branch("R", qubit_number, "<", "R",
                                              threshold, "ResultReg",
                                              qubit_number);
                } else if (gate->name == "initialize") {
                    Str qubit_number = to_string(gate->operands[0]);
                    const Str threshold = "0";
                    Str count = to_string(labelcount);


                    outfile << detail::label(count) << "\n";
                    outfile << detail::switchOn(gate->operands[0]) << "\n";
                    outfile << detail::loadimm("0", "photonReg", qubit_number) << "\n";
                    outfile << detail::excite_mw("1", "100", "200", "0", gate->operands[0]) << "\n";
                    outfile << detail::mov("photonReg", qubit_number, "R",
                                           qubit_number) << "\n";
                    outfile << detail::switchOff(gate->operands[0]) << "\n";
                    outfile << detail::branch("R", qubit_number, "<", "", threshold,
                                          "LAB", count);

                    labelcount++;
                } else if (gate->name == "wait") {
                    outfile << "wait " << gate->operands.to_string("", ", ", "");
                } else if (gate->name == "qnop") {
                    outfile << "wait 1" << "\n";
                } else if (gate->name == "sweep_bias") {

                    const auto &params = gate->get_annotation<annotations::SweepBiasParameters>();

                    Str qubit_number = to_string(gate->operands[0]);
                    Str count = to_string(labelcount);

                    outfile << detail::loadimm(to_string(params.value), "dacReg",
                                               to_string(params.dacreg)) << "\n";
                    outfile << detail::loadimm(to_string(params.start), "sweepStartReg", qubit_number)<< "\n";
                    outfile << detail::loadimm(to_string(params.step), "sweepStepReg", qubit_number) << "\n";
                    outfile << detail::loadimm(to_string(params.max), "sweepStopReg", qubit_number) << "\n";
                    outfile << detail::loadimm(to_string(params.memaddress), "memAddr", qubit_number) << "\n";
                    outfile << detail::label(count) << "\n";
                    outfile << detail::switchOn(gate->operands[0]) << "\n";
                    outfile << detail::excite_mw("1", "100", "sweepStartReg"+qubit_number, "0", gate->operands[0]) << "\n";
                    outfile << detail::switchOff(gate->operands[0]) << "\n";
                    outfile << detail::mov("photonReg", qubit_number, "R", qubit_number) << "\n";
                    outfile << detail::store("R", qubit_number, "memAddr", qubit_number,
                                             "0") << "\n";
                    outfile << detail::store("sweepStartReg", qubit_number, "memAddr", qubit_number, "0") << "\n";
                    outfile << detail::add("sweepStartReg", qubit_number, "sweepStartReg", qubit_number, "sweepStepReg", qubit_number) << "\n";
                    outfile << detail::addimm("4", "memAddr", qubit_number) << "\n";
                    outfile << detail::branch("sweepStartReg", qubit_number, ">", "sweepStopReg", qubit_number, "LAB", count);
                    labelcount++;
                } else if (gate->name == "excite_mw") {
                    const auto &params = gate->get_annotation<annotations::ExciteMicrowaveParameters>();

                    outfile << detail::excite_mw(to_string(params.envelope), to_string(params.duration), to_string(params.frequency), to_string(params.phase), gate->operands[0]);
                } else if (gate->name == "memswap") {
                    const auto &params = gate->get_annotation<annotations::MemSwapParameters>();

                    Str nuq = "nuq" + to_string(params.nuclear);
                    Str qubit = "q" + to_string(gate->operands[0]);
                    outfile << detail::qgate2("pmy90",qubit, nuq) << "\n";
                    outfile << detail::qgate("x90", gate->operands) << "\n";
                    outfile << detail::qgate2("pmx90", qubit, nuq) << "\n";
                    outfile << detail::qgate("my90", gate->operands);
                } else if (gate->name == "qentangle") {
                    const auto &params = gate->get_annotation<annotations::QEntangleParameters>();

                    Str nuq = "nuq" + to_string(params.nuclear);
                    Str qubit = "q" + to_string(gate->operands[0]);
                    outfile << detail::qgate("mx90", gate->operands) << "\n";
                    outfile << detail::qgate2("pmx90", qubit, nuq) << "\n";
                    outfile << detail::qgate("x90", gate->operands);
                } else {
                    outfile << "The name of the gate was not recognized";
                }
            }
            outfile << "\n" << "\n";
        }
    }
    return 0;
}

} // namespace microcode
} // namespace gen
} // namespace pass
} // namespace diamond
} // namespace arch
} // namespace ql
