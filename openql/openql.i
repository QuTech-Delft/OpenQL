/**
 * @file   openql.i
 * @author Imran Ashraf
 * @brief  swig interface file
 */
%define DOCSTRING
"`OpenQL` is a C++ framework for high-level quantum programming. The framework provide a compiler for compiling and optimizing quantum code. The compiler produce the intermediate quantum assembly language and the compiled micro-code for various target platforms. While the microcode is platform-specific, the quantum assembly code (qasm) is hardware-agnostic and can be simulated on the QX simulator."
%enddef

%module(docstring=DOCSTRING) openql

%include "std_string.i"
%include "std_vector.i"

namespace std {
   %template(vectori) vector<int>;
   %template(vectorui) vector<size_t>;
   %template(vectorf) vector<float>;
   %template(vectord) vector<double>;
};

%{
#include "openql.h"
%}




%feature("docstring") init
""" Initializes OpenQL environment.

Parameters
----------
None
"""


%feature("docstring") kernel
""" kernel class which contains various quantum instructions."""


%feature("docstring") kernel::kernel
""" Constructs a kernel object.

Parameters
----------
arg1 : str
    name of the kernel
"""


%feature("docstring") kernel::identity
""" Applies identity on the qubit specified in argument.

Parameters
----------
arg1 : int
    target qubit
"""


%feature("docstring") kernel::hadamard
""" Applies hadamard on the qubit specified in argument.

Parameters
----------
arg1 : int
    target qubit
"""

%feature("docstring") kernel::s
""" Applies s on the qubit specified in argument.

Parameters
----------
arg1 : int
    target qubit
"""

%feature("docstring") kernel::sdag
""" Applies sdag on the qubit specified in argument.

Parameters
----------
arg1 : int
    target qubit
"""

%feature("docstring") kernel::s
""" Applies x on the qubit specified in argument.

Parameters
----------
arg1 : int
    target qubit
"""

%feature("docstring") kernel::y
""" Applies y on the qubit specified in argument.

Parameters
----------
arg1 : int
    target qubit
"""

%feature("docstring") kernel::z
""" Applies z on the qubit specified in argument.

Parameters
----------
arg1 : int
    target qubit
"""

%feature("docstring") kernel::rx90
""" Applies rx90 on the qubit specified in argument.

Parameters
----------
arg1 : int
    target qubit
"""


%feature("docstring") kernel::mrx90
""" Applies mrx90 on the qubit specified in argument.

Parameters
----------
arg1 : int
    target qubit
"""

%feature("docstring") kernel::rx180
""" Applies rx180 on the qubit specified in argument.

Parameters
----------
arg1 : int
    target qubit
"""

%feature("docstring") kernel::ry180
""" Applies ry180 on the qubit specified in argument.

Parameters
----------
arg1 : int
    target qubit
"""

%feature("docstring") kernel::measure
""" measures input qubit.

Parameters
----------
arg1 : int
    input qubit
"""


%feature("docstring") kernel::cnot
""" Applies controlled-not operation.

Parameters
----------
arg1 : int
    control qubit
arg2 : int
    target qubit
"""

%feature("docstring") kernel::cphase
""" Applies controlled-phase operation.

Parameters
----------
arg1 : int
    control qubit
arg2 : int
    target qubit
"""


%feature("docstring") kernel::toffoli
""" Applies controlled-controlled-not operation.

Parameters
----------
arg1 : int
    control qubit
arg2 : int
    control qubit
arg3 : int
    target qubit
"""


%feature("docstring") kernel::clifford
""" Applies clifford operation of the specified id on the qubit.

Parameters
----------
arg1 : int
    clifford operation id
arg2 : int
    target qubit

The ids and the corresponding operations are:
0 : ['I']
1 : ['Y90', 'X90']
2 : ['mX90', 'mY90']
3 : ['X180']
4 : ['mY90', 'mX90']
5 : ['X90', 'mY90']
6 : ['Y180']
7 : ['mY90', 'X90']
8 : ['X90', 'Y90']
9 : ['X180', 'Y180']
10: ['Y90', 'mX90']
11: ['mX90', 'Y90']
12: ['Y90', 'X180']
13: ['mX90']
14: ['X90', 'mY90', 'mX90']
15: ['mY90']
16: ['X90']
17: ['X90', 'Y90', 'X90']
18: ['mY90', 'X180']
19: ['X90', 'Y180']
20: ['X90', 'mY90', 'X90']
21: ['Y90']
22: ['mX90', 'Y180']
23: ['X90', 'Y90', 'mX90']
"""

%feature("docstring") kernel::load_custom_instructions
""" Loads the JSON file describing custom instructions.

Parameters
----------
arg1 : str
    Path to JSON file, default instructions.json in the current directory.
"""


%feature("docstring") kernel::print_custom_instructions
""" Prints the available custom instructions.

Parameters
----------
None
"""

%feature("docstring") kernel::toffoli
""" Applies custom gate on specified qubits.

Parameters
----------
arg1 : str
    custom gate name
arg2 : []
    list of qubits involved in custom gate.
"""


%feature("docstring") program
""" program class which contains one or more kernels."""


%feature("docstring") program::program
""" Constructs a program object.

Parameters
----------
pname : str
    name of the program
nqubits : int
    number of qubits the program will use
p       : Platform
    instance of an OpenQL Platform
"""

%feature("docstring") program::set_sweep_points
""" Sets sweep points for an experiment.

Parameters
----------
arg1 : []
    list of sweep points
arg1 : int
	number of circuits
"""

%feature("docstring") program::add_kernel
""" Adds specified kernel to program.

Parameters
----------
arg1 : kernel
    kernel to be added
"""

%feature("docstring") program::compile
""" Compiles the program.

Parameters
----------
arg1 : bool
    optimize, default is False
arg2 : bool
    verbose, default is False
"""

%feature("docstring") program::schedule
""" Schedules the program.

Parameters
----------
arg1 : string
    scheduler which can be ASAP or ALAP, default is ASAP
arg2 : bool
    verbose, default is False
"""


%feature("docstring") program::qasm
""" Returns program QASM
Parameters
----------
None

Returns
-------
str
    qasm """

%feature("docstring") program::microcode
""" Returns program microcode
Parameters
----------
None

Returns
-------
str
    microcode """




// Include the header file with above prototypes
%include "openql.h"
