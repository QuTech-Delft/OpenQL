/** \file
 * Defines the pass for generation the microcode for the Fujitsu project quantum
 * computer
 */

#include "ql/arch/diamond/pass/gen/microcode/microcode.h"
#include "ql/arch/diamond/pass/gen/microcode/detail/functions.h"

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
    Str file_name(program->unique_name + ".txt");

    // Print action
    //QL_DOUT("Compiling " << program->kernels.size() << " kernels to generate Diamond microcode program" << file_name;

    // Pseudo-code example ////////////////////////////////////////
    //    switch (operation) {
    //        case x-gate
    //            print x-gate
    //        case y-gate
    //            print y-gate
    //        case measurement
    //            print switchOn q0
    //            print LDi 0, photonReg0
    //            print excite_MW 1, 100, 200, 0, q0
    //            mov photonReg0, R0
    //            print switchOff q0
    //            BR R0>R33, ResultReg0
    //    }
    // ///////////////////////////////////////////////////////////
    // Note, each print statement (for example, print switchOn) can be a function
    // with the parameters. So, there would be a .h and a .cc file where the
    // function switchOn(arg1, arg2, arg3) is declared. This way, if the function
    // changes it only needs to be changed once instead for every usage.


    // Add pass code here
    OutFile outfile{file_name};

    for (const ir::KernelRef &kernel : program->kernels) {
        for (const ir::GateRef &gate : kernel->gates) {
            if (gate->name == "i") {
                outfile << "qgate I " << gate->operands.to_string("", ", ", "");
            } else if (gate->name == "h") {
                outfile << "qgate H " << gate->operands.to_string("", ", ", "");
            } else if (gate->name == "s") {
                outfile << "qgate S " << gate->operands.to_string("", ", ", "");
            } else if (gate->name == "t") {
                outfile << "qgate T " << gate->operands.to_string("", ", ", "");
            } else if (gate->name == "x") {
                outfile << "qgate X " << gate->operands.to_string("", ", ", "");
            } else if (gate->name == "y") {
                outfile << "qgate Y " << gate->operands.to_string("", ", ", "");
            } else if (gate->name == "z") {
                outfile << "qgate Z " << gate->operands.to_string("", ", ", "");
            } else if (gate->name == "measure") {
                Str qubit_number = to_string(gate->operands[0]).erase(0,1);

                outfile << detail::switchOn(gate->operands[0]);
                outfile << detail::loadimm("0", "photonReg", qubit_number);
                //outfile << excite_mw(1, 100, 200, 0, gate->operands[0]);
                //outfile << detail::mov();
                outfile << detail::switchOff(gate->operands[0]);
                //Str comp_flag = "R" + op1.to_string() + "<R33";
                //outfile << branch(comp_flag, gate->operands[1]);

            } else if (gate->name == "prep_z") {
                outfile << "initialize " << gate->operands.to_string("", ", ", "");
            } else if (gate->name == "cnot") {
                outfile << "qgate2 CNOT " << gate->name << gate->operands.to_string("", ", ", "");
            } else if (gate->name == "cz") {
                outfile << "qgate 2 CZ " << gate->operands.to_string("", ", ", "");
            } else if (gate->name == "wait") {
                outfile << "Z " << gate->operands.to_string("", ", ", "");
            } else if (gate->name == "swap") {
                outfile << "Z " << gate->operands.to_string("", ", ", "");
            } else {
                outfile << "The name of the gate was not recognized";
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
