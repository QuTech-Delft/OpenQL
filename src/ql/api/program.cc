/** \file
 * API header for using quantum programs.
 */

#include "ql/api/program.h"

#include "ql/com/interaction_matrix.h"
#include "ql/pass/io/sweep_points/annotation.h"
#include "ql/ir/old_to_new.h"
#include "ql/api/kernel.h"
#include "ql/api/operation.h"
#include "ql/api/misc.h"

//============================================================================//
//                               W A R N I N G                                //
//----------------------------------------------------------------------------//
//         Docstrings in this file must manually be kept in sync with         //
//      program.i! This should be automated at some point, but isn't yet.     //
//============================================================================//

namespace ql {
namespace api {

/**
 * Creates a new program with the given name, using the given platform.
 * The third, fourth, and fifth arguments optionally specify the desired
 * number of qubits, classical integer registers, and classical bit
 * registers. If not specified, the number of qubits is taken from the
 * platform, and no classical or bit registers will be allocated.
 */
Program::Program(
    const std::string &name,
    const Platform &platform,
    size_t qubit_count,
    size_t creg_count,
    size_t breg_count
) :
    name(name),
    platform(platform),
    qubit_count(qubit_count),
    creg_count(creg_count),
    breg_count(breg_count)
{
    if (!qubit_count) qubit_count = platform.platform->qubit_count;
    program.emplace(name, platform.platform, qubit_count, creg_count, breg_count);
    pass_manager = platform.pass_manager;
}

/**
 * Adds an unconditionally-executed kernel to the end of the program.
 */
void Program::add_kernel(const Kernel &k) {
    program->add(k.kernel);
}

/**
 * Adds an unconditionally-executed subprogram to the end of the program.
 */
void Program::add_program(const Program &p) {
    program->add_program(p.program);
}

/**
 * Adds a conditionally-executed kernel to the end of the program. The
 * kernel will be executed if the given classical condition is true.
 */
void Program::add_if(const Kernel &k, const Operation &operation) {
    program->add_if(k.kernel, *operation.operation);
}

/**
 * Adds a conditionally-executed subprogram to the end of the program. The
 * kernel will be executed if the given classical condition evaluates to
 * true.
 */
void Program::add_if(const Program &p, const Operation &operation) {
    program->add_if(p.program, *operation.operation);
}

/**
 * Adds two conditionally-executed kernels with inverted conditions to the
 * end of the program. The first kernel will be executed if the given
 * classical condition evaluates to true; the second kernel will be executed
 * if it evaluates to false.
 */
void Program::add_if_else(const Kernel &k_if, const Kernel &k_else, const Operation &operation) {
    program->add_if_else(k_if.kernel, k_else.kernel, *(operation.operation));
}

/**
 * Adds two conditionally-executed subprograms with inverted conditions to
 * the end of the program. The first kernel will be executed if the given
 * classical condition evaluates to true; the second kernel will be executed
 * if it evaluates to false.
 */
void Program::add_if_else(const Program &p_if, const Program &p_else, const Operation &operation) {
    program->add_if_else(p_if.program, p_else.program, *(operation.operation));
}

/**
 * Adds a kernel that will be repeated until the given classical condition
 * evaluates to true. The kernel is executed at least once, since the
 * condition is evaluated at the end of the loop body.
 */
void Program::add_do_while(const Kernel &k, const Operation &operation) {
    program->add_do_while(k.kernel, *operation.operation);
}

/**
 * Adds a subprogram that will be repeated until the given classical
 * condition evaluates to true. The subprogram is executed at least once,
 * since the condition is evaluated at the end of the loop body.
 */
void Program::add_do_while(const Program &p, const Operation &operation) {
    program->add_do_while(p.program, *operation.operation);
}

/**
 * Adds an unconditionally-executed kernel that will loop for the given
 * number of iterations.
 */
void Program::add_for(const Kernel &k, size_t iterations) {
    program->add_for(k.kernel, iterations);
}

/**
 * Adds an unconditionally-executed subprogram that will loop for the given
 * number of iterations.
 */
void Program::add_for(const Program &p, size_t iterations) {
    program->add_for(p.program, iterations);
}

/**
 * Sets sweep point information for the program.
 */
void Program::set_sweep_points(const std::vector<double> &sweep_points) {
    QL_WOUT("The sweep points system is deprecated and may be removed at any time");
    using Annotation = ql::pass::io::sweep_points::Annotation;
    if (!program->has_annotation<Annotation>()) {
        program->set_annotation<Annotation>({});
    }
    program->get_annotation<Annotation>().data = sweep_points;
}

/**
 * Returns the configured sweep point information for the program.
 */
std::vector<double> Program::get_sweep_points() const {
    QL_WOUT("The sweep points system is deprecated and may be removed at any time");
    using Annotation = ql::pass::io::sweep_points::Annotation;
    auto annot = program->get_annotation_ptr<Annotation>();
    if (annot == nullptr) {
        return {};
    } else {
        return {annot->data.begin(), annot->data.end()};
    }
}

/**
 * Sets the name of the file that the sweep points will be written to.
 */
void Program::set_config_file(const std::string &config_file_name) {
    QL_WOUT("The sweep points system is deprecated and may be removed at any time");
    using Annotation = ql::pass::io::sweep_points::Annotation;
    if (!program->has_annotation<Annotation>()) {
        program->set_annotation<Annotation>({});
    }
    program->get_annotation<Annotation>().config_file_name
        = get_option("output_dir") + "/" + config_file_name;
}

/**
 * Whether a custom compiler configuration has been attached to this
 * program. When this is the case, it will be used to implement compile(),
 * rather than generating the compiler in-place from defaults and global
 * options during the call.
 */
bool Program::has_compiler() {
    return pass_manager.has_value();
}

/**
 * Returns the custom compiler configuration associated with this program.
 * If no such configuration exists yet, the default one is created,
 * attached, and returned.
 */
Compiler Program::get_compiler() {
    if (!pass_manager.has_value()) {
        pass_manager.emplace(ql::pmgr::Manager::from_defaults(program->platform));
    }
    return Compiler(pass_manager);
}

/**
 * Sets the compiler associated with this program. It will then be used for
 * compile().
 */
void Program::set_compiler(const Compiler &compiler) {
    pass_manager = compiler.pass_manager;
}

/**
 * Compiles the program.
 */
void Program::compile() {
    QL_IOUT("compiling " << name << " ...");
    auto ir = ir::convert_old_to_new(program);
    if (pass_manager.has_value()) {
        pass_manager->compile(ir);
    } else {
        ql::pmgr::Manager::from_defaults(program->platform).compile(ir);
    }
}

/**
 * Prints the interaction matrix for each kernel in the program.
 */
void Program::print_interaction_matrix() const {
    QL_IOUT("printing interaction matrix...");

    ql::com::InteractionMatrix::dump_for_program(program);
}

/**
 * Writes the interaction matrix for each kernel in the program to a file.
 * This is one of the few functions that still uses the global output_dir
 * option.
 */
void Program::write_interaction_matrix() const {
    ql::com::InteractionMatrix::write_for_program(
        get_option("output_dir") + "/",
        program
    );
}

} // namespace api
} // namespace ql
