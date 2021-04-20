/** \file
 * Implementation for Python interface classes.
 */

#include "ql/api/api.h"

#include "ql/version.h"
#include "ql/com/interaction_matrix.h"
#include "ql/pass/io/sweep_points/annotation.h"
#include "ql/pmgr/manager.h"

//============================================================================//
//                               W A R N I N G                                //
//----------------------------------------------------------------------------//
//         Docstrings in this file must manually be kept in sync with         //
//   python/openql.i! This should be automated at some point, but isn't yet.  //
//============================================================================//

namespace ql {
namespace api {

/**
 * Records whether initialized has been called yet.
 */
static bool initialized = false;

/**
 * Initializes the OpenQL library, for as far as this must be done. This should
 * be called by the user (in Python) before anything else.
 *
 * Currently this just resets the options to their default values to give the
 * user a clean slate to work with in terms of global variables (in case someone
 * else has used the library in the same interpreter before them, for instance,
 * as might happen with ipython/Jupyter in a shared notebook server, or during
 * test suites), but it may initialize more things in the future.
 */
void initialize() {
    if (initialized) {
        QL_IOUT("re-initializing OpenQL library");
    } else {
        QL_IOUT("initializing OpenQL library");
    }
    initialized = true;
    ql::com::options::global.reset();
}

/**
 * Make sure initialize() has been called.
 */
static void ensure_initialized() {
    if (!initialized) {
        QL_WOUT("Calling initialize() implicitly! In the future, please call initialize() before anything else.");
        initialize();
    }
}

/**
 * Returns the compiler's version string.
 */
std::string get_version() {
    return OPENQL_VERSION_STRING;
}

/**
 * Sets a global option for the compiler. Use print_options() to get a list of
 * all available options.
 */
void set_option(const std::string &option_name, const std::string &option_value) {
    ensure_initialized();
    ql::com::options::global[option_name] = option_value;
}

/**
 * Returns the current value for a global option. Use print_options() to get a
 * list of all available options.
 */
std::string get_option(const std::string &option_name) {
    return ql::com::options::global[option_name].as_str();
}

/**
 * Prints a list of all available options.
 */
void print_options() {
    ql::com::options::global.help();
}

/**
 * Constructor used internally to build a pass object that belongs to
 * a compiler.
 */
Pass::Pass(const ql::pmgr::PassRef &pass) : pass(pass) {
}

/**
 * Constructor used internally to build a compiler object that belongs to
 * a platform.
 */
Compiler::Compiler(
    const ql::pmgr::Ref &pass_manager
) :
    pass_manager(pass_manager)
{ }

/**
 * Creates an empty compiler, with no specified architecture.
 */
Compiler::Compiler(
) :
    pass_manager(ql::utils::Ptr<ql::pmgr::Manager>::make())
{ }

/**
 * Creates a compiler configuration from the given JSON file.
 */
Compiler::Compiler(
    const std::string &fname
) :
    pass_manager(ql::pmgr::Manager::from_json(ql::utils::load_json(fname)))
{ }

/**
 * Creates a default compiler for the given platform.
 */
Compiler::Compiler(
    const Platform &platform
) :
    pass_manager(ql::pmgr::Manager::from_defaults(platform.platform))
{ }

/**
 * Prints documentation for all available pass types, as well as the option
 * documentation for the passes.
 */
void Compiler::print_pass_types() const {
    pass_manager->dump_pass_types();
}

/**
 * Returns documentation for all available pass types, as well as the option
 * documentation for the passes.
 */
std::string Compiler::get_pass_types() const {
    std::ostringstream ss;
    pass_manager->dump_pass_types(ss);
    return ss.str();
}

/**
 * Prints the currently configured compilation strategy.
 */
void Compiler::print_strategy() const {
    pass_manager->dump_strategy();
}

/**
 * Returns the currently configured compilation strategy as a string.
 */
std::string Compiler::get_strategy() const {
    std::ostringstream ss;
    pass_manager->dump_strategy(ss);
    return ss.str();
}

/**
 * Sets a pass option. Periods are used as hierarchy separators; the last
 * element will be the option name, and the preceding elements represent
 * pass instance names. Furthermore, wildcards may be used for the pass name
 * elements (asterisks for zero or more characters and a question mark for a
 * single character) to select multiple or all immediate sub-passes of that
 * group, and a double asterisk may be used for the element before the
 * option name to chain to set_option_recursively() instead. The return
 * value is the number of passes that were affected; passes are only
 * affected when they are selected by the option path AND have an option
 * with the specified name. If must_exist is set an exception will be thrown
 * if none of the passes were affected, otherwise 0 will be returned.
 */
size_t Compiler::set_option(
    const std::string &path,
    const std::string &value,
    bool must_exist
) {
    return pass_manager->set_option(path, value, must_exist);
}

/**
 * Sets an option for all passes recursively. The return value is the number
 * of passes that were affected; passes are only affected when they have an
 * option with the specified name. If must_exist is set an exception will be
 * thrown if none of the passes were affected, otherwise 0 will be returned.
 */
size_t Compiler::set_option_recursively(
    const std::string &option,
    const std::string &value,
    bool must_exist
) {
    return pass_manager->set_option_recursively(option, value, must_exist);
}

/**
 * Returns the current value of an option. Periods are used as hierarchy
 * separators; the last element will be the option name, and the preceding
 * elements represent pass instance names.
 */
std::string Compiler::get_option(const std::string &path) const {
    return pass_manager->get_option(path).as_str();
}

/**
 * Appends a pass to the end of the pass list. If type_name is empty
 * or unspecified, a generic subgroup is added. Returns a reference to the
 * constructed pass.
 */
Pass Compiler::append_pass(
    const std::string &type_name,
    const std::string &instance_name,
    const std::map<std::string, std::string> &options
) {
    return Pass(pass_manager->append_pass(type_name, instance_name, options));
}

/**
 * Appends a pass to the beginning of the pass list. If type_name is empty
 * or unspecified, a generic subgroup is added. Returns a reference to the
 * constructed pass.
 */
Pass Compiler::prefix_pass(
    const std::string &type_name,
    const std::string &instance_name,
    const std::map<std::string, std::string> &options
) {
    return Pass(pass_manager->prefix_pass(type_name, instance_name, options));
}

/**
 * Inserts a pass immediately after the target pass (named by instance). If
 * target does not exist, an exception is thrown. If type_name is empty or
 * unspecified, a generic subgroup is added. Returns a reference to the
 * constructed pass. Periods may be used in target to traverse deeper into
 * the pass hierarchy.
 */
Pass Compiler::insert_pass_after(
    const std::string &target,
    const std::string &type_name,
    const std::string &instance_name,
    const std::map<std::string, std::string> &options
) {
    return Pass(pass_manager->insert_pass_after(target, type_name, instance_name, options));
}

/**
 * Inserts a pass immediately before the target pass (named by instance). If
 * target does not exist, an exception is thrown. If type_name is empty or
 * unspecified, a generic subgroup is added. Returns a reference to the
 * constructed pass. Periods may be used in target to traverse deeper into
 * the pass hierarchy.
 */
Pass Compiler::insert_pass_before(
    const std::string &target,
    const std::string &type_name,
    const std::string &instance_name,
    const std::map<std::string, std::string> &options
) {
    return Pass(pass_manager->insert_pass_before(target, type_name, instance_name, options));
}

/**
 * Looks for the pass with the target instance name, and embeds it into a
 * newly generated group. The group will assume the name of the original
 * pass, while the original pass will be renamed as specified by sub_name.
 * Note that this ultimately does not modify the pass order. If target does
 * not exist or this pass is not a group of sub-passes, an exception is
 * thrown. Returns a reference to the constructed group. Periods may be used
 * in target to traverse deeper into the pass hierarchy.
 */
Pass Compiler::group_pass(
    const std::string &target,
    const std::string &sub_name
) {
    return Pass(pass_manager->group_pass(target, sub_name));
}

/**
 * Like group_pass(), but groups an inclusive range of passes into a
 * group with the given name, leaving the original pass names unchanged.
 * Periods may be used in from/to to traverse deeper into the pass
 * hierarchy, but the hierarchy prefix must be the same for from and to.
 */
Pass Compiler::group_passes(
    const std::string &from,
    const std::string &to,
    const std::string &group_name
) {
    return Pass(pass_manager->group_passes(from, to, group_name));
}

/**
 * Looks for an unconditional pass group with the target instance name and
 * flattens its contained passes into its parent group. The names of the
 * passes found in the collapsed group are prefixed with name_prefix before
 * they are added to the parent group. Note that this ultimately does not
 * modify the pass order. If the target instance name does not exist or is
 * not an unconditional group, an exception is thrown. Periods may be used
 * in target to traverse deeper into the pass hierarchy.
 */
void Compiler::flatten_subgroup(
    const std::string &target,
    const std::string &name_prefix
) {
    pass_manager->flatten_subgroup(target, name_prefix);
}

/**
 * Returns a reference to the pass with the given instance name. If no such
 * pass exists, an exception is thrown. Periods may be used as hierarchy
 * separators to get nested sub-passes.
 */
Pass Compiler::get_pass(const std::string &target) const {
    return Pass(pass_manager->get_pass(target));
}

/**
 * Returns whether a pass with the target instance name exists. Periods may
 * be used in target to traverse deeper into the pass hierarchy.
 */
bool Compiler::does_pass_exist(const std::string &target) const {
    return pass_manager->does_pass_exist(target);
}

/**
 * Returns the total number of passes in the root hierarchy.
 */
size_t Compiler::get_num_passes() const {
    return pass_manager->get_num_passes();
}

/**
 * If this pass constructed into a group of passes, returns a reference to
 * the list containing all the sub-passes. Otherwise, an exception is
 * thrown.
 */
std::vector<Pass> Compiler::get_passes() const {
    std::vector<Pass> retval;
    for (const auto &pass : pass_manager->get_passes()) {
        retval.push_back(Pass(pass));
    }
    return retval;
}

/**
 * Returns an indexable list of references to all passes with the given
 * type within the root hierarchy.
 */
std::vector<Pass> Compiler::get_sub_passes_by_type(const std::string &target) const {
    std::vector<Pass> retval;
    for (const auto &pass : pass_manager->get_sub_passes_by_type(target)) {
        retval.push_back(Pass(pass));
    }
    return retval;
}

/**
 * Removes the pass with the given target instance name, or throws an
 * exception if no such pass exists.
 */
void Compiler::remove_pass(const std::string &target) {
    pass_manager->remove_pass(target);
}

/**
 * Clears the entire pass list.
 */
void Compiler::clear_passes() {
    pass_manager->clear_passes();
}

/**
 * Constructs all passes recursively. This freezes the pass options, but
 * allows subtrees to be modified.
 */
void Compiler::construct() {
    pass_manager->construct();
}

/**
 * Ensures that all passes have been constructed, and then runs the passes
 * on the given program. This is the same as Program.compile() when the
 * program's platform is referencing the same compiler
 */
void Compiler::compile(const Program &program) {
    pass_manager->compile(program.program);
}

/**
 * Constructs a platform. name is any name the user wants to give to the
 * platform; it is only used for report messages. platform_config_file must
 * point to a JSON file that represents the platform. Optionally,
 * compiler_config_file can be specified to override the compiler
 * configuration specified by the platform (if any).
 */
Platform::Platform(
    const std::string &name,
    const std::string &platform_config_file,
    const std::string &compiler_config_file
) :
    name(name),
    config_file(platform_config_file)
{
    ensure_initialized();
    platform.emplace(name, platform_config_file, compiler_config_file);
}

/**
 * Returns the number of qubits in the platform.
 */
size_t Platform::get_qubit_number() const {
    return platform->qubit_count;
}

/**
 * Prints some basic information about the platform.
 */
void Platform::print_info() const {
    platform->dump_info();
}

/**
 * Returns the result of print_info() as a string.
 */
std::string Platform::get_info() const {
    std::ostringstream ss;
    platform->dump_info(ss);
    return ss.str();
}

/**
 * Whether a custom compiler configuration has been attached to this
 * platform. When this is the case, programs constructed from this platform
 * will use it to implement Program.compile(), rather than generating the
 * compiler in-place from defaults and global options during the call.
 */
bool Platform::has_compiler() {
    return pass_manager.has_value();
}

/**
 * Returns the custom compiler configuration associated with this platform.
 * If no such configuration exists yet, the default one is created,
 * attached, and returned.
 */
Compiler Platform::get_compiler() {
    if (!pass_manager.has_value()) {
        pass_manager.emplace(ql::pmgr::Manager::from_defaults(platform));
    }
    return Compiler(pass_manager);
}

/**
 * Sets the compiler associated with this platform. Any programs constructed
 * from this platform after this call will use the given compiler.
 */
void Platform::set_compiler(const Compiler &compiler) {
    pass_manager = compiler.pass_manager;
}


CReg::CReg(size_t id) {
    creg.emplace(id);
}

Operation::Operation(const CReg &lop, const std::string &op, const CReg &rop) {
    operation.emplace(*(lop.creg), op, *(rop.creg));
}

Operation::Operation(const std::string &op, const CReg &rop) {
    operation.emplace(op, *(rop.creg));
}

Operation::Operation(const CReg &lop) {
    operation.emplace(*(lop.creg));
}

Operation::Operation(int val) {
    operation.emplace(val);
}

Unitary::Unitary(
    const std::string &name,
    const std::vector<std::complex<double>> &matrix
) :
    name(name)
{
    unitary.emplace(name, ql::utils::Vec<ql::utils::Complex>(matrix.begin(), matrix.end()));
}

void Unitary::decompose() {
    unitary->decompose();
}

bool Unitary::is_decompose_support_enabled() {
    return ql::com::Unitary::is_decompose_support_enabled();
}

Kernel::Kernel(
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
    QL_WOUT("Kernel(name,Platform,#qbit,#creg,#breg) API will soon be deprecated according to issue #266 - OpenQL v0.9");
    kernel.emplace(name, platform.platform, qubit_count, creg_count, breg_count);
}

void Kernel::identity(size_t q0) {
    kernel->identity(q0);
}

void Kernel::hadamard(size_t q0) {
    kernel->hadamard(q0);
}

void Kernel::s(size_t q0) {
    kernel->s(q0);
}

void Kernel::sdag(size_t q0) {
    kernel->sdag(q0);
}

void Kernel::t(size_t q0) {
    kernel->t(q0);
}

void Kernel::tdag(size_t q0) {
    kernel->tdag(q0);
}

void Kernel::x(size_t q0) {
    kernel->x(q0);
}

void Kernel::y(size_t q0) {
    kernel->y(q0);
}

void Kernel::z(size_t q0) {
    kernel->z(q0);
}

void Kernel::rx90(size_t q0) {
    kernel->rx90(q0);
}

void Kernel::mrx90(size_t q0) {
    kernel->mrx90(q0);
}

void Kernel::rx180(size_t q0) {
    kernel->rx180(q0);
}

void Kernel::ry90(size_t q0) {
    kernel->ry90(q0);
}

void Kernel::mry90(size_t q0) {
    kernel->mry90(q0);
}

void Kernel::ry180(size_t q0) {
    kernel->ry180(q0);
}

void Kernel::rx(size_t q0, double angle) {
    kernel->rx(q0, angle);
}

void Kernel::ry(size_t q0, double angle) {
    kernel->ry(q0, angle);
}

void Kernel::rz(size_t q0, double angle) {
    kernel->rz(q0, angle);
}

void Kernel::measure(size_t q0) {
    QL_DOUT("Python k.measure([" << q0 << "])");
    kernel->measure(q0);
}

void Kernel::measure(size_t q0, size_t b0) {
    QL_DOUT("Python k.measure([" << q0 << "], [" << b0 << "])");
    kernel->measure(q0, b0);
}

void Kernel::prepz(size_t q0) {
    kernel->prepz(q0);
}

void Kernel::cnot(size_t q0, size_t q1) {
    kernel->cnot(q0,q1);
}

void Kernel::cphase(size_t q0, size_t q1) {
    kernel->cphase(q0,q1);
}

void Kernel::cz(size_t q0, size_t q1) {
    kernel->cz(q0,q1);
}

void Kernel::toffoli(size_t q0, size_t q1, size_t q2) {
    kernel->toffoli(q0,q1,q2);
}

void Kernel::clifford(int id, size_t q0) {
    kernel->clifford(id, q0);
}

void Kernel::wait(const std::vector<size_t> &qubits, size_t duration) {
    kernel->wait({qubits.begin(), qubits.end()}, duration);
}

void Kernel::barrier(const std::vector<size_t> &qubits) {
    kernel->wait({qubits.begin(), qubits.end()}, 0);
}

std::string Kernel::get_custom_instructions() const {
    return kernel->get_gates_definition();
}

void Kernel::display() {
    kernel->display();
}

void Kernel::gate(const std::string &gname, size_t q0) {
    kernel->gate(gname, q0);
}

void Kernel::gate(const std::string &gname, size_t q0, size_t q1) {
    kernel->gate(gname, q0, q1);
}

void Kernel::gate(
    const std::string &name,
    const std::vector<size_t> &qubits,
    size_t duration,
    double angle,
    const std::vector<size_t> &bregs,
    const std::string &condstring,
    const std::vector<size_t> &condregs
) {
    QL_DOUT(
        "Python k.gate("
        << name
        << ", "
        << ql::utils::Vec<size_t>(qubits.begin(), qubits.end())
        << ", "
        << duration
        << ", "
        << angle
        << ", "
        << ql::utils::Vec<size_t>(bregs.begin(), bregs.end())
        << ", "
        << condstring
        << ", "
        << ql::utils::Vec<size_t>(condregs.begin(), condregs.end())
        << ")"
    );
    auto condvalue = kernel->condstr2condvalue(condstring);

    kernel->gate(
        name,
        {qubits.begin(), qubits.end()},
        {},
        duration,
        angle,
        {bregs.begin(), bregs.end()},
        condvalue,
        {condregs.begin(), condregs.end()}
    );
}

void Kernel::gate(
    const std::string &name,
    const std::vector<size_t> &qubits,
    const CReg &destination
) {
    QL_DOUT(
        "Python k.gate("
        << name
        << ", "
        << ql::utils::Vec<size_t>(qubits.begin(), qubits.end())
        << ",  "
        << (destination.creg)->id
        << ") # (name,qubits,creg-destination)"
    );
    kernel->gate(name, {qubits.begin(), qubits.end()}, {(destination.creg)->id} );
}

void Kernel::gate_preset_condition(
    const std::string &condstring,
    const std::vector<size_t> &condregs
) {
    QL_DOUT("Python k.gate_preset_condition("<<condstring<<", condregs)");
    kernel->gate_preset_condition(
        kernel->condstr2condvalue(condstring),
        {condregs.begin(), condregs.end()}
    );
}

void Kernel::gate_clear_condition() {
    QL_DOUT("Python k.gate_clear_condition()");
    kernel->gate_clear_condition();
}

void Kernel::condgate(
    const std::string &name,
    const std::vector<size_t> &qubits,
    const std::string &condstring,
    const std::vector<size_t> &condregs
) {
    QL_DOUT(
        "Python k.condgate("
        << name
        << ", "
        << ql::utils::Vec<size_t>(qubits.begin(), qubits.end())
        << ", "
        << condstring
        << ", "
        << ql::utils::Vec<size_t>(condregs.begin(), condregs.end())
        << ")"
    );
    kernel->condgate(
        name,
        {qubits.begin(), qubits.end()},
        kernel->condstr2condvalue(condstring),
        {condregs.begin(), condregs.end()}
    );
}

void Kernel::gate(const Unitary &u, const std::vector<size_t> &qubits) {
    kernel->gate(*(u.unitary), {qubits.begin(), qubits.end()});
}

void Kernel::classical(const CReg &destination, const Operation &operation) {
    kernel->classical(*(destination.creg), *(operation.operation));
}

void Kernel::classical(const std::string &operation) {
    kernel->classical(operation);
}

void Kernel::controlled(
    const Kernel &k,
    const std::vector<size_t> &control_qubits,
    const std::vector<size_t> &ancilla_qubits
) {
    kernel->controlled(*k.kernel, {control_qubits.begin(), control_qubits.end()}, {ancilla_qubits.begin(), ancilla_qubits.end()});
}

void Kernel::conjugate(const Kernel &k) {
    kernel->conjugate(*k.kernel);
}

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
    QL_WOUT("Program(name,Platform,#qbit,#creg,#breg) API will soon be deprecated according to issue #266 - OpenQL v0.9");
    program.emplace(name, platform.platform, qubit_count, creg_count, breg_count);
}

void Program::set_sweep_points(const std::vector<double> &sweep_points) {
    QL_WOUT("This will soon be deprecated according to issue #76");
    using Annotation = ql::pass::io::sweep_points::Annotation;
    if (!program->has_annotation<Annotation>()) {
        program->set_annotation<Annotation>({});
    }
    program->get_annotation<Annotation>().data = sweep_points;
}

std::vector<double> Program::get_sweep_points() const {
    QL_WOUT("This will soon be deprecated according to issue #76");
    using Annotation = ql::pass::io::sweep_points::Annotation;
    auto annot = program->get_annotation_ptr<Annotation>();
    if (annot == nullptr) {
        return {};
    } else {
        return {annot->data.begin(), annot->data.end()};
    }
}

void Program::set_config_file(const std::string &config_file_name) {
    QL_WOUT("This will soon be deprecated according to issue #76");
    using Annotation = ql::pass::io::sweep_points::Annotation;
    if (!program->has_annotation<Annotation>()) {
        program->set_annotation<Annotation>({});
    }
    program->get_annotation<Annotation>().config_file_name
        = get_option("output_dir") + "/" + config_file_name;
}


void Program::add_kernel(Kernel &k) {
    program->add(k.kernel);
}

void Program::add_program(Program &p) {
    program->add_program(p.program);
}

void Program::add_if(Kernel &k, const Operation &operation) {
    program->add_if(k.kernel, *operation.operation);
}

void Program::add_if(Program &p, const Operation &operation) {
    program->add_if(p.program, *operation.operation);
}

void Program::add_if_else(Kernel &k_if, Kernel &k_else, const Operation &operation) {
    program->add_if_else(k_if.kernel, k_else.kernel, *(operation.operation));
}

void Program::add_if_else(Program &p_if, Program &p_else, const Operation &operation) {
    program->add_if_else(p_if.program, p_else.program, *(operation.operation));
}

void Program::add_do_while(Kernel &k, const Operation &operation) {
    program->add_do_while(k.kernel, *operation.operation);
}

void Program::add_do_while(Program &p, const Operation &operation) {
    program->add_do_while(p.program, *operation.operation);
}

void Program::add_for(Kernel &k, size_t iterations) {
    program->add_for(k.kernel, iterations);
}

void Program::add_for(Program &p, size_t iterations) {
    program->add_for(p.program, iterations);
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
    QL_WOUT("compiling " << name << " ...");
    if (program->kernels.empty()) {
        QL_FATAL("compiling a program with no kernels !");
    }
    ql::pmgr::Manager::from_defaults(program->platform).compile(program);
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
        ql::com::options::get("output_dir") + "/",
        program
    );
}

cQasmReader::cQasmReader(
    const Platform &q_platform,
    const Program &q_program
) :
    platform(q_platform),
    program(q_program)
{
    cqasm_reader.emplace(platform.platform, program.program);
}

cQasmReader::cQasmReader(
    const Platform &q_platform,
    const Program &q_program,
    const std::string &gateset_fname
) :
    platform(q_platform),
    program(q_program)
{
    cqasm_reader.emplace(platform.platform, program.program, gateset_fname);
}

void cQasmReader::string2circuit(const std::string &cqasm_str) {
    cqasm_reader->string2circuit(cqasm_str);
}

void cQasmReader::file2circuit(const std::string &cqasm_file_path) {
    cqasm_reader->file2circuit(cqasm_file_path);
}

} // namespace api
} // namespace ql
