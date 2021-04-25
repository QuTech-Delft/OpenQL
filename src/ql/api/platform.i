
%feature("docstring") ql::api::Platform
"""
Quantum platform description. This describes everything that the compiler
needs to know about the target quantum chip, instruments, etc. Platforms are
created from either the default configuration for a particular architecture
variant or from JSON (+ comments) configuration files: there is no way to modify
a platform using the API, and introspection is limited. Instead, if you want to
use a custom configuration, you will need to write a JSON configuration file
for it, or use get_platform_json() and from_json() to modify an existing one
from within Python.

The syntax of the platform configuration file is too extensive to describe here.
It has its own section in the manual.

In addition to the platform itself, the Platform object provides an interface
for obtaining a Compiler object. This object describes the *strategy* for
transforming the quantum algorithm to something that can be executed on the
device described by the platform. You can think of the difference between
them as the difference between a verb and a noun: the platform describes
something that just exists, while the compilation strategy describes how to
get there.

The (initial) strategy can be set using a separate configuration file
(compiler_config), directly from within the platform configuration file,
or one can be inferred based on the previously hardcoded defaults. Unlike the
platform itself however, an extensive API is available for adjusting the
strategy as you see fit; just use get_compiler() to get a reference to a
Compiler object that may be used for this purpose. If you don't do anything
with the compiler methods and object, don't specify the compiler_config_file
parameter, and the \"eqasm_compiler\" key of the platform configuration file
refers to one of the previously-hardcoded compiler, a strategy will be
generated to mimic the old logic for backward compatibility.

Eight constructors are provided:

 - Platform(): shorthand for Platform('none', 'none').
 - Platform(name): shorthand for Platform(name, name).
 - Platform(name, platform_config): builds a platform with the given name (only
   used for log messages) and platform configuration, the latter of which can
   be either a recognized platform name with or without variant suffix (for
   example \"cc\" or \"cc_light.s7\"), or a path to a JSON configuration
   filename.
 - Platform(name, platform_config, compiler_config): as above, but specifies a
   custom compiler configuration file in addition.
 - Platform.from_json(name, platform_config_json): instead of loading the
   platform JSON data from a file, it is taken from its Python object
   representation (as per json.loads()/dumps()).
 - Platform.from_json(name, platform_config_json, compiler_config): as above,
   with compiler JSON file override.
 - Platform.from_json_string(name, platform_config_json): as from_json, but
   loads the data from a string rather than a Python object.
 - Platform.from_json_string(name, platform_config_json, compiler_config): as
   from_json, but loads the data from a string rather than a Python object.
"""


%feature("docstring") ql::api::Platform::name
"""
The user-given name of the platform.
"""


%feature("docstring") ql::api::Platform::config_file
"""
The configuration file that the platform was loaded from.
"""


%feature("docstring") ql::api::Platform::from_json_string
"""
Alternative constructor. Instead of the platform JSON data being loaded from a
file, they are loaded from the given string. See also from_json().

Parameters
----------
name : str
    The name for the platform.
platform_config_json : str
    The platform JSON configuration data as a string. This will accept anything
    that the normal constructor accepts when it reads the configuration from a
    file.
compiler_config : str
    Optional compiler configuration JSON filename. This is *NOT* JSON data.

Returns
-------
Platform
    The constructed platform.
"""


%feature("docstring") ql::api::Platform::get_platform_json_string
"""
Returns the default platform configuration data as a JSON + comments string.
The comments use double-slash syntax. Note that JSON itself does not support
such comments (or comments of any kind), so these comments need to be removed
from the data before the JSON data can be parsed.

Parameters
----------
platform_config : str
    The platform configuration. Same syntax as the platform constructor, so this
    supports architecture names, architecture variant names, or JSON filenames.
    In the latter case, this function just loads the file contents into a string
    and returns it.

Returns
-------
str
    The JSON + comments data for the given platform configuration string.
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
