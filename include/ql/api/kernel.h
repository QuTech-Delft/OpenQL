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
 * just sequences of gates with no classical control-flow in between: they may
 * end in a (conditional) branch to the start of another kernel, but otherwise,
 * they may only consist of quantum gates and mixed quantum-classical data flow
 * operations.
 *
 * Currently, kernels can be constructed only by adding gates and classical data
 * flow instructions in the order in which they are to be executed, and there is
 * no way to get information about which gates are in the kernel after the fact.
 * If you need this kind of bookkeeping, you will have to wrap OpenQL's kernels
 * for now.
 *
 * Classical flow-control is configured when a completed kernel is added to a
 * program, via basic structured control-flow paradigms (if-else, do-while, and
 * loops with a fixed iteration count).
 *
 * NOTE: the way gates are represented in OpenQL is on the list to be completely
 * revised. Currently OpenQL works using a mixture of "default gates" and the
 * "custom gates" that you can specify in the platform configuration file, but
 * these two things are not orthogonal and largely incompatible with each other,
 * yet are currently used interchangeably. Furthermore, there is no proper way
 * to specify lists of generic arguments to a gate, leading to lots of code
 * duplication inside OpenQL and long gate() argument lists. Finally, the
 * semantics of gates are largely derived by undocumented and somewhat heuristic
 * string comparisons with the names of gates, which is terrible design in
 * combination with user-specified instruction sets via the platform
 * configuration file. The interface for adding simple *quantum* gates to a
 * kernel is something we want to keep 100% backward compatible, but the more
 * advanced gate() signatures may change in the (near) future.
 *
 * NOTE: classical logic is on the list to be completely revised. This interface
 * may change in the (near) future.
 *
 * NOTE: the higher-order functions for constructing controlled kernels and
 * conjugating kernels have not been maintained for a while and thus probably
 * won't work right. They may be removed entirely in a later version of OpenQL.
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
     * Old alias for dump_custom_instructions(). Deprecated.
     */
    std::string get_custom_instructions() const;

    /**
     * Prints a list of all custom gates supported by the platform.
     */
    void print_custom_instructions() const;

    /**
     * Returns the result of print_custom_instructions() as a string.
     */
    std::string dump_custom_instructions() const;

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
     * Shorthand for appending the given gate name with a single qubit.
     */
    void gate(const std::string &name, size_t q0);

    /**
     * Shorthand for appending the given gate name with two qubits.
     */
    void gate(const std::string &name, size_t q0, size_t q1);

    /**
     * Main function for appending arbitrary quantum gates.
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
     *  - "COND_NOR" or "!|": no condition; gate is always executed.
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
     * Main function for appending mixed quantum-classical gates involving
     * integer registers.
     */
    void gate(
        const std::string &name,
        const std::vector<size_t> &qubits,
        const CReg &destination
    );

    /**
     * Appends a unitary gate to the circuit. The size of the unitary gate must
     * of course align with the number of qubits presented.
     */
    void gate(const Unitary &u, const std::vector<size_t> &qubits);

    /**
     * Alternative function for appending normal conditional quantum gates.
     * Avoids having to specify duration, angle, and bregs.
     */
    void condgate(
        const std::string &name,
        const std::vector<size_t> &qubits,
        const std::string &condstring,
        const std::vector<size_t> &condregs
    );

    /**
     * Appends a classical assignment gate to the circuit. The classical integer
     * register is assigned to the result of the given operation.
     */
    void classical(const CReg &destination, const Operation &operation);

    /**
     * Appends a classical gate without operands. Only "nop" is currently (more
     * or less) supported.
     */
    void classical(const std::string &operation);

    /**
     * Shorthand for appending an "identity" gate with a single qubit.
     */
    void identity(size_t q0);

    /**
     * Shorthand for appending a "hadamard" gate with a single qubit.
     */
    void hadamard(size_t q0);

    /**
     * Shorthand for appending an "s" gate with a single qubit.
     */
    void s(size_t q0);

    /**
     * Shorthand for appending an "sdag" gate with a single qubit.
     */
    void sdag(size_t q0);

    /**
     * Shorthand for appending a "t" gate with a single qubit.
     */
    void t(size_t q0);

    /**
     * Shorthand for appending a "tdag" gate with a single qubit.
     */
    void tdag(size_t q0);

    /**
     * Shorthand for appending an "x" gate with a single qubit.
     */
    void x(size_t q0);

    /**
     * Shorthand for appending a "y" gate with a single qubit.
     */
    void y(size_t q0);

    /**
     * Shorthand for appending a "z" gate with a single qubit.
     */
    void z(size_t q0);

    /**
     * Shorthand for appending an "rx90" gate with a single qubit.
     */
    void rx90(size_t q0);

    /**
     * Shorthand for appending an "mrx90" gate with a single qubit.
     */
    void mrx90(size_t q0);

    /**
     * Shorthand for appending an "rx180" gate with a single qubit.
     */
    void rx180(size_t q0);

    /**
     * Shorthand for appending an "ry90" gate with a single qubit.
     */
    void ry90(size_t q0);

    /**
     * Shorthand for appending an "mry90" gate with a single qubit.
     */
    void mry90(size_t q0);

    /**
     * Shorthand for appending an "ry180" gate with a single qubit.
     */
    void ry180(size_t q0);

    /**
     * Shorthand for appending an "rx" gate with a single qubit and the given
     * rotation in radians.
     */
    void rx(size_t q0, double angle);

    /**
     * Shorthand for appending an "ry" gate with a single qubit and the given
     * rotation in radians.
     */
    void ry(size_t q0, double angle);

    /**
     * Shorthand for appending an "rz" gate with a single qubit and the given
     * rotation in radians.
     */
    void rz(size_t q0, double angle);

    /**
     * Shorthand for appending a "measure" gate with a single qubit and implicit
     * result bit register.
     */
    void measure(size_t q0);

    /**
     * Shorthand for appending a "measure" gate with a single qubit and explicit
     * result bit register.
     */
    void measure(size_t q0, size_t b0);

    /**
     * Shorthand for appending a "prepz" gate with a single qubit.
     */
    void prepz(size_t q0);

    /**
     * Shorthand for appending a "cnot" gate with two qubits.
     */
    void cnot(size_t q0, size_t q1);

    /**
     * Shorthand for appending a "cphase" gate with two qubits.
     */
    void cphase(size_t q0, size_t q1);

    /**
     * Shorthand for appending a "cz" gate with two qubits.
     */
    void cz(size_t q0, size_t q1);

    /**
     * Shorthand for appending a "toffoli" gate with three qubits.
     */
    void toffoli(size_t q0, size_t q1, size_t q2);

    /**
     * Shorthand for appending the Clifford gate with the specific number using
     * the minimal number of rx90, rx180, mrx90, ry90, ry180, and mry90 gates.
     * The expansions are as follows:
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
     * Shorthand for appending a "wait" gate with the specified qubits and
     * duration in nanoseconds. If no qubits are specified, the wait applies to
     * all qubits instead (a wait with no qubits is meaningless). Note that the
     * duration will usually end up being rounded up to multiples of the
     * platform's cycle time.
     */
    void wait(const std::vector<size_t> &qubits, size_t duration);

    /**
     * Shorthand for appending a "wait" gate with the specified qubits and
     * duration 0. If no qubits are specified, the wait applies to all qubits
     * instead (a wait with no qubits is meaningless).
     */
    void barrier(const std::vector<size_t> &qubits = std::vector<size_t>());

    /**
     * Shorthand for appending a "display" gate with no qubits.
     */
    void display();

    /**
     * Appends the diamond excite_MW instruction.
     */
    void diamond_excite_mw(size_t envelope, size_t duration, size_t frequency, size_t phase, size_t qubit);

    /**
     * Appends the diamond memswap instruction, that swaps the state from a qubit
     * to a nuclear spin qubit within the color center.
     */
    void diamond_memswap(size_t qubit, size_t nuclear_qubit);

    /**
     * Appends the diamond qentangle instruction, that entangles a qubit with a
     * nuclear spin qubit within the color center.
     */
    void diamond_qentangle(size_t qubit, size_t nuclear_qubit);

    /**
     * Appends the diamond sweep_bias instruction, that sweeps the frequency over
     * a color center to help determine the magnetic biasing.
     */
    void diamond_sweep_bias(size_t qubit, size_t value, size_t dacreg, size_t start, size_t step, size_t max, size_t memaddress);

    /**
     * Appends the diamond crc instruction, that checks whether the color center is
     * still in the correct charge state.
     */
    void diamond_crc(size_t qubit, size_t threshold, size_t value);

    /**
     * Appends a controlled kernel. The number of control and ancilla qubits
     * must be equal.
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
     * Appends the conjugate of the given kernel to this kernel.
     *
     * NOTE: this high-level functionality is poorly/not maintained, and relies
     * on default gates, which are on the list for removal.
     */
    void conjugate(const Kernel &k);

};

} // namespace api
} // namespace ql
