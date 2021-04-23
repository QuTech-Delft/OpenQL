
%feature("docstring") ql::api::cQasmReader
"""
cQASM reader interface.

To read a cQASM file, build a cQASM reader for the an already-existing program,
and then call file2circuit or string2circuit to add the kernels from the cQASM
file/string to the program. Optionally a platform can be specified as well, but
this is redundant (it must be the same platform as the one that the program was
constructed with); these overloads only exist for backward compatibility.

Because OpenQL supports custom gates and cQASM (historically) does not, and also
because OpenQL's internal representation of gates is still a bit different from
what cQASM uses, you may need custom conversion rules for the gates. This can
be done by specifying a gateset configuration JSON file using gateset_fname.
This file must consist of a JSON array containing objects with the following
structure:

.. code-block::

   {
       \"name\": \"<name>\",               # mandatory
       \"params\": \"<typespec>\",         # mandatory
       \"allow_conditional\": <bool>,    # whether conditional gates of this type are accepted, defaults to true
       \"allow_parallel\": <bool>,       # whether parallel gates of this type are accepted, defaults to true
       \"allow_reused_qubits\": <bool>,  # whether reused qubit args for this type are accepted, defaults to false
       \"ql_name\": \"<name>\",            # defaults to \"name\"
       \"ql_qubits\": [                  # list or \"all\", defaults to the \"Q\" args
           0,                          # hardcoded qubit index
           \"%0\"                        # reference to argument 0, which can be a qubitref, bitref, or int
       ],
       \"ql_cregs\": [                   # list or \"all\", defaults to the \"I\" args
           0,                          # hardcoded creg index
           \"%0\"                        # reference to argument 0, which can be an int variable reference, or int for creg index
       ],
       \"ql_bregs\": [                   # list or \"all\", defaults to the \"B\" args
           0,                          # hardcoded breg index
           \"%0\"                        # reference to argument 0, which can be an int variable reference, or int for creg index
       ],
       \"ql_duration\": 0,               # duration; int to hardcode or \"%i\" to take from param i (must be of type int), defaults to 0
       \"ql_angle\": 0.0,                # angle; float to hardcode or \"%i\" to take from param i (must be of type int or real), defaults to first arg of type real or 0.0
       \"ql_angle_type\": \"<type>\",      # interpretation of angle arg; one of \"rad\" (radians), \"deg\" (degrees), or \"pow2\" (2pi/2^k radians), defaults to \"rad\"
       \"implicit_sgmq\": <bool>,        # if multiple qubit args are present, a single-qubit gate of this type should be replicated for these qubits (instead of a single gate with many qubits)
       \"implicit_breg\": <bool>         # the breg operand(s) that implicitly belongs to the qubit operand(s) in the gate should be added to the OpenQL operand list
   }

The typespec string defines the expected argument types for the gate. Each
character in the string represents an argument. The following characters are
supported by libqasm:

 - Q = qubit
 - B = assignable bit/boolean (measurement register)
 - b = bit/boolean
 - a = axis (x, y, or z)
 - I = assignable integer
 - i = integer
 - r = real
 - c = complex
 - u = complex matrix of size 4^n, where n is the number of qubits in the
   parameter list (automatically deduced)
 - s = (quoted) string
 - j = json

Note that OpenQL only uses an argument if it is referred to in one of the
\"ql_*\" keys, either implicitly or explicitly.
"""


%feature("docstring") ql::api::cQasmReader::platform
"""
The platform associated with the reader.
"""


%feature("docstring") ql::api::cQasmReader::program
"""
The program that the cQASM circuits will be added to.
"""


%feature("docstring") ql::api::cQasmReader::string2circuit
"""
Interprets a string as cQASM file and adds its contents to the program
associated with this reader.

Parameters
----------
cqasm_str : str
    The string representing the contents of the cQASM file.

Returns
-------
None
"""


%feature("docstring") ql::api::cQasmReader::file2circuit
"""
Interprets a string as cQASM file and adds its contents to the program
associated with this reader.

Parameters
----------
cqasm_file_path : str
    The path to the cQASM file to load.

Returns
-------
None
"""


%include "ql/api/cqasm_reader.h"
