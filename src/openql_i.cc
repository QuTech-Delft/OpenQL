/** \file
 * Implementation for Python interface classes.
 */

#include "openql_i.h"

#include "version.h"

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
    ql::options::global.reset();
}

std::string get_version() {
    return OPENQL_VERSION_STRING;
}

void set_option(const std::string &option_name, const std::string &option_value) {
    if (!initialized) {
        QL_WOUT("option set before initialize()! In the future, please call initialize() before anything else!");
        initialize();
    }
    ql::options::global[option_name] = option_value;
}

std::string get_option(const std::string &option_name) {
    return ql::options::global[option_name].as_str();
}

void print_options() {
    ql::options::global.help();
}

Platform::Platform() {}

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
    platform = new ql::quantum_platform(name, config_file);
}

size_t Platform::get_qubit_number() const {
    return platform->get_qubit_number();
}

CReg::CReg(size_t id) {
    creg = new ql::creg(id);
}

CReg::~CReg() {
    delete(creg);
}

Operation::Operation(const CReg &lop, const std::string &op, const CReg &rop) {
    operation = new ql::operation(*(lop.creg), op, *(rop.creg));
}

Operation::Operation(const std::string &op, const CReg &rop) {
    operation = new ql::operation(op, *(rop.creg));
}

Operation::Operation(const CReg &lop) {
    operation = new ql::operation(*(lop.creg));
}

Operation::Operation(int val) {
    operation = new ql::operation(val);
}

Operation::~Operation() {
    delete(operation);
}

Unitary::Unitary(
    const std::string &name,
    const std::vector<std::complex<double>> &matrix
) :
    name(name)
{
    unitary = new ql::unitary(name, {matrix.begin(), matrix.end()});
}

Unitary::~Unitary() {
    delete(unitary);
}

void Unitary::decompose() {
    unitary->decompose();
}

bool Unitary::is_decompose_support_enabled() {
    return ql::unitary::is_decompose_support_enabled();
}

Kernel::Kernel(const std::string &name) : name(name) {
    QL_DOUT(" API::Kernel named: " << name);
    kernel = new ql::quantum_kernel(name);
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
    kernel = new ql::quantum_kernel(name, *(platform.platform), qubit_count, creg_count, breg_count);
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
    ql::cond_type_t condvalue = kernel->condstr2condvalue(condstring);

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
    kernel->controlled(k.kernel, {control_qubits.begin(), control_qubits.end()}, {ancilla_qubits.begin(), ancilla_qubits.end()});
}

void Kernel::conjugate(const Kernel &k) {
    kernel->conjugate(k.kernel);
}

Kernel::~Kernel() {
    delete(kernel);
}

Program::Program(const std::string &name) : name(name) {
    QL_DOUT("SWIG Program(name) constructor for name: " << name);
    program = new ql::quantum_program(name);
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
    program = new ql::quantum_program(name, *(platform.platform), qubit_count, creg_count, breg_count);
}

void Program::set_sweep_points(const std::vector<double> &sweep_points) {
    QL_WOUT("This will soon be deprecated according to issue #76");
    program->sweep_points = sweep_points;
}

std::vector<double> Program::get_sweep_points() const {
    QL_WOUT("This will soon be deprecated according to issue #76");
    return std::vector<double>(program->sweep_points.begin(), program->sweep_points.end());
}

void Program::add_kernel(const Kernel &k) {
    program->add(*(k.kernel));
}

void Program::add_program(const Program &p) {
    program->add_program(*(p.program));
}

void Program::add_if(const Kernel &k, const Operation &operation) {
    program->add_if(*(k.kernel), *(operation.operation));
}

void Program::add_if(const Program &p, const Operation &operation) {
    program->add_if( *(p.program), *(operation.operation));
}

void Program::add_if_else(const Kernel &k_if, const Kernel &k_else, const Operation &operation) {
    program->add_if_else(*(k_if.kernel), *(k_else.kernel), *(operation.operation));
}

void Program::add_if_else(const Program &p_if, const Program &p_else, const Operation &operation) {
    program->add_if_else(*(p_if.program), *(p_else.program), *(operation.operation));
}

void Program::add_do_while(const Kernel &k, const Operation &operation) {
    program->add_do_while(*(k.kernel), *(operation.operation));
}

void Program::add_do_while(const Program &p, const Operation &operation) {
    program->add_do_while(*(p.program), *(operation.operation));
}

void Program::add_for(const Kernel &k, size_t iterations) {
    program->add_for( *(k.kernel), iterations);
}

void Program::add_for(const Program &p, size_t iterations) {
    program->add_for( *(p.program), iterations);
}

void Program::compile() {
    //program->compile();
    program->compile_modular();
}

std::string Program::microcode() const {
#if OPT_MICRO_CODE
    return program->microcode();
#else
    return std::string("microcode disabled");
#endif
}

void Program::print_interaction_matrix() const {
    program->print_interaction_matrix();
}

void Program::write_interaction_matrix() const {
    program->write_interaction_matrix();
}

Program::~Program() {
    // std::cout << "program::~program()" << std::endl;
    // leave deletion to SWIG, otherwise the python unit test framework fails
    // FIXME JvS: above is impressively broken, this just means it's never
    //  deleted. It's not like SWIG has some magical garbage collector or
    //  something.
    //delete(program);
}

cQasmReader::cQasmReader(
    const Platform &q_platform,
    const Program &q_program
) :
    platform(q_platform),
    program(q_program)
{
    cqasm_reader_ = new ql::cqasm_reader(*(platform.platform), *(program.program));
}

cQasmReader::cQasmReader(
    const Platform &q_platform,
    const Program &q_program,
    const std::string &gateset_fname
) :
    platform(q_platform),
    program(q_program)
{
    cqasm_reader_ = new ql::cqasm_reader(*(platform.platform), *(program.program), gateset_fname);
}

void cQasmReader::string2circuit(const std::string &cqasm_str) {
    cqasm_reader_->string2circuit(cqasm_str);
}

void cQasmReader::file2circuit(const std::string &cqasm_file_path) {
    cqasm_reader_->file2circuit(cqasm_file_path);
}

cQasmReader::~cQasmReader() {
    // leave deletion to SWIG, otherwise the python unit test framework fails
    // FIXME JvS: above is impressively broken, this just means it's never
    //  deleted. It's not like SWIG has some magical garbage collector or
    //  something.
    // delete cqasm_reader_;
}

Compiler::Compiler(const std::string &name) : name(name) {
    compiler = new ql::quantum_compiler(name);
}

void Compiler::compile(Program &program) {
    QL_DOUT(" Compiler " << name << " compiles program  " << program.name);
    compiler->compile(program.program);
}

void Compiler::add_pass_alias(const std::string &realPassName, const std::string &symbolicPassName) {
    QL_DOUT(" Add pass " << realPassName << " under alias name  " << symbolicPassName);
    compiler->addPass(realPassName,symbolicPassName);
}

void Compiler::add_pass(const std::string &realPassName) {
    QL_DOUT(" Add pass " << realPassName << " with no alias");
    compiler->addPass(realPassName);
}

void Compiler::set_pass_option(
    const std::string &passName,
    const std::string &optionName,
    const std::string &optionValue
) {
    QL_DOUT(" Set option " << optionName << " = " << optionValue << " for pass " << passName);
    compiler->setPassOption(passName,optionName, optionValue);
}
