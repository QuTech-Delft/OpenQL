/**
 * @file   openql.i
 * @author Imran Ashraf
 * @brief  swig interface file
 */
%define DOCSTRING
"`OpenQL` is a C++ framework for high-level quantum programming. The framework provide a compiler for compiling and optimizing quantum code. The compiler produce the intermediate quantum assembly language and the compiled micro-code for various target platforms. While the microcode is platform-specific, the quantum assembly code (qasm) is hardware-agnostic and can be simulated on the QX simulator."
%enddef

%module(docstring=DOCSTRING) openql

%include "std_vector.i"
%include "exception.i"
%include "std_string.i"

namespace std {
   %template(vectori) vector<int>;
   %template(vectorui) vector<size_t>;
   %template(vectorf) vector<float>;
   %template(vectord) vector<double>;
};

%{
#include "openql.h"
%}



%pythoncode %{
import os, errno

def set_output_dir(path):
    try:
        os.makedirs(path)
    except OSError as e:
        if e.errno != errno.EEXIST:
            raise
    _openql.set_output_dir_(path)
%}



%exception
{
try
    {
        $action
        if (PyErr_Occurred()) SWIG_fail;
    }
    catch(ql::exception & e )
    {
        SWIG_exception(SWIG_TypeError, e.what());
    }
    SWIG_CATCH_STDEXCEPT
    catch(...)
    {
        SWIG_exception(SWIG_UnknownError, "Unknown C++ exception");
    }
}



%feature("docstring") set_output_dir
""" Sets output directory for the generated files. The direcoty will be created of it does not already exist.

Parameters
----------
arg1 : str
    Path to a directory.
"""

%feature("docstring") get_output_dir
""" Returns the path of current set directory for output files.

Parameters
----------
None

Returns
-------
str
    Path of output directory
"""


%feature("docstring") Platform
""" Platform class specifiying the target platform to be used for compilation."""


%feature("docstring") Platform::Platform
""" Constructs a Platform object.

Parameters
----------
arg1 : str
    name of the Platform
arg2 : str
    name of the configuration file specifying the platform
"""


%feature("docstring") Platform::get_qubit_number
""" returns number of qubits in the platform.

Parameters
----------
None

Returns
-------
int
    number of qubits
"""


%feature("docstring") Kernel
""" Kernel class which contains various quantum instructions."""


%feature("docstring") Kernel::Kernel
""" Constructs a Kernel object.

Parameters
----------
arg1 : str
    name of the Kernel
arg2 : Platform
    target platform for which the kernel will be compiled
"""


%feature("docstring") Kernel::identity
""" Applies identity on the qubit specified in argument.

Parameters
----------
arg1 : int
    target qubit
"""


%feature("docstring") Kernel::hadamard
""" Applies hadamard on the qubit specified in argument.

Parameters
----------
arg1 : int
    target qubit
"""

%feature("docstring") Kernel::s
""" Applies s on the qubit specified in argument.

Parameters
----------
arg1 : int
    target qubit
"""

%feature("docstring") Kernel::sdag
""" Applies sdag on the qubit specified in argument.

Parameters
----------
arg1 : int
    target qubit
"""

%feature("docstring") Kernel::s
""" Applies x on the qubit specified in argument.

Parameters
----------
arg1 : int
    target qubit
"""

%feature("docstring") Kernel::y
""" Applies y on the qubit specified in argument.

Parameters
----------
arg1 : int
    target qubit
"""

%feature("docstring") Kernel::z
""" Applies z on the qubit specified in argument.

Parameters
----------
arg1 : int
    target qubit
"""

%feature("docstring") Kernel::rx90
""" Applies rx90 on the qubit specified in argument.

Parameters
----------
arg1 : int
    target qubit
"""


%feature("docstring") Kernel::mrx90
""" Applies mrx90 on the qubit specified in argument.

Parameters
----------
arg1 : int
    target qubit
"""

%feature("docstring") Kernel::rx180
""" Applies rx180 on the qubit specified in argument.

Parameters
----------
arg1 : int
    target qubit
"""

%feature("docstring") Kernel::ry180
""" Applies ry180 on the qubit specified in argument.

Parameters
----------
arg1 : int
    target qubit
"""

%feature("docstring") Kernel::measure
""" measures input qubit.

Parameters
----------
arg1 : int
    input qubit
"""


%feature("docstring") Kernel::cnot
""" Applies controlled-not operation.

Parameters
----------
arg1 : int
    control qubit
arg2 : int
    target qubit
"""

%feature("docstring") Kernel::cphase
""" Applies controlled-phase operation.

Parameters
----------
arg1 : int
    control qubit
arg2 : int
    target qubit
"""


%feature("docstring") Kernel::toffoli
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


%feature("docstring") Kernel::clifford
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

%feature("docstring") Kernel::wait
""" inserts explicit wait on specified qubits.

Parameters
----------
arg1 : []
    list of qubits
arg2 : int
    duration in ns
"""


%feature("docstring") Kernel::get_custom_instructions
""" Returns list of available custom instructions.

Parameters
----------
None

Returns
-------
[]
    List of available custom instructions
"""


%feature("docstring") Kernel::gate
""" adds custom/default gates to kernel.

Parameters
----------
arg1 : str
    name of gate
arg2 : []
    list of qubits
arg3 : int
    duration in ns (at the moment it is only supported for wait instruction, in the future it will be extended to override duration of other gates as well)
"""





%feature("docstring") Program
""" Program class which contains one or more kernels."""


%feature("docstring") Program::Program
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

%feature("docstring") Program::set_sweep_points
""" Sets sweep points for an experiment.

Parameters
----------
arg1 : []
    list of sweep points
arg1 : int
	number of sweep points
"""

%feature("docstring") Program::add_kernel
""" Adds specified kernel to program.

Parameters
----------
arg1 : kernel
    kernel to be added
"""

%feature("docstring") Program::compile
""" Compiles the program.

Parameters
----------
arg1 : bool
    optimize, default is False
arg2 : str
    scheduler which can be 'ASAP' or 'ALAP', default is 'ALAP'
arg2 : bool
    verbose, default is False
"""

%feature("docstring") Program::schedule
""" Schedules the program.

Parameters
----------
arg1 : string
    scheduler which can be 'ASAP' or 'ALAP', default is ASAP
arg2 : bool
    verbose, default is False
"""


%feature("docstring") Program::qasm
""" Returns program QASM
Parameters
----------
None

Returns
-------
str
    qasm """

%feature("docstring") Program::microcode
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
