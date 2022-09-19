/** \file
 * API header for using quantum kernels.
 */

#include "ql/api/kernel.h"

#include "ql/api/creg.h"
#include "ql/api/operation.h"
#include "ql/api/unitary.h"
#include "ql/arch/diamond/annotations.h"

//============================================================================//
//                               W A R N I N G                                //
//----------------------------------------------------------------------------//
//         Docstrings in this file must manually be kept in sync with         //
//      kernel.i! This should be automated at some point, but isn't yet.      //
//============================================================================//

namespace ql {
namespace api {

/**
 * Creates a new kernel with the given name, using the given platform.
 * The third, fourth, and fifth arguments optionally specify the desired
 * number of qubits, classical integer registers, and classical bit
 * registers. If not specified, the number of qubits is taken from the
 * platform, and no classical or bit registers will be allocated.
 */
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
    if (!qubit_count) qubit_count = platform.platform->qubit_count;
    kernel.emplace(name, platform.platform, qubit_count, creg_count, breg_count);
}

/**
 * Old alias for dump_custom_instructions(). Deprecated.
 */
std::string Kernel::get_custom_instructions() const {
    return kernel->get_gates_definition();
}

/**
 * Prints a list of all custom gates supported by the platform.
 */
void Kernel::print_custom_instructions() const {
    std::cout << kernel->get_gates_definition();
}

/**
 * Returns the result of print_custom_instructions() as a string.
 */
std::string Kernel::dump_custom_instructions() const {
    return kernel->get_gates_definition();
}

/**
 * Shorthand for appending the given gate name with a single qubit.
 */
void Kernel::gate(const std::string &name, size_t q0) {
    kernel->gate(name, q0);
}

/**
 * Shorthand for appending the given gate name with two qubits.
 */
void Kernel::gate(const std::string &name, size_t q0, size_t q1) {
    kernel->gate(name, q0, q1);
}

/**
 * Main function for appending arbitrary quantum gates.
 */
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

/**
 * Main function for appending mixed quantum-classical gates involving
 * integer registers.
 */
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

/**
 * Appends a unitary gate to the circuit. The size of the unitary gate must
 * of course align with the number of qubits presented.
 */
void Kernel::gate(const Unitary &u, const std::vector<size_t> &qubits) {
    kernel->gate(*(u.unitary), {qubits.begin(), qubits.end()});
}

/**
 * Automatic state preparation, currently requires unitary decomposition for all cases.
 */
void Kernel::state_prep(
    const std::vector<std::complex<double>> &states, 
    const std::vector<size_t> &qubits
) {
    kernel->state_prep({states.begin(), states.end()}, {qubits.begin(), qubits.end()});
}


/**
 * Alternative function for appending normal conditional quantum gates.
 * Avoids having to specify duration, angle, and bregs.
 */
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

/**
 * Appends a classical assignment gate to the circuit. The classical integer
 * register is assigned to the result of the given operation.
 */
void Kernel::classical(const CReg &destination, const Operation &operation) {
    kernel->classical(*(destination.creg), *(operation.operation));
}

/**
 * Appends a classical gate without operands. Only "nop" is currently (more
 * or less) supported.
 */
void Kernel::classical(const std::string &operation) {
    kernel->classical(operation);
}

/**
 * Sets the condition for all gates subsequently added to this kernel.
 * Thus, essentially shorthand notation. Reset with gate_clear_condition().
 */
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

/**
 * Clears a condition previously set via gate_preset_condition().
 */
void Kernel::gate_clear_condition() {
    QL_DOUT("Python k.gate_clear_condition()");
    kernel->gate_clear_condition();
}

/**
 * Shorthand for appending an "identity" gate with a single qubit.
 */
void Kernel::identity(size_t q0) {
    kernel->identity(q0);
}

/**
 * Shorthand for appending a "hadamard" gate with a single qubit.
 */
void Kernel::hadamard(size_t q0) {
    kernel->hadamard(q0);
}

/**
 * Shorthand for appending an "s" gate with a single qubit.
 */
void Kernel::s(size_t q0) {
    kernel->s(q0);
}

/**
 * Shorthand for appending an "sdag" gate with a single qubit.
 */
void Kernel::sdag(size_t q0) {
    kernel->sdag(q0);
}

/**
 * Shorthand for appending a "t" gate with a single qubit.
 */
void Kernel::t(size_t q0) {
    kernel->t(q0);
}

/**
 * Shorthand for appending a "tdag" gate with a single qubit.
 */
void Kernel::tdag(size_t q0) {
    kernel->tdag(q0);
}

/**
 * Shorthand for appending an "x" gate with a single qubit.
 */
void Kernel::x(size_t q0) {
    kernel->x(q0);
}

/**
 * Shorthand for appending a "y" gate with a single qubit.
 */
void Kernel::y(size_t q0) {
    kernel->y(q0);
}

/**
 * Shorthand for appending a "z" gate with a single qubit.
 */
void Kernel::z(size_t q0) {
    kernel->z(q0);
}

/**
 * Shorthand for appending an "rx90" gate with a single qubit.
 */
void Kernel::rx90(size_t q0) {
    kernel->rx90(q0);
}

/**
 * Shorthand for appending an "mrx90" gate with a single qubit.
 */
void Kernel::mrx90(size_t q0) {
    kernel->mrx90(q0);
}

/**
 * Shorthand for appending an "rx180" gate with a single qubit.
 */
void Kernel::rx180(size_t q0) {
    kernel->rx180(q0);
}

/**
 * Shorthand for appending an "ry90" gate with a single qubit.
 */
void Kernel::ry90(size_t q0) {
    kernel->ry90(q0);
}

/**
 * Shorthand for appending an "mry90" gate with a single qubit.
 */
void Kernel::mry90(size_t q0) {
    kernel->mry90(q0);
}

/**
 * Shorthand for appending an "ry180" gate with a single qubit.
 */
void Kernel::ry180(size_t q0) {
    kernel->ry180(q0);
}

/**
 * Shorthand for appending an "rx" gate with a single qubit and the given
 * rotation in radians.
 */
void Kernel::rx(size_t q0, double angle) {
    kernel->rx(q0, angle);
}

/**
 * Shorthand for appending an "ry" gate with a single qubit and the given
 * rotation in radians.
 */
void Kernel::ry(size_t q0, double angle) {
    kernel->ry(q0, angle);
}

/**
 * Shorthand for appending an "rz" gate with a single qubit and the given
 * rotation in radians.
 */
void Kernel::rz(size_t q0, double angle) {
    kernel->rz(q0, angle);
}

/**
 * Shorthand for appending a "measure" gate with a single qubit and implicit
 * result bit register.
 */
void Kernel::measure(size_t q0) {
    QL_DOUT("Python k.measure([" << q0 << "])");
    kernel->measure(q0);
}

/**
 * Shorthand for appending a "measure" gate with a single qubit and explicit
 * result bit register.
 */
void Kernel::measure(size_t q0, size_t b0) {
    QL_DOUT("Python k.measure([" << q0 << "], [" << b0 << "])");
    kernel->measure(q0, b0);
}

/**
 * Shorthand for appending a "prepz" gate with a single qubit.
 */
void Kernel::prepz(size_t q0) {
    kernel->prepz(q0);
}

/**
 * Shorthand for appending a "cnot" gate with two qubits.
 */
void Kernel::cnot(size_t q0, size_t q1) {
    kernel->cnot(q0,q1);
}

/**
 * Shorthand for appending a "cphase" gate with two qubits.
 */
void Kernel::cphase(size_t q0, size_t q1) {
    kernel->cphase(q0,q1);
}

/**
 * Shorthand for appending a "cz" gate with two qubits.
 */
void Kernel::cz(size_t q0, size_t q1) {
    kernel->cz(q0,q1);
}

/**
 * Shorthand for appending a "toffoli" gate with three qubits.
 */
void Kernel::toffoli(size_t q0, size_t q1, size_t q2) {
    kernel->toffoli(q0,q1,q2);
}

/**
 * Shorthand for appending the Clifford gate with the specific number using
 * the minimal number of rx90, rx180, mrx90, ry90, ry180, and mry90 gates.
 */
void Kernel::clifford(int id, size_t q0) {
    kernel->clifford(id, q0);
}

/**
 * Shorthand for appending a "wait" gate with the specified qubits and
 * duration in nanoseconds. If no qubits are specified, the wait applies to
 * all qubits instead (a wait with no qubits is meaningless). Note that the
 * duration will usually end up being rounded up to multiples of the
 * platform's cycle time.
 */
void Kernel::wait(const std::vector<size_t> &qubits, size_t duration) {
    kernel->wait({qubits.begin(), qubits.end()}, duration);
}

/**
 * Shorthand for appending a "wait" gate with the specified qubits and
 * duration 0. If no qubits are specified, the wait applies to all qubits
 * instead (a wait with no qubits is meaningless).
 */
void Kernel::barrier(const std::vector<size_t> &qubits) {
    kernel->wait({qubits.begin(), qubits.end()}, 0);
}

/**
 * Shorthand for appending a "display" gate with no qubits.
 */
void Kernel::display() {
    kernel->display();
}

/**
 * Appends the diamond excite_mw instruction.
 */
void Kernel::diamond_excite_mw(size_t envelope, size_t duration, size_t frequency, size_t phase, size_t amplitude, size_t qubit) {
    kernel->gate("excite_mw", qubit);
    kernel->gates.back()->set_annotation<ql::arch::diamond::annotations::ExciteMicrowaveParameters>({envelope, duration, frequency, phase, amplitude});
}

/**
 * Appends the diamond memswap instruction, that swaps the state from a qubit
 * to a nuclear spin qubit within the color center.
 */
void Kernel::diamond_memswap(size_t qubit, size_t nuclear_qubit) {
    kernel->gate("memswap", qubit);
    kernel->gates.back()->set_annotation<ql::arch::diamond::annotations::MemSwapParameters>({nuclear_qubit});
}

/**
 * Appends the diamond qentangle instruction, that entangles a qubit with a
 * nuclear spin qubit within the color center.
 */
void Kernel::diamond_qentangle(size_t qubit, size_t nuclear_qubit){
    kernel->gate("qentangle", qubit);
    kernel->gates.back()->set_annotation<ql::arch::diamond::annotations::QEntangleParameters>({nuclear_qubit});
}

/**
 * Appends the diamond sweep_bias instruction, that sweeps the frequency over
 * a color center to help determine the magnetic biasing.
 */
void Kernel::diamond_sweep_bias(size_t qubit, size_t value, size_t dacreg, size_t start, size_t step, size_t max, size_t memaddress)
{
    kernel->gate("sweep_bias", qubit);
    kernel->gates.back()->set_annotation<ql::arch::diamond::annotations::SweepBiasParameters>({value, dacreg, start, step, max, memaddress});
}

/**
 * Appends the diamond crc instruction, that checks whether the color center is
 * still in the correct charge state.
 */
void Kernel::diamond_crc(size_t qubit, size_t threshold, size_t value) {
    kernel->gate("crc", qubit);
    kernel->gates.back()->set_annotation<ql::arch::diamond::annotations::CRCParameters>({threshold, value});
}

/**
 * Appends the diamond rabi_check instruction, that measures the result of
 * an operation on a qubit to determine how long the color centers needs to
 * be excited for to have it flip.
 */
void Kernel::diamond_rabi_check(size_t qubit, size_t measurements, size_t duration, size_t t_max){
    kernel->gate("rabi_check", qubit);
    kernel->gates.back()->set_annotation<ql::arch::diamond::annotations::RabiParameters>({measurements, duration, t_max});
}

/**
 * Appends a controlled kernel. The number of control and ancilla qubits
 * must be equal.
 *
 * NOTE: this high-level functionality is poorly/not maintained, and relies
 * on default gates, which are on the list for removal.
 */
void Kernel::controlled(
    const Kernel &k,
    const std::vector<size_t> &control_qubits,
    const std::vector<size_t> &ancilla_qubits
) {
    kernel->controlled(*k.kernel, {control_qubits.begin(), control_qubits.end()}, {ancilla_qubits.begin(), ancilla_qubits.end()});
}

/**
 * Appends the conjugate of the given kernel to this kernel.
 *
 * NOTE: this high-level functionality is poorly/not maintained, and relies
 * on default gates, which are on the list for removal.
 */
void Kernel::conjugate(const Kernel &k) {
    kernel->conjugate(*k.kernel);
}

} // namespace api
} // namespace ql
