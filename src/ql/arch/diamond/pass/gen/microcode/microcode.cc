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
#include "ql/ir/compat/platform.h"
#include "ql/com/options.h"
#include "ql/pmgr/factory.h"

namespace ql {
namespace arch {
namespace diamond {
namespace pass {
namespace gen {
namespace microcode {

using namespace utils;

bool GenerateMicrocodePass::is_pass_registered = pmgr::Factory::register_pass<GenerateMicrocodePass>("arch.diamond.gen.Microcode");

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
    const ir::compat::ProgramRef &program,
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
        ir::compat::Kernel temp_kernel(
            "dummy",
            program->platform,
            kernel->qubit_count,
            kernel->creg_count,
            kernel->breg_count
        );

        // Only for the first kernel

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
            temp_kernel.gate("rabi_check", q);
            temp_kernel.gates.back()->set_annotation<ql::arch::diamond::annotations::RabiParameters>({100, 2, 3});
        }

        // For every qubit, insert a CRC
        for (UInt q = 0; q < kernel->qubit_count; q++) {

            temp_kernel.gate("crc", q);
            temp_kernel.gates.back()->set_annotation<ql::arch::diamond::annotations::CRCParameters>({5, q});
        }

        // For every qubit, initialize to |0>
        for (UInt q = 0; q < kernel->qubit_count; q++) {

            temp_kernel.gate("initialize", q);
        }

        // Add the gates from the original kernel. Every 10 gates, add a CRC check
        // for all qubits.
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

        // Copy the gates to the original kernel and set the cycles_valid flag to false
        // because the cycle count is not accurate anymore.
        kernel->gates = std::move(temp_kernel.gates);
        kernel->cycles_valid = false;
    }

    // Make global variable for keeping track label numbers.
    int labelcount = 0;

    for (const ir::compat::KernelRef &kernel : program->kernels) {
        for (const ir::compat::GateRef &gate : kernel->gates) {
            utils::Json data;
            if (gate->name != "wait" && gate->name != "barrier") {
                data = program->platform->find_instruction(gate->name);
            }

            outfile << "# " << gate->qasm() << "\n";

            // Determine gate type.
            utils::Str type = "unknown";
            auto iterator = data.find("diamond_type");
            if (iterator != data.end() && iterator->is_string()) {
                type = iterator->get<utils::Str>();
            }

            // Check for condition
            Str end_label;
            if (gate->condition == ir::compat::ConditionType::UNARY) {
                end_label = to_string(labelcount++);
                outfile << detail::branch("ResultReg", to_string(gate->cond_operands[0]), "<", "", "1", "LAB", end_label) << "\n";
            } else if (gate->condition != ir::compat::ConditionType::ALWAYS) {
                throw utils::Exception("gate with " + to_string(gate->condition) + " is not supported");
            }

            // Determine the microcode output for the given gate-type
            // If the gate-type is known, check the gate name.
            // If the gate-type is not known, check the gate name
            // If the gate-name is not known, print that the gate `name` is not known.
            // Last option likely will not occur as OpenQL will throw an error
            // when running the algorithm.
            if (type == "qgate") {
                // Single qubit gate
                outfile << detail::qgate(gate->name, gate->operands[0]) << "\n";
            } else if (type == "qgate2") {
                // Two qubit gate. Not that in the diamond structure, 2 qubit gates are only possible between
                // a qubit and a nuclear spin qubit.
                Str op_1 = "q" + to_string(gate->operands[0]);
                Str op_2 = "nuq" + to_string(gate->operands[1]);
                outfile << detail::qgate2(gate->name, op_1, op_2) << "\n";
            } else if (type == "rotation") {
                // custom rotations around an axis
                UInt a = 1000 / 3.14159265359;
                Str duration = to_string(a * gate->angle);
                if (gate->name == "rx") {
                    Str phase = to_string(1.57);
                    outfile << detail::excite_mw("0", duration, "200", phase, "60",
                                                 gate->operands[0]);
                } else if (gate->name == "ry") {
                    Str phase = to_string(3.14);
                    outfile << detail::excite_mw("0", duration, "200", phase, "60",
                                                 gate->operands[0]);
                } else if (gate->name == "rz") {
                    Str phase = to_string(0);
                    outfile << detail::excite_mw("0", duration, "200", phase, "60",
                                                 gate->operands[0]);
                } else if (gate->name == "cr") {
                    Str phase = to_string(0);
                    outfile << detail::qgate2(gate->name, "q" + to_string(
                        gate->operands[0]), "nuq" + to_string(gate->operands[1]));
                    outfile << ", " << duration << "\n";
                } else if (gate->name == "crk") {
                    Str phase = to_string(0);
                    UInt angle = 1000 / (3.14 / pow(2, gate->angle));
                    outfile << detail::qgate2(gate->name, "q" + to_string(
                        gate->operands[0]), "nuq" + to_string(gate->operands[1]));
                    outfile << ", " << to_string(angle) << "\n";
                }

                // Alternate representation of x90, mx90, y90 and my90. Now works with using qgate.
                // Can be changed to excite_MW by setting diamond_type in hw config file to
                // "rotation" instead of "qgate".
                else if (gate->name == "x90") {
                    Str phase = to_string(1.57);
                    duration = to_string((1000 / 3.14) * 1.57);
                    outfile << detail::excite_mw("0", duration, "200", phase, "60",
                                                 gate->operands[0]);
                } else if (gate->name == "mx90") {
                    Str phase = to_string(1.57);
                    duration = to_string((1000 / 3.14) * 4.71);
                    outfile << detail::excite_mw("0", duration, "200", phase, "60",
                                                 gate->operands[0]);
                } else if (gate->name == "y90") {
                    Str phase = to_string(3.14);
                    duration = to_string((1000 / 3.14) * 1.57);
                    outfile << detail::excite_mw("0", duration, "200", phase, "60",
                                                 gate->operands[0]);
                } else if (gate->name == "my90") {
                    Str phase = to_string(3.14);
                    duration = to_string((1000 / 3.14) * 4.71);
                    outfile << detail::excite_mw("0", duration, "200", phase, "60",
                                                 gate->operands[0]);
                }
            } else if (type == "prepare") {
                if (gate->name == "prep_z") {
                    // nothing, already in z-basis
                } else if (gate->name == "prep_x") {
                    // Rotate 1/2-pi around y-axis
                    outfile
                        << detail::excite_mw("0", to_string(500), "200", "3.14", "60",
                                             gate->operands[0]);
                } else if (gate->name == "prep_y") {
                    // Rotate 1/2-pi around x-axis
                    outfile
                        << detail::excite_mw("0", to_string(500), "200", "1.57", "60",
                                             gate->operands[0]);
                } else if (gate->name == "mprep_x") {
                    // Rotate -1/2-pi around y-axis
                    outfile << detail::excite_mw("0", to_string(1500), "200", "3.14", "60", gate->operands[0]);
                } else if (gate->name == "mprep_y") {
                    // Rotate -1/2-pi around x-axis
                    outfile << detail::excite_mw("0", to_string(1500), "200", "1.57", "60", gate->operands[0]);
                }
            } else if (type == "classical") {
                // These instructions calculate a new value for either current or
                // voltage using purely classical instructions. They have to be
                // elaborated using the classical instructions from the micro ISA.
                if (gate->name == "calculate_current") {
                    outfile << "calculate_current()" << "\n";
                } else if (gate->name == "calculate_voltage") {
                    outfile << "calculate_voltage()" << "\n";
                }
            } else if (type == "initial_checks") {
                if (gate->name == "mag_bias") {
                    // Code for magnetic biasing
                    // Not added because it is decomposed into sweep_bias and calculate_current()
                    // at lines 76-79 of microcode.cc
                } else if (gate->name == "rabi_check") {
                    // Implements the rabi check.
                    Str qubit_number = to_string(gate->operands[0]);

                    const auto &params =
                        gate->get_annotation<annotations::RabiParameters>();
                    const Str threshold = "0";
                    const Str threshold_measure = "33";
                    Str count = to_string(labelcount);
                    Str count_1 = to_string(labelcount + 1);
                    Str count_2 = to_string(labelcount + 2);

                    outfile
                        << detail::loadimm(to_string(params.measurements), "R",
                                           "1") << "\n";
                    outfile
                        << detail::loadimm(to_string(params.duration), "R", "2")
                        << "\n";
                    outfile
                        << detail::loadimm(to_string(params.t_max), "R", "3")
                        << "\n";

                    outfile << detail::loadimm("0", "R", "32")
                            << "\n"; // number measurements
                    outfile << detail::label(count) << "\n";
                    outfile << detail::label(count_1) << "\n";
                    //Init qubit
                    outfile << detail::label(count_2) << "\n";
                    outfile << detail::switchOn(gate->operands[0]) << "\n";
                    outfile << detail::loadimm("0", "photonReg", qubit_number)
                            << "\n";
                    outfile << detail::excite_mw("1", "100", "200", "0", "60",
                                                 gate->operands[0]) << "\n";
                    outfile << detail::mov("photonReg", qubit_number, "R",
                                           qubit_number) << "\n";
                    outfile << detail::switchOff(gate->operands[0]) << "\n";
                    outfile
                        << detail::branch("R", qubit_number, ">", "", threshold,
                                          "LAB", count_2) << "\n";

                    //Excite with Time Duration T
                    outfile << detail::excite_mw("1", "R2", "200", "0", "60",
                                                 gate->operands[0]) << "\n";

                    //Readout
                    outfile << detail::switchOn(gate->operands[0]) << "\n";
                    outfile << detail::loadimm("0", "photonReg", qubit_number)
                            << "\n";
                    outfile << detail::excite_mw("1", "100", "200", "0", "60",
                                                 gate->operands[0]) << "\n";
                    outfile << detail::mov("photonReg", qubit_number, "R",
                                           qubit_number) << "\n";
                    outfile << detail::switchOff(gate->operands[0]) << "\n";
                    outfile << detail::branch("R", qubit_number, "<", "R",
                                              threshold_measure, "ResultReg",
                                              qubit_number) << "\n";

                    // Store result and adjust memory address for next value
                    outfile << detail::store("ResultReg", qubit_number,
                                             "memAddress", qubit_number, "0")
                            << "\n";
                    outfile << detail::addimm("4", "memAddr", qubit_number)
                            << "\n";
                    outfile << detail::addimm("1", "R", "32") << "\n";

                    // if #measurements < threshold, measure again
                    outfile << detail::branch("R", "32", "<", "R", "1", "LAB",
                                              count_1) << "\n";
                    outfile
                        << detail::store("R", "2", "memAddr", qubit_number, "0")
                        << "\n";
                    outfile << detail::addimm("4", "memAddr", qubit_number)
                            << "\n";
                    outfile << detail::addimm("10", "R", "2") << "\n";
                    outfile
                        << detail::branch("R", "2", "<", "R", "3", "LAB", count)
                        << "\n";

                    // Add 3 to labelcount because label was used thrice.
                    labelcount++;
                    labelcount++;
                    labelcount++;
                } else if (gate->name == "crc") {
                    // Implements the Charge Resonance Check.
                    const auto &params =
                        gate->get_annotation<annotations::CRCParameters>();

                    Str qubit_number = to_string(gate->operands[0]);
                    Str count = to_string(labelcount);
                    Str count2 = to_string(labelcount + 1);

                    outfile << detail::loadimm(to_string(params.threshold),
                                               "treshReg", qubit_number)
                            << "\n";
                    outfile
                        << detail::loadimm(to_string(params.value), "dacReg",
                                           qubit_number) << "\n";

                    outfile << detail::label(count) << "\n";
                    outfile << detail::loadimm("0", "photon Reg", qubit_number)
                            << "\n";
                    outfile << detail::switchOn(gate->operands[0]) << "\n";
                    outfile << detail::excite_mw("1", "100", "200", "0", "60",
                                                 gate->operands[0]) << "\n";
                    outfile << detail::mov("photonReg", qubit_number, "R",
                                           qubit_number) << "\n";
                    outfile << detail::switchOff(gate->operands[0]) << "\n";
                    outfile
                        << detail::branch("R", qubit_number, ">", "treshReg",
                                          qubit_number, "LAB",
                                          count2) << "\n";
                    outfile << "calculateVoltage()" << "\n";
                    outfile << detail::jump(count) << "\n";
                    outfile << detail::label(count2) << "\n";

                    // Add 2 to labelcount because label was used twice.
                    labelcount++;
                    labelcount++;
                }
            } else if (type == "calibration") {
               if (gate->name == "decouple") {
                   // Code for XY-8 dynamical decoupling
                   Str t = to_string(50);
                   Str t2 = to_string(100);
                   outfile << detail::excite_mw("0", "500", "200", "1.57", "60", gate->operands[0])<< "\n"; // pi/2 x
                   outfile << "wait "<< t << "\n";
                   outfile << detail::excite_mw("0", "1000", "200", "1.57", "60", gate->operands[0]) << "\n"; // pi x
                   outfile << "wait "<< t2 << "\n";
                   outfile << detail::excite_mw("0", "1000", "200", "3.14", "60", gate->operands[0]) << "\n"; // pi y
                   outfile << "wait "<< t2 << "\n";
                   outfile << detail::excite_mw("0", "1000", "200", "1.57", "60", gate->operands[0]) << "\n"; // pi x
                   outfile << "wait "<< t2 << "\n";
                   outfile << detail::excite_mw("0", "1000", "200", "3.14", "60", gate->operands[0]) << "\n"; // pi y
                   outfile << "wait "<< t2 << "\n";
                   outfile << detail::excite_mw("0", "1000", "200", "3.14", "60", gate->operands[0]) << "\n"; // pi y
                   outfile << "wait "<< t2 << "\n";
                   outfile << detail::excite_mw("0", "1000", "200", "1.57", "60", gate->operands[0]) << "\n"; // pi x
                   outfile << "wait "<< t2 << "\n";
                   outfile << detail::excite_mw("0", "1000", "200", "3.14", "60", gate->operands[0]) << "\n"; // pi y
                   outfile << "wait "<< t2 << "\n";
                   outfile << detail::excite_mw("0", "1000", "200", "1.57", "60", gate->operands[0]) << "\n"; // pi x
                   outfile << "wait "<< t << "\n";
                   outfile << detail::excite_mw("0", "500", "200", "1.57", "60", gate->operands[0]) << "\n"; // pi/2 x

               } else if (gate->name == "cal_measure") {
                   // code for measurement calibration
                   Str lab_1 = to_string(labelcount+1);
                   Str lab_2 = to_string(labelcount+3);

                   Str qubit_number = to_string(gate->operands[0]);
                   const Str threshold = "0";
                   Str count = to_string(labelcount);
                   Str count_2 = to_string(labelcount+2);

                   // initialize qubit to 0
                   outfile << detail::label(count) << "\n";
                   outfile << detail::switchOn(gate->operands[0]) << "\n";
                   outfile << detail::loadimm("0", "photonReg", qubit_number)
                           << "\n";
                   outfile << detail::excite_mw("1", "100", "200", "0", "60",
                                                gate->operands[0]) << "\n";
                   outfile << detail::mov("photonReg", qubit_number, "R",
                                          qubit_number) << "\n";
                   outfile << detail::switchOff(gate->operands[0]) << "\n";
                   outfile
                       << detail::branch("R", qubit_number, ">", "", threshold,
                                         "LAB", count) << "\n";

                   outfile << detail::loadimm(to_string(0), "photonReg", to_string(gate->operands[0])) << "\n";
                   outfile << detail::loadimm(to_string(1), "R", "30") << "\n";
                   outfile << detail::label(lab_1) << "\n";
                   outfile << detail::switchOn(gate->operands[0]) << "\n";
                   outfile << detail::excite_mw("0", "1", "200", "0", "60", gate->operands[0]) << "\n";
                   outfile << detail::switchOff(gate->operands[0]) << "\n";
                   outfile << detail::store("photonReg", to_string(gate->operands[0]), "R", "1", "0") << "\n";
                   outfile << detail::store("R", "30", "R", "1", "0") << "\n";
                   outfile << detail::addimm("1", "R", "30") << "\n";
                   outfile << detail::branch("R", "30", "<", "", "40", "LAB", lab_1) << "\n";

                   // initialize qubit to 0
                   outfile << detail::label(count_2) << "\n";
                   outfile << detail::switchOn(gate->operands[0]) << "\n";
                   outfile << detail::loadimm("0", "photonReg", qubit_number)
                           << "\n";
                   outfile << detail::excite_mw("1", "100", "200", "0", "60",
                                                gate->operands[0]) << "\n";
                   outfile << detail::mov("photonReg", qubit_number, "R",
                                          qubit_number) << "\n";
                   outfile << detail::switchOff(gate->operands[0]) << "\n";
                   outfile
                       << detail::branch("R", qubit_number, ">", "", threshold,
                                         "LAB", count_2) << "\n";
                   outfile << detail::qgate("x", gate->operands[0]) << "\n";

                   outfile << detail::loadimm(to_string(0), "photonReg", to_string(gate->operands[0])) << "\n";
                   outfile << detail::loadimm(to_string(1), "R", "30") << "\n";
                   outfile << detail::label(lab_2) << "\n";
                   outfile << detail::switchOn(gate->operands[0]) << "\n";
                   outfile << detail::excite_mw("0", "1", "200", "0", "60", gate->operands[0]) << "\n";
                   outfile << detail::switchOff(gate->operands[0]) << "\n";
                   outfile << detail::store("photonReg", to_string(gate->operands[0]), "R", "2", "0") << "\n";
                   outfile << detail::store("R", "30", "R", "3", "0") << "\n";
                   outfile << detail::addimm("1", "R", "30") << "\n";
                   outfile << detail::branch("R", "30", "<", "", "40", "LAB",
                                             lab_2) << "\n";

                   outfile << "calculate_readouttime(R0, R1, R2, R3)" << "\n"; //function still needs to be implemented/designed

                   labelcount = labelcount+4;

               } else if (gate->name == "cal_pi") {
                   // code for pi-rotation calibration
                   Str qubit_number = to_string(gate->operands[0]);
                   const Str threshold = "0";
                   Str count = to_string(labelcount);
                   Str lab1 = to_string(labelcount+1);
                   Str lab2 = to_string(labelcount+2);

                   // init qubit to 0
                   outfile << detail::label(count) << "\n";
                   outfile << detail::switchOn(gate->operands[0]) << "\n";
                   outfile << detail::loadimm("0", "photonReg", qubit_number)
                           << "\n";
                   outfile << detail::excite_mw("1", "100", "200", "0", "60",
                                                gate->operands[0]) << "\n";
                   outfile << detail::mov("photonReg", qubit_number, "R",
                                          qubit_number) << "\n";
                   outfile << detail::switchOff(gate->operands[0]) << "\n";
                   outfile
                       << detail::branch("R", qubit_number, ">", "", threshold,
                                         "LAB", count) << "\n";


                   outfile << detail::loadimm("0", "R", to_string(gate->operands[0])) << "\n";
                   outfile << detail::loadimm("0", "R", to_string(gate->operands[0]+1)) << "\n";
                   outfile << detail::loadimm("0", "R", to_string(gate->operands[0]+2)) << "\n";
                   outfile << detail::label(lab1) << "\n";
                   outfile << detail::label(lab2) << "\n";
                   outfile << detail::switchOn(gate->operands[0]) << "\n";
                   outfile << detail::excite_mw("0", "1000", "200", "0", "R"+to_string(gate->operands[0]), gate->operands[0]) << "\n";
                   outfile << detail::switchOff(gate->operands[0]) << "\n";
                   outfile << detail::addimm("1", "R", to_string(gate->operands[0]+1)) << "\n";
                   outfile << detail::branch("R", to_string(gate->operands[0]+1), "<", "", "12", "LAB", lab1) << "\n";
                   outfile << "measure_fidelity(R0) \n";
                   outfile << detail::addimm("0.1", "R", to_string(gate->operands[0])) << "\n";
                   outfile << detail::addimm("1", "R", to_string(gate->operands[0]+2)) << "\n";
                   outfile << detail::branch("R", to_string(gate->operands[0]+2), ">", "", "10", "LAB", lab2) << "\n";
                   outfile << "calculate_minimum_fidelity() \n";

                   labelcount = labelcount+2;
               } else if (gate->name == "cal_halfpi") {
                   // code for pi/2-rotation calibration
                   Str qubit_number = to_string(gate->operands[0]);
                   const Str threshold = "0";
                   Str count = to_string(labelcount);
                   Str lab1 = to_string(labelcount+1);
                   Str lab2 = to_string(labelcount+2);

                   // init qubit to 0
                   outfile << detail::label(count) << "\n";
                   outfile << detail::switchOn(gate->operands[0]) << "\n";
                   outfile << detail::loadimm("0", "photonReg", qubit_number)
                           << "\n";
                   outfile << detail::excite_mw("1", "100", "200", "0", "60",
                                                gate->operands[0]) << "\n";
                   outfile << detail::mov("photonReg", qubit_number, "R",
                                          qubit_number) << "\n";
                   outfile << detail::switchOff(gate->operands[0]) << "\n";
                   outfile
                       << detail::branch("R", qubit_number, ">", "", threshold,
                                         "LAB", count) << "\n";

                   outfile << detail::loadimm("0", "R", to_string(gate->operands[0])) << "\n";
                   outfile << detail::loadimm("0", "R", to_string(gate->operands[0]+1)) << "\n";
                   outfile << detail::loadimm("0", "R", to_string(gate->operands[0]+2)) << "\n";
                   outfile << detail::label(lab1) << "\n";
                   outfile << detail::switchOn(gate->operands[0]) << "\n";
                   outfile << detail::excite_mw("0", "500", "200", "0", "R"+to_string(gate->operands[0]), gate->operands[0]) << "\n";
                   outfile << detail::switchOff(gate->operands[0]) << "\n";

                   outfile << detail::addimm("1", "R", to_string(gate->operands[0]+1)) << "\n";
                   outfile << detail::branch("R", to_string(gate->operands[0]+1), "<", "", "7", "LAB", lab1) << "\n";
                   outfile << "measure_fidelity(R0) \n";

                   // init qubit to 0
                   outfile << detail::label(count) << "\n";
                   outfile << detail::switchOn(gate->operands[0]) << "\n";
                   outfile << detail::loadimm("0", "photonReg", qubit_number)
                           << "\n";
                   outfile << detail::excite_mw("1", "100", "200", "0", "60",
                                                gate->operands[0]) << "\n";
                   outfile << detail::mov("photonReg", qubit_number, "R",
                                          qubit_number) << "\n";
                   outfile << detail::switchOff(gate->operands[0]) << "\n";
                   outfile
                       << detail::branch("R", qubit_number, ">", "", threshold,
                                         "LAB", count) << "\n";

                   outfile << detail::loadimm("0", "R", to_string(gate->operands[0])) << "\n";
                   outfile << detail::loadimm("0", "R", to_string(gate->operands[0]+1)) << "\n";
                   outfile << detail::loadimm("0", "R", to_string(gate->operands[0]+2)) << "\n";
                   outfile << detail::label(lab2) << "\n";
                   outfile << detail::switchOn(gate->operands[0]) << "\n";
                   outfile << detail::excite_mw("0", "500", "200", "0", "R"+to_string(gate->operands[0]), gate->operands[0]) << "\n";
                   outfile << detail::excite_mw("0", "1000", "200", "0", "60", gate->operands[0]) << "\n";
                   outfile << detail::switchOff(gate->operands[0]) << "\n";

                   outfile << detail::addimm("1", "R", to_string(gate->operands[0]+1)) << "\n";
                   outfile << detail::branch("R", to_string(gate->operands[0]+1), "<", "", "7", "LAB", lab2) << "\n";
                   outfile << "measure_fidelity(R0) \n";

                   labelcount = labelcount + 3;
               }
            } else {
                if (gate->name == "measure") {
                    // Measures a qubit and stores the result in ResultRegQ,
                    // where Q is the qubit number. Also stores the result in
                    // breg[Q], as per OpenQL standard.
                    Str qubit_number = to_string(gate->operands[0]);
                    const Str threshold = "33";

                    outfile << detail::switchOn(gate->operands[0]) << "\n";
                    outfile << detail::loadimm("0", "photonReg", qubit_number)
                            << "\n";
                    outfile << detail::excite_mw("1", "100", "200", "0", "60",
                                                 gate->operands[0]) << "\n";
                    outfile
                        << detail::mov("photonReg", qubit_number, "R",
                                       qubit_number)
                        << "\n";
                    outfile << detail::switchOff(gate->operands[0]) << "\n";
                    outfile << detail::branch("R", qubit_number, "<", "R",
                                              threshold, "ResultReg",
                                              qubit_number) << "\n";
                } else if (gate->name == "initialize") {
                    // Initializes a qubit to |0>.
                    Str qubit_number = to_string(gate->operands[0]);
                    const Str threshold = "0";
                    Str count = to_string(labelcount);

                    outfile << detail::label(count) << "\n";
                    outfile << detail::switchOn(gate->operands[0]) << "\n";
                    outfile << detail::loadimm("0", "photonReg", qubit_number)
                            << "\n";
                    outfile << detail::excite_mw("1", "100", "200", "0", "60",
                                                 gate->operands[0]) << "\n";
                    outfile << detail::mov("photonReg", qubit_number, "R",
                                           qubit_number) << "\n";
                    outfile << detail::switchOff(gate->operands[0]) << "\n";
                    outfile
                        << detail::branch("R", qubit_number, ">", "", threshold,
                                          "LAB", count) << "\n";

                    labelcount++;
                } else if (gate->name == "wait") {
                    // Implements the wait x-cycles instruction.
                    outfile << "wait "
                            << to_string(gate->duration) << "\n";
                } else if (gate->name == "barrier") {
                    // Ignore barriers
                } else if (gate->name == "qnop") {
                    // Quantum nop instruction
                    outfile << "wait 1" << "\n";
                } else if (gate->name == "sweep_bias") {
                    // Implements the instruction sweep_bias, that sweeps the frequency
                    // of the laser of a qubit to help determine the magnetic biasing value
                    // for correct biasing.
                    const auto &params =
                        gate->get_annotation<annotations::SweepBiasParameters>();

                    Str qubit_number = to_string(gate->operands[0]);
                    Str count = to_string(labelcount);

                    outfile
                        << detail::loadimm(to_string(params.value), "dacReg",
                                           to_string(params.dacreg)) << "\n";
                    outfile
                        << detail::loadimm(to_string(params.start),
                                           "sweepStartReg",
                                           qubit_number) << "\n";
                    outfile
                        << detail::loadimm(to_string(params.step),
                                           "sweepStepReg",
                                           qubit_number) << "\n";
                    outfile
                        << detail::loadimm(to_string(params.max),
                                           "sweepStopReg",
                                           qubit_number) << "\n";
                    outfile
                        << detail::loadimm(to_string(params.memaddress),
                                           "memAddr",
                                           qubit_number) << "\n";
                    outfile << detail::label(count) << "\n";
                    outfile << detail::switchOn(gate->operands[0]) << "\n";
                    outfile << detail::excite_mw("1", "100",
                                                 "sweepStartReg" + qubit_number,
                                                 "0", "60", gate->operands[0])
                            << "\n";
                    outfile << detail::switchOff(gate->operands[0]) << "\n";
                    outfile
                        << detail::mov("photonReg", qubit_number, "R",
                                       qubit_number)
                        << "\n";
                    outfile
                        << detail::store("R", qubit_number, "memAddr",
                                         qubit_number,
                                         "0") << "\n";
                    outfile
                        << detail::store("sweepStartReg", qubit_number,
                                         "memAddr",
                                         qubit_number, "0") << "\n";
                    outfile << detail::add("sweepStartReg", qubit_number,
                                           "sweepStartReg", qubit_number,
                                           "sweepStepReg", qubit_number)
                            << "\n";
                    outfile << detail::addimm("4", "memAddr", qubit_number)
                            << "\n";
                    outfile
                        << detail::branch("sweepStartReg", qubit_number, "<",
                                          "sweepStopReg", qubit_number, "LAB",
                                          count) << "\n";
                    labelcount++;
                } else if (gate->name == "excite_mw") {
                    // Implements the custom instruction on how the user wants
                    // to use the laser.
                    const auto &params =
                        gate->get_annotation<annotations::ExciteMicrowaveParameters>();

                    outfile << detail::excite_mw(to_string(params.envelope),
                                                 to_string(params.duration),
                                                 to_string(params.frequency),
                                                 to_string(params.phase),
                                                 to_string(params.amplitude),
                                                 gate->operands[0]) << "\n";
                } else if (gate->name == "memswap") {
                    // Implements the swap from electron qubit to nuclear spin qubit.
                    const auto &params =
                        gate->get_annotation<annotations::MemSwapParameters>();

                    Str nuq = "nuq" + to_string(params.nuclear);
                    Str qubit = "q" + to_string(gate->operands[0]);
                    outfile << detail::qgate2("pmy90", qubit, nuq) << "\n";
                    outfile << detail::qgate("x90", gate->operands[0]) << "\n";
                    outfile << detail::qgate2("pmx90", qubit, nuq) << "\n";
                    outfile << detail::qgate("my90", gate->operands[0]) << "\n";
                } else if (gate->name == "qentangle") {
                    // Implements electron-nuclear spin entanglement.
                    const auto &params =
                        gate->get_annotation<annotations::QEntangleParameters>();

                    Str nuq = "nuq" + to_string(params.nuclear);
                    Str qubit = "q" + to_string(gate->operands[0]);
                    outfile << detail::qgate("mx90", gate->operands[0]) << "\n";
                    outfile << detail::qgate2("pmx90", qubit, nuq) << "\n";
                    outfile << detail::qgate("x90", gate->operands[0]) << "\n";
                } else if (gate->name == "nventangle") {
                    // Implements electron-electron entanglement following the
                    // Barrett and Kok scheme.
                    Str count = to_string(labelcount);
                    Str count_1 = to_string(labelcount + 1);

                    outfile << detail::loadimm("0", "R", "2") << "\n";
                    outfile << detail::label(count) << "\n";
                    outfile << detail::switchOn(gate->operands[0]) << "\n";
                    outfile << detail::switchOn(gate->operands[1]) << "\n";
                    outfile << detail::excite_mw("1", "100", "200", "0", "60",
                                                 gate->operands[0]) << "\n";
                    outfile << detail::excite_mw("1", "100", "200", "0", "60",
                                                 gate->operands[1]) << "\n";
                    outfile << "wait 100" << "\n";
                    outfile << detail::mov("R", "0", "photonReg", "01") << "\n";
                    outfile << detail::switchOff(gate->operands[0]) << "\n";
                    outfile << detail::switchOff(gate->operands[1]) << "\n";
                    outfile << detail::addimm("1", "R", "2") << "\n";
                    outfile << "wait 50" << "\n";
                    outfile
                        << detail::branch("R", "1", ">", "", "1", "LAB",
                                          count_1)
                        << "\n";
                    outfile << detail::qgate("x", gate->operands[0]) << "\n";
                    outfile << detail::qgate("x", gate->operands[1]) << "\n";
                    outfile << detail::mov("R", "0", "R", "1") << "\n";
                    outfile << detail::jump(count) << "\n";
                    outfile << detail::label(count_1) << "\n";

                    labelcount++;
                    labelcount++;
                } else {
                    outfile << "ERROR: Gate " + gate->name +
                               " is not supported by the Diamond Architecture."
                            << "\n";
                }
            }

            if (!end_label.empty()) {
                outfile << detail::label(end_label) << "\n";
            }

            outfile << "\n";
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
