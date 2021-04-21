/** \file
 * API header for using quantum kernels.
 */

#pragma once

#include "ql/ir/ir.h"
#include "ql/api/declarations.h"
#include "ql/api/platform.h"

//============================================================================//
//                               W A R N I N G                                //
//----------------------------------------------------------------------------//
//         Docstrings in this file must manually be kept in sync with         //
//      kernel.i! This should be automated at some point, but isn't yet.      //
//============================================================================//

namespace ql {
namespace api {

/**
 * Represents a kernel of a quantum program, a.k.a. a basic block. Kernels are
 * just sequences of gates with no classical control-flow in between.
 */
class Kernel {
private:
    friend class Program;

    /**
     * The wrapped kernel object.
     */
    ql::ir::KernelRef kernel;

public:

    /**
     * The name of the kernel as given by the user.
     */
    const std::string name;

    /**
     * The platform that the kernel was built for.
     */
    const Platform platform;

    /**
     * The number of (virtual) qubits allocated for the kernel.
     */
    const size_t qubit_count;

    /**
     * The number of classical integer registers allocated for the kernel.
     */
    const size_t creg_count;

    /**
     * The number of classical bit registers allocated for the kernel.
     */
    const size_t breg_count;

    /**
     * Creates a new kernel with the given name, using the given platform.
     * The third, fourth, and fifth arguments optionally specify the desired
     * number of qubits, classical integer registers, and classical bit
     * registers. If not specified, the number of qubits is taken from the
     * platform, and no classical or bit registers will be allocated.
     */
    Kernel(
        const std::string &name,
        const Platform &platform,
        size_t qubit_count = 0,
        size_t creg_count = 0,
        size_t breg_count = 0
    );

    /**
     * Shorthand for an "identity" gate with a single qubit.
     */
    void identity(size_t q0);

    /**
     * Shorthand for a "hadamard" gate with a single qubit.
     */
    void hadamard(size_t q0);

    /**
     * Shorthand for a "s" gate with a single qubit.
     */
    void s(size_t q0);

    /**
     * Shorthand for a "sdag" gate with a single qubit.
     */
    void sdag(size_t q0);

    /**
     * Shorthand for a "t" gate with a single qubit.
     */
    void t(size_t q0);

    /**
     * Shorthand for a "tdag" gate with a single qubit.
     */
    void tdag(size_t q0);

    /**
     * Shorthand for a "x" gate with a single qubit.
     */
    void x(size_t q0);

    /**
     * Shorthand for a "y" gate with a single qubit.
     */
    void y(size_t q0);

    /**
     * Shorthand for a "z" gate with a single qubit.
     */
    void z(size_t q0);

    /**
     * Shorthand for an "rx90" gate with a single qubit.
     */
    void rx90(size_t q0);

    /**
     * Shorthand for an "mrx90" gate with a single qubit.
     */
    void mrx90(size_t q0);

    /**
     * Shorthand for an "rx180" gate with a single qubit.
     */
    void rx180(size_t q0);

    /**
     * Shorthand for an "ry90" gate with a single qubit.
     */
    void ry90(size_t q0);

    /**
     * Shorthand for an "mry90" gate with a single qubit.
     */
    void mry90(size_t q0);

    /**
     * Shorthand for an "ry180" gate with a single qubit.
     */
    void ry180(size_t q0);

    /**
     * Shorthand for an "rx" gate with a single qubit and the given rotation in
     * radians.
     */
    void rx(size_t q0, double angle);

    /**
     * Shorthand for an "ry" gate with a single qubit and the given rotation in
     * radians.
     */
    void ry(size_t q0, double angle);

    /**
     * Shorthand for an "rz" gate with a single qubit and the given rotation in
     * radians.
     */
    void rz(size_t q0, double angle);

    /**
     * Shorthand for a "measure" gate with a single qubit and implicit result
     * bit register.
     */
    void measure(size_t q0);

    /**
     * Shorthand for a "measure" gate with a single qubit and explicit result
     * bit register.
     */
    void measure(size_t q0, size_t b0);

    /**
     * Shorthand for a "prepz" gate with a single qubit.
     */
    void prepz(size_t q0);

    /**
     * Shorthand for a "cnot" gate with two qubits.
     */
    void cnot(size_t q0, size_t q1);

    /**
     * Shorthand for a "cphase" gate with two qubits.
     */
    void cphase(size_t q0, size_t q1);

    /**
     * Shorthand for a "cz" gate with two qubits.
     */
    void cz(size_t q0, size_t q1);

    /**
     * Shorthand for a "toffoli" gate with three qubits.
     */
    void toffoli(size_t q0, size_t q1, size_t q2);

    /**
     * Shorthand for the Clifford gate with the specific number using the
     * minimal number of rx90, rx180, mrx90, ry90, ry180, mry90 and Y gates.
     * These are as follows:
     *
     *  - 0: no gates inserted.
     *  - 1: ry90; rx90
     *  - 2: mrx90, mry90
     *  - 3: rx180
     *  - 4: mry90, mrx90
     *  - 5: rx90, mry90
     *  - 6: ry180
     *  - 7: mry90, rx90
     *  - 8: rx90, ry90
     *  - 9: rx180, ry180
     *  - 10: ry90, mrx90
     *  - 11: mrx90, ry90
     *  - 12: ry90, rx180
     *  - 13: mrx90
     *  - 14: rx90, mry90, mrx90
     *  - 15: mry90
     *  - 16: rx90
     *  - 17: rx90, ry90, rx90
     *  - 18: mry90, rx180
     *  - 19: rx90, ry180
     *  - 20: rx90, mry90, rx90
     *  - 21: ry90
     *  - 22: mrx90, ry180
     *  - 23: rx90, ry90, mrx90
     */
    void clifford(int id, size_t q0);

    /**
     * Shorthand for a "wait" gate with the specified qubits and duration in
     * nanoseconds. If no qubits are specified, the wait applies to all qubits
     * instead (a wait with no qubits is meaningless). Note that the duration
     * will usually end up being rounded up to multiples of the platform's cycle
     * time.
     */
    void wait(const std::vector<size_t> &qubits, size_t duration);

    /**
     * Shorthand for a "wait" gate with the specified qubits and duration 0. If
     * no qubits are specified, the wait applies to all qubits instead (a wait
     * with no qubits is meaningless).
     */
    void barrier(const std::vector<size_t> &qubits = std::vector<size_t>());

    /**
     * Returns a newline-separated list of all custom gates supported by the
     * platform.
     */
    std::string get_custom_instructions() const;

    /**
     * Shorthand for a "display" gate with no qubits.
     */
    void display();

    /**
     * Shorthand for the given gate name with a single qubit.
     */
    void gate(const std::string &gname, size_t q0);

    /**
     * Shorthand for the given gate name with two qubits.
     */
    void gate(const std::string &gname, size_t q0, size_t q1);

    /**
     * Main function for adding arbitrary quantum gates.
     *
     * Note that OpenQL currently uses string comparisons with gate names all
     * over the place to derive functionality, and to derive what the actual
     * arguments do. This is inherently a bad idea and something we want to
     * move away from, so documenting it all would not be worthwhile.
     *
     * For conditional gates, the following condition strings are supported:
     *
     *  - "COND_ALWAYS" or "1": no condition; gate is always executed.
     *  - "COND_NEVER" or "0": no condition; gate is never executed.
     *  - "COND_UNARY" or "" (empty): gate is executed if the single bit
     *    specified via condregs is 1.
     *  - "COND_NOT" or "!": gate is executed if the single bit specified via
     *    condregs is 0.
     *  - "COND_AND" or "&": gate is executed if the two bits specified via
     *    condregs are both 1.
     *  - "COND_NAND" or "!&": gate is executed if either of the two bits
     *    specified via condregs is zero.
     *  - "COND_OR" or "|": gate is executed if either of the two bits specified
     *    via condregs is one.
     *  - "COND_NOR" or "1": no condition; gate is always executed.
     */
    void gate(
        const std::string &name,
        const std::vector<size_t> &qubits,
        size_t duration = 0,
        double angle = 0.0,
        const std::vector<size_t> &bregs = {},
        const std::string &condstring = "COND_ALWAYS",
        const std::vector<size_t> &condregs = {}
    );

    /**
     * Alternative function for adding normal conditional quantum gates. Avoids
     * having to specify duration, angle, and bregs.
     */
    void condgate(
        const std::string &name,
        const std::vector<size_t> &qubits,
        const std::string &condstring,
        const std::vector<size_t> &condregs
    );

    /**
     * Main function for mixed quantum-classical gates involving integer
     * registers.
     */
    void gate(
        const std::string &name,
        const std::vector<size_t> &qubits,
        const CReg &destination
    );

    /**
     * Adds a unitary gate to the circuit. The size of the unitary gate must of
     * course align with the number of qubits presented.
     */
    void gate(const Unitary &u, const std::vector<size_t> &qubits);

    /**
     * Adds a classical assignment gate to the circuit. The classical integer
     * register is assigned to the result of the given operation.
     */
    void classical(const CReg &destination, const Operation &operation);

    /**
     * Adds a classical gate without operands. Only "nop" is currently (more or
     * less) supported.
     */
    void classical(const std::string &operation);

    /**
     * Sets the condition for all gates subsequently added to this kernel.
     * Thus, essentially shorthand notation. Reset with gate_clear_condition().
     */
    void gate_preset_condition(
        const std::string &condstring,
        const std::vector<size_t> &condregs
    );

    /**
     * Clears a condition previously set via gate_preset_condition().
     */
    void gate_clear_condition();

    /**
     * Adds a controlled kernel. The number of control and ancilla qubits must
     * be equal.
     *
     * NOTE: this high-level functionality is poorly/not maintained, and relies
     * on default gates, which are on the list for removal.
     */
    void controlled(
        const Kernel &k,
        const std::vector<size_t> &control_qubits,
        const std::vector<size_t> &ancilla_qubits
    );

    /**
     * Adds the conjugate of the given kernel to this kernel.
     *
     * NOTE: this high-level functionality is poorly/not maintained, and relies
     * on default gates, which are on the list for removal.
     */
    void conjugate(const Kernel &k);

};

} // namespace api
} // namespace ql
