/**
 * @file   openql.i
 * @author Imran Ashraf
 * @brief  swig interface file
 */
%define DOCSTRING
"`OpenQL` is a C++/Python framework for high-level quantum programming. The framework provides a compiler for compiling and optimizing quantum code. The compiler produces the intermediate quantum assembly language in cQASM (Common QASM) and the compiled eQASM (executable QASM) for various target platforms. While the eQASM is platform-specific, the quantum assembly code (QASM) is hardware-agnostic and can be simulated on the QX simulator."
%enddef

%module(docstring=DOCSTRING) openql

%include "std_vector.i"
%include "exception.i"
%include "std_string.i"
%include "std_complex.i"


namespace std {
   %template(vectori) vector<int>;
   %template(vectorui) vector<size_t>;
   %template(vectorf) vector<float>;
   %template(vectord) vector<double>;
   %template(vectorc) vector<std::complex<double>>;
};

%{
#include "openql_i.h"
%}

/*
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
*/


%exception {
    try {
        $action
        if (PyErr_Occurred()) SWIG_fail;
    } catch (ql::utils::Exception &e) {
        SWIG_exception(SWIG_RuntimeError, e.what());
    }
    SWIG_CATCH_STDEXCEPT
    catch (...) {
        SWIG_exception(SWIG_UnknownError, "Unknown C++ exception");
    }
}



%feature("docstring") get_version
""" Returns OpenQL version

Parameters
----------
None

Returns
-------
str
    version number as a string
"""


%feature("docstring") set_option
""" Sets any of the following OpenQL options:

===================  ============= ==================================================
Opt. Name               Defaults                       Possible values
===================  ============= ==================================================
log_level             LOG_NOTHING   LOG_{NOTHING/CRITICAL/ERROR/WARNING/INFO/DEBUG}
output_dir            test_output   <output directory>
optimize              no            yes/no
use_default_gates     yes           yes/no
decompose_toffoli     no            yes/no
scheduler             ASAP          ASAP/ALAP
scheduler_uniform     no            yes/no
scheduler_commute     no            yes/no
scheduler_post179     yes           yes/no
cz_mode               manual        auto/manual
===================  ============= ==================================================


Parameters
----------
arg1 : str
    Option name
arg2 : str
    Option value
"""

%feature("docstring") get_option
""" Returns value of any of the following OpenQL options:

===================  ============= ==================================================
Opt. Name               Defaults                       Possible values
===================  ============= ==================================================
log_level             LOG_NOTHING   LOG_{NOTHING/CRITICAL/ERROR/WARNING/INFO/DEBUG}
output_dir            test_output   <output directory>
optimize              no            yes/no
use_default_gates     yes           yes/no
decompose_toffoli     no            yes/no
scheduler             ASAP          ASAP/ALAP
scheduler_uniform     no            yes/no
scheduler_commute     no            yes/no
scheduler_post179     yes           yes/no
cz_mode               manual        auto/manual
===================  ============= ==================================================

Parameters
----------
arg1 : str
    Option name

Returns
-------
str
    Option value
"""

%feature("docstring") print_options
""" Prints a list of available OpenQL options with their values.

"""



%feature("docstring") Platform
""" Platform class specifying the target platform to be used for compilation."""


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


%feature("docstring") CReg
""" Classical register class."""


%feature("docstring") CReg::CReg
""" Constructs a classical register which can be source/destination for classical operations.

Parameters
----------
None

Returns
-------
CReg
    classical register object

"""






%feature("docstring") Operation
""" Operation class representing classical operations."""

%feature("docstring") Operation::Operation
""" Constructs an Operation object (Binary operation).

Parameters
----------
arg1 : CReg
    left hand side operand
arg2 : str
    classical binary operation (+, -, &, |, ^, ==, !=, <, >, <=, >=)
arg3 : CReg
    right hand side operand
"""

%feature("docstring") Operation::Operation
""" Constructs an Operation object (Unary operation).

Parameters
----------
arg1 : str
    classical unary operation (~)
arg2 : CReg
    right hand side operand
"""

%feature("docstring") Operation::Operation
""" Constructs an Operation object (used for assignment).

Parameters
----------
arg1 : CReg
    operand
"""


%feature("docstring") Operation::Operation
""" Constructs an Operation object (used for initializing with immediate values).

Parameters
----------
arg1 : int
    immediate value
"""



%feature("docstring") Unitary
""" Unitary class to hold the matrix and its decomposition"""


%feature("docstring") Unitary::Unitary
""" Constructs a unitary

Parameters
----------
arg1 : str
    name of the unitary
arg2 : matrix
    complex unitary matrix

Returns
-------
None
"""

%feature("docstring") Unitary::decompose
""" Decomposes the unitary matrix

Parameters
----------
None

Returns
-------
None
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
arg3 : int
    qubit count
arg4 : int
    classical register count
arg4 : int
    bit register count
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

The ids and the corresponding operations are:

====  ==========================
 id    Operations
====  ==========================
0       ['I']
1       ['Y90', 'X90']
2       ['mX90', 'mY90']
3       ['X180']
4       ['mY90', 'mX90']
5       ['X90', 'mY90']
6       ['Y180']
7       ['mY90', 'X90']
8       ['X90', 'Y90']
9       ['X180', 'Y180']
10      ['Y90', 'mX90']
11      ['mX90', 'Y90']
12      ['Y90', 'X180']
13      ['mX90']
14      ['X90', 'mY90', 'mX90']
15      ['mY90']
16      ['X90']
17      ['X90', 'Y90', 'X90']
18      ['mY90', 'X180']
19      ['X90', 'Y180']
20      ['X90', 'mY90', 'X90']
21      ['Y90']
22      ['mX90', 'Y180']
23      ['X90', 'Y90', 'mX90']
====  ==========================

Parameters
----------
arg1 : int
    clifford operation id
arg2 : int
    target qubit
"""

%feature("docstring") Kernel::wait
""" inserts explicit wait of specified duration on specified qubits.

    wait with duration '0' is equivalent to barrier on specified list of qubits.
    If no qubits are specified, then wait/barrier is applied on all the qubits.


Parameters
----------
arg1 : []
    list of qubits
arg2 : int
    duration in ns
"""

%feature("docstring") Kernel::barrier
""" inserts explicit barrier on specified qubits.

    wait with duration '0' is also equivalent to applying barrier on specified list of qubits.
    If no qubits are specified, then barrier is applied on all the qubits.

Parameters
----------
arg1 : []
    list of qubits
"""


%feature("docstring") Kernel::display
""" inserts QX display instruction (so QX specific).

Parameters
----------
None

Returns
-------
None
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
    duration in ns (default: 0)
arg4 : double
    angle of rotation, used internally only for rotations (rx, ry and rz) (default: 0.0)
arg5 : []
    list of bit registers (default: [])
arg6 : str
    condition (default: 'COND_ALWAYS')
arg7 : []
    list of condition registers (default: [])
"""

%feature("docstring") Kernel::gate
""" adds custom/default gates to kernel.

Parameters
----------
arg1 : str
    name of gate
arg2 : []
    list of qubits
arg3 : CReg
    classical destination register for measure operation.
"""


%feature("docstring") Kernel::condgate
""" adds conditional gates to kernel.

Parameters
----------
arg1 : str
    name of gate
arg2 : []
    list of qubits
arg6 : str
    condition (default: 'COND_ALWAYS')
arg7 : []
    list of condition registers (default: [])
"""



%feature("docstring") Kernel::gate
""" adds unitary to kernel.

Parameters
----------
arg1 : Unitary
    unitary matrix
arg2 : []
    list of qubits
"""



%feature("docstring") Kernel::classical
""" adds classical operation kernel.

Parameters
----------
arg1 : str
    name of operation (used for inserting nop)
"""


%feature("docstring") Kernel::classical
""" adds classical operation kernel.

Parameters
----------
arg1 : CReg
    destination register for classical operation.
arg2 : Operation
    classical operation.
"""


%feature("docstring") Kernel::controlled
""" generates controlled version of the kernel from the input kernel.

Parameters
----------

arg1 : ql::Kernel
    input kernel. Except measure, Kernel to be controlled may contain any of the default gates as well custom gates which are not specialized for a specific qubits.

arg2 : []
    list of control qubits.

arg3 : []
    list of ancilla qubits. Number of ancilla qubits should be equal to number of control qubits.


Returns
-------
None
"""

%feature("docstring") Kernel::conjugate
""" generates conjugate version of the kernel from the input kernel.

Parameters
----------

arg1 : ql::Kernel
    input kernel. Except measure, Kernel to be conjugated.


Returns
-------
None
"""



%feature("docstring") Program
""" Program class which contains one or more kernels."""


%feature("docstring") Program::Program
""" Constructs a program object.

Parameters
----------
arg1 : str
    name of the program
arg2 : Platform
    instance of an OpenQL Platform
arg3 : int
    number of qubits the program will use
arg4 : int
    number of classical registers the program will use (default: 0)
arg4 : int
    number of bit registers the program will use (default: 0)
"""


%feature("docstring") Program::set_sweep_points
""" Sets sweep points for an experiment.

Parameters
----------
arg1 : []
    list of sweep points
"""


%feature("docstring") Program::get_sweep_points
""" Returns sweep points for an experiment.

Parameters
----------
None

Returns
-------
[]
    list of sweep points """



%feature("docstring") Program::add_kernel
""" Adds specified kernel to program.

Parameters
----------
arg1 : kernel
    kernel to be added
"""


%feature("docstring") Program::add_if
""" Adds specified kernel to a program which will be executed if specified condition is true.

Parameters
----------
arg1 : kernel
    kernel to be executed
arg2: Operation
    classical relational operation (<, >, <=, >=, ==, !=)
"""

%feature("docstring") Program::add_if
""" Adds specified sub-program to a program which will be executed if specified condition is true. This allows nesting of operations.

Parameters
----------
arg1 : Program
    program to be executed
arg2: Operation
    classical relational operation (<, >, <=, >=, ==, !=)
"""

%feature("docstring") Program::add_if_else
""" Adds specified kernels to a program. First kernel will be executed if specified condition is true. Second kernel will be executed if specified condition is false.

Parameters
----------
arg1 : kernel
    kernel to be executed when specified condition is true (if part).
arg2 : kernel
    kernel to be executed when specified condition is false (else part).
arg3: Operation
    classical relational operation (<, >, <=, >=, ==, !=)
"""

%feature("docstring") Program::add_if_else
""" Adds specified sub-programs to a program. First sub-program will be executed if specified condition is true. Second sub-program will be executed if specified condition is false.

Parameters
----------
arg1 : Program
    program to be executed when specified condition is true (if part).
arg2 : Program
    program to be executed when specified condition is false (else part).
arg3: Operation
    classical relational operation (<, >, <=, >=, ==, !=)
"""


%feature("docstring") Program::add_do_while
""" Adds specified kernel to a program which will be repeatedly executed while specified condition is true.

Parameters
----------
arg1 : kernel
    kernel to be executed
arg2: Operation
    classical relational operation (<, >, <=, >=, ==, !=)
"""

%feature("docstring") Program::add_do_while
""" Adds specified sub-program to a program which will be repeatedly executed while specified condition is true.

Parameters
----------
arg1 : Program
    program to be executed repeatedly
arg2: Operation
    classical relational operation (<, >, <=, >=, ==, !=)
"""


%feature("docstring") Program::add_for
""" Adds specified kernel to a program which will be executed for specified iterations.

Parameters
----------
arg1 : Kernel
    program to be executed repeatedly
arg2: int
    iteration count
"""

%feature("docstring") Program::add_for
""" Adds specified sub-program to a program which will be executed for specified iterations.

Parameters
----------
arg1 : Program
    sub-program to be executed repeatedly
arg2: int
    iteration count
"""


%feature("docstring") Program::compile
""" Compiles the program

Parameters
----------
None
"""


%feature("docstring") Program::qasm
""" Generates and returns program QASM

Parameters
----------
None

Returns
-------
str
    qasm
"""


%feature("docstring") Program::microcode
""" Returns program microcode

Parameters
----------
None

Returns
-------
str
    microcode
"""

%feature("docstring") cQasmReader
""" cQasmReader class specifies an interface to add cqasm programs to a program."""

%feature("docstring") cQasmReader::~cQasmReader
""" Destructs a cQasmReader object. """

%feature("docstring") cQasmReader::cQasmReader
""" Constructs a cQasmReader object.

Parameters
----------
arg1 : platform
    Instance of an OpenQL Platform.
arg2 : program
    Program class to which the kernels with cqasm quantum instructions are added.
"""

%feature("docstring") cQasmReader::string2circuit
""" Adds a cqasm program defined in a string.

Parameters
----------
arg1 : str
    The cqasm that is added to the program.
"""

%feature("docstring") cQasmReader::file2circuit
""" Adds a cqasm program read from a file.

Parameters
----------
arg1 : str
    File path to the file specifying the cqasm that is added to the program.
"""

%feature("docstring") Compiler
""" Compiler class which contains one or more compiler passes."""

%feature("docstring") Compiler::Compiler
""" Constructs a compiler object.

Parameters
----------
arg1 : str
    name of the compiler
"""

%feature("docstring") Compiler::compile
""" Compiles the program

Parameters
----------
arg1 : Program
    program object to be compiled.
"""

%feature("docstring") Compiler::add_pass_alias
""" Adds a compiler pass under an alias name

Parameters
----------
arg1 : str
    name of the real pass to be added.
arg2 : str
    alias name of the pass to be added. 
"""

%feature("docstring") Compiler::add_pass
""" Adds a compiler pass under its real name

Parameters
----------
arg1 : str
    name of the real pass to be added.
"""

%feature("docstring") Compiler::set_pass_option
""" Sets a compiler pass option

Parameters
----------
arg1 : str
    name (real or alias) of the compiler pass to be added.
arg2 : str
    option name of the option to be configured. 
arg3 : str
    value of the option. 
"""

// Include the header file with above prototypes
%include "openql_i.h"
