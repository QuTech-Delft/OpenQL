
%feature("docstring") ql::api::Unitary
"""
Unitary matrix interface.

The constructor creates a unitary gate from the given row-major, square,
unitary matrix.
"""


%feature("docstring") ql::api::Unitary::name
"""
The name given to the unitary gate.
"""


%feature("docstring") ql::api::Unitary::decompose
"""
Explicitly decomposes the gate. Does not need to be called; it will be
called automatically when the gate is added to the kernel.

Parameters
----------
None

Returns
-------
None
"""


%feature("docstring") ql::api::Unitary::is_decompose_support_enabled
"""
Returns whether OpenQL was built with unitary decomposition support enabled.

Parameters
----------
None

Returns
-------
bool
    Whether OpenQL was built with unitary decomposition support enabled.
"""


%include "ql/api/unitary.h"
