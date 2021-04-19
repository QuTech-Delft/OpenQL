/** \file
 * Implementation for Python interface classes.
 */

#include "ql/api/api.h"

#include "ql/version.h"
#include "ql/com/interaction_matrix.h"
#include "ql/pass/io/sweep_points/annotation.h"
#include "ql/pmgr/manager.h"

namespace ql {
namespace api {

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

std::string get_version() {
    return OPENQL_VERSION_STRING;
}

void set_option(const std::string &option_name, const std::string &option_value) {
    if (!initialized) {
        QL_WOUT("option set before initialize()! In the future, please call initialize() before anything else!");
        initialize();
    }
    ql::com::options::global[option_name] = option_value;
}

std::string get_option(const std::string &option_name) {
    return ql::com::options::global[option_name].as_str();
}

void print_options() {
    ql::com::options::global.help();
}

Platform::Platform(
    const std::string &name,
    const std::string &config_file
) :
    name(name),
    config_file(config_file)
{
    if (!initialized) {
        QL_WOUT("platform constructed before initialize()! In the future, please call initialize() before anything else!");
        initialize();
    }
    _platform.emplace(name, config_file);
}

size_t Platform::get_qubit_number() const {
    return _platform->qubit_count;
}

CReg::CReg(size_t id) {
    _creg.emplace(id);
}

Operation::Operation(const CReg &lop, const std::string &op, const CReg &rop) {
    _operation.emplace(*(lop._creg), op, *(rop._creg));
}

Operation::Operation(const std::string &op, const CReg &rop) {
    _operation.emplace(op, *(rop._creg));
}

Operation::Operation(const CReg &lop) {
    _operation.emplace(*(lop._creg));
}

Operation::Operation(int val) {
    _operation.emplace(val);
}

Unitary::Unitary(
    const std::string &name,
    const std::vector<std::complex<double>> &matrix
) :
    name(name)
{
    _unitary.emplace(name, ql::utils::Vec<ql::utils::Complex>(matrix.begin(), matrix.end()));
}

void Unitary::decompose() {
    _unitary->decompose();
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
    _kernel.emplace(name, platform._platform, qubit_count, creg_count, breg_count);
}

void Kernel::identity(size_t q0) {
    _kernel->identity(q0);
}

void Kernel::hadamard(size_t q0) {
    _kernel->hadamard(q0);
}

void Kernel::s(size_t q0) {
    _kernel->s(q0);
}

void Kernel::sdag(size_t q0) {
    _kernel->sdag(q0);
}

void Kernel::t(size_t q0) {
    _kernel->t(q0);
}

void Kernel::tdag(size_t q0) {
    _kernel->tdag(q0);
}

void Kernel::x(size_t q0) {
    _kernel->x(q0);
}

void Kernel::y(size_t q0) {
    _kernel->y(q0);
}

void Kernel::z(size_t q0) {
    _kernel->z(q0);
}

void Kernel::rx90(size_t q0) {
    _kernel->rx90(q0);
}

void Kernel::mrx90(size_t q0) {
    _kernel->mrx90(q0);
}

void Kernel::rx180(size_t q0) {
    _kernel->rx180(q0);
}

void Kernel::ry90(size_t q0) {
    _kernel->ry90(q0);
}

void Kernel::mry90(size_t q0) {
    _kernel->mry90(q0);
}

void Kernel::ry180(size_t q0) {
    _kernel->ry180(q0);
}

void Kernel::rx(size_t q0, double angle) {
    _kernel->rx(q0, angle);
}

void Kernel::ry(size_t q0, double angle) {
    _kernel->ry(q0, angle);
}

void Kernel::rz(size_t q0, double angle) {
    _kernel->rz(q0, angle);
}

void Kernel::measure(size_t q0) {
    QL_DOUT("Python k.measure([" << q0 << "])");
    _kernel->measure(q0);
}

void Kernel::measure(size_t q0, size_t b0) {
    QL_DOUT("Python k.measure([" << q0 << "], [" << b0 << "])");
    _kernel->measure(q0, b0);
}

void Kernel::prepz(size_t q0) {
    _kernel->prepz(q0);
}

void Kernel::cnot(size_t q0, size_t q1) {
    _kernel->cnot(q0,q1);
}

void Kernel::cphase(size_t q0, size_t q1) {
    _kernel->cphase(q0,q1);
}

void Kernel::cz(size_t q0, size_t q1) {
    _kernel->cz(q0,q1);
}

void Kernel::toffoli(size_t q0, size_t q1, size_t q2) {
    _kernel->toffoli(q0,q1,q2);
}

void Kernel::clifford(int id, size_t q0) {
    _kernel->clifford(id, q0);
}

void Kernel::wait(const std::vector<size_t> &qubits, size_t duration) {
    _kernel->wait({qubits.begin(), qubits.end()}, duration);
}

void Kernel::barrier(const std::vector<size_t> &qubits) {
    _kernel->wait({qubits.begin(), qubits.end()}, 0);
}

std::string Kernel::get_custom_instructions() const {
    return _kernel->get_gates_definition();
}

void Kernel::display() {
    _kernel->display();
}

void Kernel::gate(const std::string &gname, size_t q0) {
    _kernel->gate(gname, q0);
}

void Kernel::gate(const std::string &gname, size_t q0, size_t q1) {
    _kernel->gate(gname, q0, q1);
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
    auto condvalue = _kernel->condstr2condvalue(condstring);

    _kernel->gate(
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
        << (destination._creg)->id
        << ") # (name,qubits,creg-destination)"
    );
    _kernel->gate(name, {qubits.begin(), qubits.end()}, {(destination._creg)->id} );
}

void Kernel::gate_preset_condition(
    const std::string &condstring,
    const std::vector<size_t> &condregs
) {
    QL_DOUT("Python k.gate_preset_condition("<<condstring<<", condregs)");
    _kernel->gate_preset_condition(
        _kernel->condstr2condvalue(condstring),
        {condregs.begin(), condregs.end()}
    );
}

void Kernel::gate_clear_condition() {
    QL_DOUT("Python k.gate_clear_condition()");
    _kernel->gate_clear_condition();
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
    _kernel->condgate(
        name,
        {qubits.begin(), qubits.end()},
        _kernel->condstr2condvalue(condstring),
        {condregs.begin(), condregs.end()}
    );
}

void Kernel::gate(const Unitary &u, const std::vector<size_t> &qubits) {
    _kernel->gate(*(u._unitary), {qubits.begin(), qubits.end()});
}

void Kernel::classical(const CReg &destination, const Operation &operation) {
    _kernel->classical(*(destination._creg), *(operation._operation));
}

void Kernel::classical(const std::string &operation) {
    _kernel->classical(operation);
}

void Kernel::controlled(
    const Kernel &k,
    const std::vector<size_t> &control_qubits,
    const std::vector<size_t> &ancilla_qubits
) {
    _kernel->controlled(*k._kernel, {control_qubits.begin(), control_qubits.end()}, {ancilla_qubits.begin(), ancilla_qubits.end()});
}

void Kernel::conjugate(const Kernel &k) {
    _kernel->conjugate(*k._kernel);
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
    _program.emplace(name, platform._platform, qubit_count, creg_count, breg_count);
}

void Program::set_sweep_points(const std::vector<double> &sweep_points) {
    QL_WOUT("This will soon be deprecated according to issue #76");
    using Annotation = ql::pass::io::sweep_points::Annotation;
    if (!_program->has_annotation<Annotation>()) {
        _program->set_annotation<Annotation>({});
    }
    _program->get_annotation<Annotation>().data = sweep_points;
}

std::vector<double> Program::get_sweep_points() const {
    QL_WOUT("This will soon be deprecated according to issue #76");
    using Annotation = ql::pass::io::sweep_points::Annotation;
    auto annot = _program->get_annotation_ptr<Annotation>();
    if (annot == nullptr) {
        return {};
    } else {
        return {annot->data.begin(), annot->data.end()};
    }
}

void Program::set_config_file(const std::string &config_file_name) {
    QL_WOUT("This will soon be deprecated according to issue #76");
    using Annotation = ql::pass::io::sweep_points::Annotation;
    if (!_program->has_annotation<Annotation>()) {
        _program->set_annotation<Annotation>({});
    }
    _program->get_annotation<Annotation>().config_file_name
        = get_option("output_dir") + "/" + config_file_name;
}


void Program::add_kernel(Kernel &k) {
    _program->add(k._kernel);
}

void Program::add_program(Program &p) {
    _program->add_program(p._program);
}

void Program::add_if(Kernel &k, const Operation &operation) {
    _program->add_if(k._kernel, *operation._operation);
}

void Program::add_if(Program &p, const Operation &operation) {
    _program->add_if(p._program, *operation._operation);
}

void Program::add_if_else(Kernel &k_if, Kernel &k_else, const Operation &operation) {
    _program->add_if_else(k_if._kernel, k_else._kernel, *(operation._operation));
}

void Program::add_if_else(Program &p_if, Program &p_else, const Operation &operation) {
    _program->add_if_else(p_if._program, p_else._program, *(operation._operation));
}

void Program::add_do_while(Kernel &k, const Operation &operation) {
    _program->add_do_while(k._kernel, *operation._operation);
}

void Program::add_do_while(Program &p, const Operation &operation) {
    _program->add_do_while(p._program, *operation._operation);
}

void Program::add_for(Kernel &k, size_t iterations) {
    _program->add_for(k._kernel, iterations);
}

void Program::add_for(Program &p, size_t iterations) {
    _program->add_for(p._program, iterations);
}

void Program::compile() {
    QL_IOUT("compiling " << name << " ...");
    QL_WOUT("compiling " << name << " ...");
    if (_program->kernels.empty()) {
        QL_FATAL("compiling a program with no kernels !");
    }
    ql::pmgr::Manager::from_defaults(_program->platform).compile(_program);
}

std::string Program::microcode() const {
#if OPT_MICRO_CODE
    return program->microcode();
#else
    return std::string("microcode disabled");
#endif
}

void Program::print_interaction_matrix() const {
    QL_IOUT("printing interaction matrix...");

    ql::com::InteractionMatrix::dump_for_program(_program);
}

void Program::write_interaction_matrix() const {
    ql::com::InteractionMatrix::write_for_program(
        ql::com::options::get("output_dir") + "/",
        _program
    );
}

cQasmReader::cQasmReader(
    const Platform &q_platform,
    const Program &q_program
) :
    platform(q_platform),
    program(q_program)
{
    _cqasm_reader.emplace(platform._platform, program._program);
}

cQasmReader::cQasmReader(
    const Platform &q_platform,
    const Program &q_program,
    const std::string &gateset_fname
) :
    platform(q_platform),
    program(q_program)
{
    _cqasm_reader.emplace(platform._platform, program._program, gateset_fname);
}

void cQasmReader::string2circuit(const std::string &cqasm_str) {
    _cqasm_reader->string2circuit(cqasm_str);
}

void cQasmReader::file2circuit(const std::string &cqasm_file_path) {
    _cqasm_reader->file2circuit(cqasm_file_path);
}

} // namespace api
} // namespace ql
