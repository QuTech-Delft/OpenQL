
%feature("docstring") ql::api::Kernel
"""
Represents a kernel of a quantum program, a.k.a. a basic block. Kernels are
just sequences of gates with no classical control-flow in between: they may
end in a (conditional) branch to the start of another kernel, but otherwise,
they may only consist of quantum gates and mixed quantum-classical data flow
operations.

The constructor creates a new kernel with the given name, using the given
platform. The third, fourth, and fifth arguments optionally specify the
desired number of qubits, classical integer registers, and classical bit
registers. If not specified, the number of qubits is taken from the
platform, and no classical or bit registers will be allocated.

Currently, the contents of a kernel can only be constructed by adding gates
and classical data flow instructions in the order in which they are to be
executed, and there is no way to get information about which gates are in the
kernel after the fact. If you need this kind of bookkeeping, you will have to
wrap OpenQL's kernels for now.

Classical flow-control is configured when a completed kernel is added to a
program, via basic structured control-flow paradigms (if-else, do-while, and
loops with a fixed iteration count).

NOTE: the way gates are represented in OpenQL is on the list to be completely
revised. Currently OpenQL works using a mixture of \"default gates\" and the
\"custom gates\" that you can specify in the platform configuration file, but
these two things are not orthogonal and largely incompatible with each other,
yet are currently used interchangeably. Furthermore, there is no proper way
to specify lists of generic arguments to a gate, leading to lots of code
duplication inside OpenQL and long gate() argument lists. Finally, the
semantics of gates are largely derived by undocumented and somewhat heuristic
string comparisons with the names of gates, which is terrible design in
combination with user-specified instruction sets via the platform
configuration file. The interface for adding simple *quantum* gates to a
kernel is something we want to keep 100% backward compatible, but the more
advanced gate() signatures may change in the (near) future.

NOTE: classical logic is on the list to be completely revised. This interface
may change in the (near) future.

NOTE: the higher-order functions for constructing controlled kernels and
conjugating kernels have not been maintained for a while and thus probably
won't work right. They may be removed entirely in a later version of OpenQL.
"""


%feature("docstring") ql::api::Kernel::name
"""
The name of the kernel as given by the user.
"""


%feature("docstring") ql::api::Kernel::platform
"""
The platform that the kernel was built for.
"""


%feature("docstring") ql::api::Kernel::qubit_count
"""
The number of (virtual) qubits allocated for the kernel.
"""


%feature("docstring") ql::api::Kernel::creg_count
"""
The number of classical integer registers allocated for the kernel.
"""


%feature("docstring") ql::api::Kernel::breg_count
"""
The number of classical bit registers allocated for the kernel.
"""


%feature("docstring") ql::api::Kernel::get_custom_instructions
"""
Old alias for dump_custom_instructions(). Deprecated.

Parameters
----------
None

Returns
-------
str
    A newline-separated list of all custom gates supported by the platform.
"""


%feature("docstring") ql::api::Kernel::print_custom_instructions
"""
Prints a list of all custom gates supported by the platform.

Parameters
----------
None

Returns
-------
None
"""


%feature("docstring") ql::api::Kernel::dump_custom_instructions
"""
Returns the result of print_custom_instructions() as a string.

Parameters
----------
None

Returns
-------
str
    A newline-separated list of all custom gates supported by the platform.
"""


%feature("docstring") ql::api::Kernel::gate_preset_condition
"""
Sets the condition for all gates subsequently added to this kernel.
Thus, essentially shorthand notation. Reset with gate_clear_condition().

Parameters
----------
condstring : str
    Must be one of:

     - \"COND_ALWAYS\" or \"1\": no condition; gate is always executed.
     - \"COND_NEVER\" or \"0\": no condition; gate is never executed.
     - \"COND_UNARY\" or \"\" (empty): gate is executed if the single bit
       specified via condregs is 1.
     - \"COND_NOT\" or \"!\": gate is executed if the single bit specified via
       condregs is 0.
     - \"COND_AND\" or \"&\": gate is executed if the two bits specified via
       condregs are both 1.
     - \"COND_NAND\" or \"!&\": gate is executed if either of the two bits
       specified via condregs is zero.
     - \"COND_OR\" or \"|\": gate is executed if either of the two bits specified
       via condregs is one.
     - \"COND_NOR\" or \"!|\": no condition; gate is always executed.

condregs : List[int]
    Depending on condstring, must be a list of 0, 1, or 2 breg indices.

Returns
-------
None
"""


%feature("docstring") ql::api::Kernel::gate_clear_condition
"""
Clears a condition previously set via gate_preset_condition().

Parameters
----------
None

Returns
-------
None
"""


%feature("docstring") ql::api::Kernel::gate
"""
Main function for appending arbitrary quantum gates.

Parameters
----------
name : str
    The name of the gate. Note that OpenQL currently uses string comparisons
    with these names all over the place to derive functionality, and to derive
    what the actual arguments do. This is inherently a bad idea and something
    we want to move away from, so documenting it all would not be worthwhile.
    For now, just use common sense, and you'll probably be okay.

q0 : int
    Index of the first qubit to apply the gate to. For controlled gates, this
    is the control qubit.

q1 : int
    Index of the second qubit to apply the gate to. For controlled gates, this
    is the target qubit.

qubits : List[int]
    The full list of qubit indices to apply the gate to.

duration : int
    Gate duration in nanoseconds, or 0 to use the default value from the
    platform configuration file. This is primarily intended to be used for wait
    gates.

angle : float
    Rotation angle in radians for gates that use it (rx, ry, rz, etc). Ignored
    for all other gates.

bregs : List[int]
    The full list of bit register argument indices for the gate, excluding
    any bit registers used for conditional execution. Currently only used for
    the measure gate, which may be given an explicit bit register index to
    return its result in. If no such register is specified, the result is
    assumed to implicitly go to the bit register with the same index as the
    qubit being measured. Ignored for gates that don't use bit registers.

condstring : str
    If specified, must be one of:

     - \"COND_ALWAYS\" or \"1\": no condition; gate is always executed.
     - \"COND_NEVER\" or \"0\": no condition; gate is never executed.
     - \"COND_UNARY\" or \"\" (empty): gate is executed if the single bit
       specified via condregs is 1.
     - \"COND_NOT\" or \"!\": gate is executed if the single bit specified via
       condregs is 0.
     - \"COND_AND\" or \"&\": gate is executed if the two bits specified via
       condregs are both 1.
     - \"COND_NAND\" or \"!&\": gate is executed if either of the two bits
       specified via condregs is zero.
     - \"COND_OR\" or \"|\": gate is executed if either of the two bits specified
       via condregs is one.
     - \"COND_NOR\" or \"!|\": no condition; gate is always executed.

condregs : List[int]
    Depending on condstring, must be a list of 0, 1, or 2 breg indices.

destination : CReg
    An integer control register that receives the result of the mixed
    quantum-classical gate identified by name.

u : Unitary
    The unitary gate to insert.

Returns
-------
None
"""


%feature("docstring") ql::api::Kernel::condgate
"""
Alternative function for appending normal conditional quantum gates. Avoids
having to specify duration, angle, and bregs for gates that don't need it.

Parameters
----------
name : str
    The name of the gate. Note that OpenQL currently uses string comparisons
    with these names all over the place to derive functionality, and to derive
    what the actual arguments do. This is inherently a bad idea and something
    we want to move away from, so documenting it all would not be worthwhile.
    For now, just use common sense, and you'll probably be okay.

qubits : List[int]
    The full list of qubit indices to apply the gate to.

condstring : str
    If specified, must be one of:

     - \"COND_ALWAYS\" or \"1\": no condition; gate is always executed.
     - \"COND_NEVER\" or \"0\": no condition; gate is never executed.
     - \"COND_UNARY\" or \"\" (empty): gate is executed if the single bit
       specified via condregs is 1.
     - \"COND_NOT\" or \"!\": gate is executed if the single bit specified via
       condregs is 0.
     - \"COND_AND\" or \"&\": gate is executed if the two bits specified via
       condregs are both 1.
     - \"COND_NAND\" or \"!&\": gate is executed if either of the two bits
       specified via condregs is zero.
     - \"COND_OR\" or \"|\": gate is executed if either of the two bits specified
       via condregs is one.
     - \"COND_NOR\" or \"!|\": no condition; gate is always executed.

condregs : List[int]
    Depending on condstring, must be a list of 0, 1, or 2 breg indices.

Returns
-------
None
"""


%feature("docstring") ql::api::Kernel::classical
"""
Appends a classical assignment gate to the circuit. The classical integer
register is assigned to the result of the given operation.

Parameters
----------
destination : CReg
    An integer control register at the left-hand side of the classical
    assignment gate.

operation : Operation
    The expression to evaluate on the right-hand side of the classical
    assignment gate.

Returns
-------
None
"""


%feature("docstring") ql::api::Kernel::identity
"""
Shorthand for an \"identity\" gate with a single qubit.

Parameters
----------
q0 : int
    The qubit to apply the gate to.

Returns
-------
None
"""


%feature("docstring") ql::api::Kernel::hadamard
"""
Shorthand for appending a \"hadamard\" gate with a single qubit.

Parameters
----------
q0 : int
    The qubit to apply the gate to.

Returns
-------
None
"""


%feature("docstring") ql::api::Kernel::s
"""
Shorthand for appending an \"s\" gate with a single qubit.

Parameters
----------
q0 : int
    The qubit to apply the gate to.

Returns
-------
None
"""


%feature("docstring") ql::api::Kernel::sdag
"""
Shorthand for appending an \"sdag\" gate with a single qubit.

Parameters
----------
q0 : int
    The qubit to apply the gate to.

Returns
-------
None
"""


%feature("docstring") ql::api::Kernel::t
"""
Shorthand for appending a \"t\" gate with a single qubit.

Parameters
----------
q0 : int
    The qubit to apply the gate to.

Returns
-------
None
"""


%feature("docstring") ql::api::Kernel::tdag
"""
Shorthand for appending a \"tdag\" gate with a single qubit.

Parameters
----------
q0 : int
    The qubit to apply the gate to.

Returns
-------
None
"""


%feature("docstring") ql::api::Kernel::x
"""
Shorthand for appending an \"x\" gate with a single qubit.

Parameters
----------
q0 : int
    The qubit to apply the gate to.

Returns
-------
None
"""


%feature("docstring") ql::api::Kernel::y
"""
Shorthand for appending a \"y\" gate with a single qubit.

Parameters
----------
q0 : int
    The qubit to apply the gate to.

Returns
-------
None
"""


%feature("docstring") ql::api::Kernel::z
"""
Shorthand for appending a \"z\" gate with a single qubit.

Parameters
----------
q0 : int
    The qubit to apply the gate to.

Returns
-------
None
"""


%feature("docstring") ql::api::Kernel::rx90
"""
Shorthand for appending an \"rx90\" gate with a single qubit.

Parameters
----------
q0 : int
    The qubit to apply the gate to.

Returns
-------
None
"""


%feature("docstring") ql::api::Kernel::mrx90
"""
Shorthand for appending an \"mrx90\" gate with a single qubit.

Parameters
----------
q0 : int
    The qubit to apply the gate to.

Returns
-------
None
"""


%feature("docstring") ql::api::Kernel::rx180
"""
Shorthand for appending an \"rx180\" gate with a single qubit.

Parameters
----------
q0 : int
    The qubit to apply the gate to.

Returns
-------
None
"""


%feature("docstring") ql::api::Kernel::ry90
"""
Shorthand for appending an \"ry90\" gate with a single qubit.

Parameters
----------
q0 : int
    The qubit to apply the gate to.

Returns
-------
None
"""


%feature("docstring") ql::api::Kernel::mry90
"""
Shorthand for appending an \"mry90\" gate with a single qubit.

Parameters
----------
q0 : int
    The qubit to apply the gate to.

Returns
-------
None
"""


%feature("docstring") ql::api::Kernel::ry180
"""
Shorthand for appending an \"ry180\" gate with a single qubit.

Parameters
----------
q0 : int
    The qubit to apply the gate to.

Returns
-------
None
"""


%feature("docstring") ql::api::Kernel::rx
"""
Shorthand for appending an \"rx\" gate with a single qubit and the given rotation
in radians.

Parameters
----------
q0 : int
    The qubit to apply the gate to.

angle : float
    The rotation angle in radians.

Returns
-------
None
"""


%feature("docstring") ql::api::Kernel::ry
"""
Shorthand for appending an \"ry\" gate with a single qubit and the given rotation
in radians.

Parameters
----------
q0 : int
    The qubit to apply the gate to.

angle : float
    The rotation angle in radians.

Returns
-------
None
"""


%feature("docstring") ql::api::Kernel::rz
"""
Shorthand for appending an \"rz\" gate with a single qubit and the given rotation
in radians.

Parameters
----------
q0 : int
    The qubit to apply the gate to.

angle : float
    The rotation angle in radians.

Returns
-------
None
"""


%feature("docstring") ql::api::Kernel::measure
"""
Shorthand for appending a \"measure\" gate with a single qubit and implicit or
explicit result bit register.

Parameters
----------
q0 : int
    The qubit to measure.

b0 : int
    The bit register to store the result in. If not specified, the result will
    be placed in the bit register corresponding to the index of the measured
    qubit.

Returns
-------
None
"""


%feature("docstring") ql::api::Kernel::prepz
"""
Shorthand for appending a \"prepz\" gate with a single qubit.

Parameters
----------
q0 : int
    The qubit to prepare in the Z basis.

Returns
-------
None
"""


%feature("docstring") ql::api::Kernel::cnot
"""
Shorthand for appending a \"cnot\" gate with two qubits.

Parameters
----------
q0 : int
    The control qubit index.

q1 : int
    The target qubit index

Returns
-------
None
"""


%feature("docstring") ql::api::Kernel::cphase
"""
Shorthand for appending a \"cphase\" gate with two qubits.

Parameters
----------
q0 : int
    The first qubit index.

q1 : int
    The second qubit index.

Returns
-------
None
"""


%feature("docstring") ql::api::Kernel::cz
"""
Shorthand for appending a \"cz\" gate with two qubits.

Parameters
----------
q0 : int
    The first qubit index.

q1 : int
    The second qubit index.

Returns
-------
None
"""


%feature("docstring") ql::api::Kernel::toffoli
"""
Shorthand for appending a \"toffoli\" gate with three qubits.

Parameters
----------
q0 : int
    The first control qubit index.

q1 : int
    The second control qubit index.

q2 : int
    The target qubit index.

Returns
-------
None
"""


%feature("docstring") ql::api::Kernel::clifford
"""
Shorthand for appending the Clifford gate with the specific number using
the minimal number of rx90, rx180, mrx90, ry90, ry180, and mry90 gates.

Parameters
----------
id : int
    The Clifford gate expansion index:

     - 0: no gates inserted.
     - 1: ry90; rx90
     - 2: mrx90, mry90
     - 3: rx180
     - 4: mry90, mrx90
     - 5: rx90, mry90
     - 6: ry180
     - 7: mry90, rx90
     - 8: rx90, ry90
     - 9: rx180, ry180
     - 10: ry90, mrx90
     - 11: mrx90, ry90
     - 12: ry90, rx180
     - 13: mrx90
     - 14: rx90, mry90, mrx90
     - 15: mry90
     - 16: rx90
     - 17: rx90, ry90, rx90
     - 18: mry90, rx180
     - 19: rx90, ry180
     - 20: rx90, mry90, rx90
     - 21: ry90
     - 22: mrx90, ry180
     - 23: rx90, ry90, mrx90

q0 : int
    The target qubit.

Returns
-------
None
"""


%feature("docstring") ql::api::Kernel::wait
"""
Shorthand for appending a \"wait\" gate with the specified qubits and
duration in nanoseconds. If no qubits are specified, the wait applies to
all qubits instead (a wait with no qubits is meaningless). Note that the
duration will usually end up being rounded up to multiples of the
platform's cycle time.

Parameters
----------
qubits : List[int]
    The list of qubits to apply the wait gate to. If empty, the list will be
    replaced with the set of all qubits.

duration : int
    The duration of the wait gate in nanoseconds.

Returns
-------
None
"""


%feature("docstring") ql::api::Kernel::barrier
"""
Shorthand for appending a \"wait\" gate with the specified qubits and
duration 0. If no qubits are specified, the wait applies to all qubits
instead (a wait with no qubits is meaningless).

Parameters
----------
qubits : List[int]
    The list of qubits to apply the wait gate to. If empty or unspecified, the
    list will be replaced with the set of all qubits.

Returns
-------
None
"""


%feature("docstring") ql::api::Kernel::display
"""
Shorthand for appending a \"display\" gate with no qubits.

Parameters
----------
None

Returns
-------
None
"""


%feature("docstring") ql::api::Kernel::diamond_excite_mw
"""
Appends the diamond \"excite_mw\" instruction.

Parameters
----------
envelope : int
    The envelope of the microwave.

duration : int
    The duration of the microwave in nanoseconds.

frequency : int
    The frequency of the microwave in kilohertz.

phase: int
    The phase of the microwave.

amplitude: int
    The amplitude of the microwave.

qubit: int
    The target qubit index.

Returns
-------
None
"""


%feature("docstring") ql::api::Kernel::diamond_memswap
"""
Appends the diamond \"memswap\" instruction.

Parameters
----------
qubit : int
    The index of the qubit.

nuclear_qubit : int
    The index of the nuclear spin qubit of the color center.

Returns
-------
None
"""


%feature("docstring") ql::api::Kernel::diamond_qentangle
"""
Appends the diamond \"qentangle\" instruction.

Parameters
----------
qubit : int
    The index of the qubit.

nuclear_qubit : int
    The index of the nuclear spin qubit of the color center.

Returns
-------
None
"""

%feature("docstring") ql::api::Kernel::diamond_sweep_bias
"""
Appends the diamond \"sweep_bias\" instruction.

Parameters
----------
qubit : int
    The index of the qubit.

value : int
    The value that has to be send to the dac for biasing.

dacreg: int
    The index or address of the register of the dac.

start: int
    The start frequency value of the sweep.

step: int
    The step frequency value of the sweep.

max: int
    The maximum frequency value of the sweep.

memaddress: int
    The memory address to write the results to.

Returns
-------
None
"""


%feature("docstring") ql::api::Kernel::diamond_crc
"""
Appends the diamond \"crc\" instruction.

Parameters
----------
qubit : int
    The index of the qubit.

treshold : int
    The threshold value that has to be matched.

value: int
    The value of the voltage sent to the dac.

Returns
-------
None
"""

%feature("docstring") ql::api::Kernel::diamond_rabi_check
"""
Appends the diamond \"rabi_check\" instruction.

Parameters
----------
qubit : int
    The index of the qubit.

measurements : int
    How manu measurements have to be recorded.

duration: int
    The starting value of the duration of the microwave.

t_max: int
    The value of the voltage sent to the dac.

Returns
-------
None
"""


%feature("docstring") ql::api::Kernel::controlled
"""
Appends a controlled kernel. The number of control and ancilla qubits
must be equal.

Parameters
----------
k : Kernel
    The kernel to make controlled.

control_qubits : List[int]
    The qubits that control the kernel.

ancilla_qubits : List[int]
    The ancilla qubits to use to make the kernel controlled. The number of
    ancilla qubits must equal the number of control qubits.

Returns
-------
None
"""


%feature("docstring") ql::api::Kernel::conjugate
"""
Appends the conjugate of the given kernel to this kernel.

NOTE: this high-level functionality is poorly/not maintained, and relies
on default gates, which are on the list for removal.

Parameters
----------
k : Kernel
    The kernel to conjugate.

Returns
-------
None
"""


%include "ql/api/kernel.h"
