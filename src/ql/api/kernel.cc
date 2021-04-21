/** \file
 * API header for using quantum kernels.
 */

#include "ql/api/kernel.h"

#include "ql/api/creg.h"
#include "ql/api/operation.h"
#include "ql/api/unitary.h"

//============================================================================//
//                               W A R N I N G                                //
//----------------------------------------------------------------------------//
//         Docstrings in this file must manually be kept in sync with         //
//   python/openql.i! This should be automated at some point, but isn't yet.  //
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
 * Shorthand for an "identity" gate with a single qubit.
 */
void Kernel::identity(size_t q0) {
    kernel->identity(q0);
}

/**
 * Shorthand for a "hadamard" gate with a single qubit.
 */
void Kernel::hadamard(size_t q0) {
    kernel->hadamard(q0);
}

/**
 * Shorthand for a "s" gate with a single qubit.
 */
void Kernel::s(size_t q0) {
    kernel->s(q0);
}

/**
 * Shorthand for a "sdag" gate with a single qubit.
 */
void Kernel::sdag(size_t q0) {
    kernel->sdag(q0);
}

/**
 * Shorthand for a "t" gate with a single qubit.
 */
void Kernel::t(size_t q0) {
    kernel->t(q0);
}

/**
 * Shorthand for a "tdag" gate with a single qubit.
 */
void Kernel::tdag(size_t q0) {
    kernel->tdag(q0);
}

/**
 * Shorthand for a "x" gate with a single qubit.
 */
void Kernel::x(size_t q0) {
    kernel->x(q0);
}

/**
 * Shorthand for a "y" gate with a single qubit.
 */
void Kernel::y(size_t q0) {
    kernel->y(q0);
}

/**
 * Shorthand for a "z" gate with a single qubit.
 */
void Kernel::z(size_t q0) {
    kernel->z(q0);
}

/**
 * Shorthand for an "rx90" gate with a single qubit.
 */
void Kernel::rx90(size_t q0) {
    kernel->rx90(q0);
}

/**
 * Shorthand for an "mrx90" gate with a single qubit.
 */
void Kernel::mrx90(size_t q0) {
    kernel->mrx90(q0);
}

/**
 * Shorthand for an "rx180" gate with a single qubit.
 */
void Kernel::rx180(size_t q0) {
    kernel->rx180(q0);
}

/**
 * Shorthand for an "ry90" gate with a single qubit.
 */
void Kernel::ry90(size_t q0) {
    kernel->ry90(q0);
}

/**
 * Shorthand for an "mry90" gate with a single qubit.
 */
void Kernel::mry90(size_t q0) {
    kernel->mry90(q0);
}

/**
 * Shorthand for an "ry180" gate with a single qubit.
 */
void Kernel::ry180(size_t q0) {
    kernel->ry180(q0);
}

/**
 * Shorthand for an "rx" gate with a single qubit and the given rotation in
 * radians.
 */
void Kernel::rx(size_t q0, double angle) {
    kernel->rx(q0, angle);
}

/**
 * Shorthand for an "ry" gate with a single qubit and the given rotation in
 * radians.
 */
void Kernel::ry(size_t q0, double angle) {
    kernel->ry(q0, angle);
}

/**
 * Shorthand for an "rz" gate with a single qubit and the given rotation in
 * radians.
 */
void Kernel::rz(size_t q0, double angle) {
    kernel->rz(q0, angle);
}

/**
 * Shorthand for a "measure" gate with a single qubit and implicit result
 * bit register.
 */
void Kernel::measure(size_t q0) {
    QL_DOUT("Python k.measure([" << q0 << "])");
    kernel->measure(q0);
}

/**
 * Shorthand for a "measure" gate with a single qubit and explicit result
 * bit register.
 */
void Kernel::measure(size_t q0, size_t b0) {
    QL_DOUT("Python k.measure([" << q0 << "], [" << b0 << "])");
    kernel->measure(q0, b0);
}

/**
 * Shorthand for a "prepz" gate with a single qubit.
 */
void Kernel::prepz(size_t q0) {
    kernel->prepz(q0);
}

/**
 * Shorthand for a "cnot" gate with two qubits.
 */
void Kernel::cnot(size_t q0, size_t q1) {
    kernel->cnot(q0,q1);
}

/**
 * Shorthand for a "cphase" gate with two qubits.
 */
void Kernel::cphase(size_t q0, size_t q1) {
    kernel->cphase(q0,q1);
}

/**
 * Shorthand for a "cz" gate with two qubits.
 */
void Kernel::cz(size_t q0, size_t q1) {
    kernel->cz(q0,q1);
}

/**
 * Shorthand for a "toffoli" gate with three qubits.
 */
void Kernel::toffoli(size_t q0, size_t q1, size_t q2) {
    kernel->toffoli(q0,q1,q2);
}

/**
 * Shorthand for the Clifford gate with the specific number using the
 * minimal number of rx90, rx180, mrx90, ry90, ry180, mry90 and Y gates.
 */
void Kernel::clifford(int id, size_t q0) {
    kernel->clifford(id, q0);
}

/**
 * Shorthand for a "wait" gate with the specified qubits and duration in
 * nanoseconds. If no qubits are specified, the wait applies to all qubits
 * instead (a wait with no qubits is meaningless). Note that the duration
 * will usually end up being rounded up to multiples of the platform's cycle
 * time.
 */
void Kernel::wait(const std::vector<size_t> &qubits, size_t duration) {
    kernel->wait({qubits.begin(), qubits.end()}, duration);
}

/**
 * Shorthand for a "wait" gate with the specified qubits and duration 0. If
 * no qubits are specified, the wait applies to all qubits instead (a wait
 * with no qubits is meaningless).
 */
void Kernel::barrier(const std::vector<size_t> &qubits) {
    kernel->wait({qubits.begin(), qubits.end()}, 0);
}

/**
 * Returns a newline-separated list of all custom gates supported by the
 * platform.
 */
std::string Kernel::get_custom_instructions() const {
    return kernel->get_gates_definition();
}

/**
 * Shorthand for a "display" gate with no qubits.
 */
void Kernel::display() {
    kernel->display();
}

/**
 * Shorthand for the given gate name with a single qubit.
 */
void Kernel::gate(const std::string &gname, size_t q0) {
    kernel->gate(gname, q0);
}

/**
 * Shorthand for the given gate name with two qubits.
 */
void Kernel::gate(const std::string &gname, size_t q0, size_t q1) {
    kernel->gate(gname, q0, q1);
}

/**
 * Main function for adding arbitrary quantum gates.
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
 * Alternative function for adding normal conditional quantum gates. Avoids
 * having to specify duration, angle, and bregs.
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
 * Main function for mixed quantum-classical gates involving integer
 * registers.
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
 * Adds a unitary gate to the circuit. The size of the unitary gate must of
 * course align with the number of qubits presented.
 */
void Kernel::gate(const Unitary &u, const std::vector<size_t> &qubits) {
    kernel->gate(*(u.unitary), {qubits.begin(), qubits.end()});
}

/**
 * Adds a classical assignment gate to the circuit. The classical integer
 * register is assigned to the result of the given operation.
 */
void Kernel::classical(const CReg &destination, const Operation &operation) {
    kernel->classical(*(destination.creg), *(operation.operation));
}

/**
 * Adds a classical gate without operands. Only "nop" is currently (more or
 * less) supported.
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
 * Adds a controlled kernel. The number of control and ancilla qubits must
 * be equal.
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
 * Adds the conjugate of the given kernel to this kernel.
 *
 * NOTE: this high-level functionality is poorly/not maintained, and relies
 * on default gates, which are on the list for removal.
 */
void Kernel::conjugate(const Kernel &k) {
    kernel->conjugate(*k.kernel);
}

} // namespace api
} // namespace ql
