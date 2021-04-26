
%feature("docstring") ql::api::Operation
"""
Wrapper for a classical operation.

A classical operation acts as a simple expression that returns an integer or a
boolean. The expression can be a literal number (val), a single CReg (lop,
primarily used for assignments), a unary function applied to one CReg (op/rop),
or a binary function applied to two CRegs (lop/op/rop).

Function selection is done via strings. The following unary functions are
recognized:

 - '~': bitwise NOT.

The following binary functions are recognized:

 - '+': addition.
 - '-': subtraction.
 - '&': bitwise AND.
 - '|': bitwise OR.
 - '^': bitwise XOR.
 - '==': equality.
 - '!=': inequality.
 - '>': greater-than.
 - '>=': greater-or-equal.
 - '<': less-than.
 - '<=': less-or-equal.

NOTE: classical logic is on the list to be completely revised. This interface
may change in the (near) future.
"""


%include "ql/api/operation.h"
