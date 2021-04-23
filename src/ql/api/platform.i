
%feature("docstring") ql::api::Platform
"""
Quantum platform description. This describes everything that the compiler
needs to know about the target quantum chip, instruments, etc. Platforms are
created from either the default configuration for a particular architecture
or from JSON (+ comments) configuration files: there is no way to modify a
platform using the API, and introspection is limited. The syntax of the
platform configuration file is too extensive to describe here, and has its own
section in the manual.

In order to use a default configuration for a particular architecture, simply
set the platform_config_file parameter to the namespace of said architecture,
instead of to a JSON filename. The default configuration file for a platform
is listed in the documentation for that platform.

In addition to the platform itself, the Platform object provides an interface
for obtaining a Compiler object. This object describes the *strategy* for
transforming the quantum algorithm to something that can be executed on the
device described by the platform. You can think of the difference between
them as the difference between a verb and a noun: the platform describes
something that just exists, while the compilation strategy describes how to
get there.

The (initial) strategy can be set using a separate configuration file
(compiler_config_file), directly from within the platform configuration file,
or one can be inferred based on the previously hardcoded defaults. Unlike the
platform itself however, an extensive API is available for adjusting the
strategy as you see fit; just use get_compiler() to get a reference to a
Compiler object that may be used for this purpose. If you don't do anything
with the compiler methods and object, don't specify the compiler_config_file
parameter, and the \"eqasm_compiler\" key of the platform configuration file
refers to one of the previously-hardcoded compiler, a strategy will be
generated to mimic the old logic for backward compatibility.
"""


%feature("docstring") ql::api::Platform::name
"""
The user-given name of the platform.
"""


%feature("docstring") ql::api::Platform::config_file
"""
The configuration file that the platform was loaded from.
"""


%feature("docstring") ql::api::Platform::get_qubit_number
"""
Returns the number of qubits in the platform.

Parameters
----------
None

Returns
-------
int
    The number of qubits in the platform.
"""


%feature("docstring") ql::api::Platform::print_info
"""
Prints some basic information about the platform.

Parameters
----------
None

Returns
-------
None
"""


%feature("docstring") ql::api::Platform::get_info
"""
Returns the result of print_info() as a string.

Parameters
----------
None

Returns
-------
str
    The result of print_info() as a string.
"""


%feature("docstring") ql::api::Platform::has_compiler
"""
Returns whether a custom compiler configuration has been attached to this
platform. When this is the case, programs constructed from this platform
will use it to implement Program.compile(), rather than generating the
compiler in-place from defaults and global options during the call.

Parameters
----------
None

Returns
-------
bool
    Whether a custom compiler configuration has been attached to this platform.
"""


%feature("docstring") ql::api::Platform::get_compiler
"""
Returns the custom compiler configuration associated with this platform.
If no such configuration exists yet, the default one is created,
attached, and returned.

Parameters
----------
None

Returns
-------
Compiler
    A Compiler object that may be used to introspect or modify the compilation
    strategy associated with this platform.
"""


%feature("docstring") ql::api::Platform::set_compiler
"""
Sets the compiler associated with this platform. Any programs constructed
from this platform after this call will use the given compiler.

Parameters
----------
compiler : Compiler
    The new compiler configuration.

Returns
-------
None
"""


%include "ql/api/platform.h"
