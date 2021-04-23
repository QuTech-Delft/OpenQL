
%feature("docstring") ql::api::Program
"""
Represents a complete quantum program.

The constructor creates a new program with the given name, using the given
platform. The third, fourth, and fifth arguments optionally specify the
desired number of qubits, classical integer registers, and classical bit
registers. If not specified, the number of qubits is taken from the
platform, and no classical or bit registers will be allocated.
"""


%feature("docstring") ql::api::Program::name
"""
The name given to the program by the user.
"""


%feature("docstring") ql::api::Program::platform
"""
The platform associated with the program.
"""


%feature("docstring") ql::api::Program::qubit_count
"""
The number of (virtual) qubits allocated for the program.
"""


%feature("docstring") ql::api::Program::creg_count
"""
The number of classical integer registers allocated for the program.
"""


%feature("docstring") ql::api::Program::breg_count
"""
The number of classical bit registers allocated for the program.
"""


%feature("docstring") ql::api::Program::add_kernel
"""
Adds an unconditionally-executed kernel to the end of the program.

Parameters
----------
k : Kernel
    The kernel to add.

Returns
-------
None
"""


%feature("docstring") ql::api::Program::add_program
"""
Adds an unconditionally-executed subprogram to the end of the program.

Parameters
----------
p : Program
    The subprogram to add.

Returns
-------
None
"""


%feature("docstring") ql::api::Program::add_if
"""
Adds a conditionally-executed kernel or subprogram to the end of the program.
The kernel/subprogram will be executed if the given classical condition
evaluates to true.

Parameters
----------
k : Kernel
    The kernel to add.

p : Program
    The subprogram to add.

operation : Operation
    The operation that must evaluate to true for the kernel/subprogram to be
    executed.

Returns
-------
None
"""


%feature("docstring") ql::api::Program::add_if_else
"""
Adds two conditionally-executed kernels/subprograms with inverted conditions to
the end of the program. The first kernel/subprogram will be executed if the
given classical condition evaluates to true; the second kernel/subprogram will
be executed if it evaluates to false.

Parameters
----------
k_if : Kernel
    The kernel to execute when the condition evaluates to true.

p_if : Program
    The subprogram to execute when the condition evaluates to true.

k_else : Kernel
    The kernel to execute when the condition evaluates to false.

p_else : Program
    The subprogram to execute when the condition evaluates to false.

operation : Operation
    The operation that determines which kernel/subprogram will be executed.

Returns
-------
None
"""


%feature("docstring") ql::api::Program::add_do_while
"""
Adds a kernel/subprogram that will be repeated until the given classical
condition evaluates to true. The kernel/subprogram is executed at least once,
since the condition is evaluated at the end of the loop body.

Parameters
----------
k : Kernel
    The kernel that represents the loop body.

p : Program
    The subprogram that represents the loop body.

operation : Operation
    The operation that must evaluate to true at the end of the loop body for
    the loop body to be executed again.

Returns
-------
None
"""


%feature("docstring") ql::api::Program::add_for
"""
Adds an unconditionally-executed kernel/subprogram that will loop for the given
number of iterations.

Parameters
----------
k : Kernel
    The kernel that represents the loop body.

p : Program
    The subprogram that represents the loop body.

iterations : int
    The number of loop iterations.

Returns
-------
None
"""


%feature("docstring") ql::api::Program::set_sweep_points
"""
Sets sweep point information for the program.

NOTE: sweep points functionality is deprecated and may be removed at any time.
Do not use it in new programs.

Parameters
----------
sweep_points : List[float]
    The list of sweep points.

Returns
-------
None
"""


%feature("docstring") ql::api::Program::get_sweep_points
"""
Returns the configured sweep point information for the program.

NOTE: sweep points functionality is deprecated and may be removed at any time.
Do not use it in new programs.

Parameters
----------
None

Returns
-------
List[float]
    The previously configured sweep point information for the program, or an
    empty list if none were configured.
"""


%feature("docstring") ql::api::Program::set_config_file
"""
Sets the name of the file that the sweep points will be written to.

NOTE: sweep points functionality is deprecated and may be removed at any time.
Do not use it in new programs.

Parameters
----------
config_file_name : str
    The name of the file that the sweep points are to be written to.

Returns
-------
None
"""


%feature("docstring") ql::api::Program::has_compiler
"""
Whether a custom compiler configuration has been attached to this
program. When this is the case, it will be used to implement compile(),
rather than generating the compiler in-place from defaults and global
options during the call.

Parameters
----------
None

Returns
-------
bool
    Whether a custom compiler configuration has been attached to this program.
"""


%feature("docstring") ql::api::Program::get_compiler
"""
Returns the custom compiler configuration associated with this program.
If no such configuration exists yet, the default one is created,
attached, and returned.

Parameters
----------
None

Returns
-------
Compiler
    A Compiler object that may be used to introspect or modify the compilation
    strategy associated with this program.
"""


%feature("docstring") ql::api::Program::set_compiler
"""
Sets the compiler associated with this program. It will then be used for
compile().

Parameters
----------
compiler : Compiler
    The new compiler configuration.

Returns
-------
None
"""


%feature("docstring") ql::api::Program::compile
"""
Compiles the program.

Parameters
----------
None

Returns
-------
None
"""


%feature("docstring") ql::api::Program::print_interaction_matrix
"""
Prints the interaction matrix for each kernel in the program.

Parameters
----------
None

Returns
-------
None
"""


%feature("docstring") ql::api::Program::write_interaction_matrix
"""
Writes the interaction matrix for each kernel in the program to a file.
This is one of the few functions that still uses the global output_dir
option.

Parameters
----------
None

Returns
-------
None
"""


%include "ql/api/program.h"
