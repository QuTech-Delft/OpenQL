
%feature("docstring") initialize
"""
Initializes the OpenQL library, for as far as this must be done. This should
ideally be called by the user (in Python) before anything else, but
set_option() and the constructors of Compiler and Platform will automatically
call this when it hasn't been done yet as well.

Currently this just resets the options to their default values to give the
user a clean slate to work with in terms of global variables (in case someone
else has used the library in the same interpreter before them, for instance,
as might happen with ipython/Jupyter in a shared notebook server, or during
test suites), but it may initialize more things in the future.

Parameters
----------
None

Returns
-------
None
"""


%feature("docstring") ensure_initialized
"""
Calls initialize() if it hasn't been called yet.

Parameters
----------
None

Returns
-------
None
"""


%feature("docstring") get_version
"""
Returns the compiler's version string.

Parameters
----------
None

Returns
-------
str
    version number as a string
"""


%feature("docstring") set_option
"""
Sets a global option for the compiler. Use print_options() to get a list of
all available options.

Parameters
----------
option : str
    Name of the option to set.
value : str
    The value to set the option to.

Returns
-------
None
"""


%feature("docstring") get_option
"""
Returns the current value for a global option. Use print_options() to get a
list of all available options.

Parameters
----------
option : str
    Name of the option to retrieve the value of.

Returns
-------
str
    The value that the option has been set to, or its default value if the
    option has not been set yet.
"""


%feature("docstring") print_options
"""
Prints a list of all available options.

Parameters
----------
None

Returns
-------
None
"""

%include "ql/api/misc.h"
