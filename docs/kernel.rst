Kernel
======

A kernel in its root form models a circuit ending in one or measurements.

In OpenQL a ``kernel`` is an object; it has a name and a circuit as its main structural attributes.
This circuit is a vector of gates.
During execution, all gates of the circuit of a kernel are executed from the first to the last.
The last gate can be a control gate that can transfer control to a specific other kernel;
if not present,
control will be transferred to the next kernel in the vector of kernels
that is an attribute of the governing program object.
The kernel's name functions as a label to be used in such control gates.

In OpenQL this kernel object also supports adding gates to its circuit using the kernel API.
To that end, a kernel object has attributes such as ``qubit_count``, and ``creg_count``
to check validity of the operands of the gates that are to be created.
And it needs to know the platform configuration file that is to be used to create custom gates;
for this, the API that creates a kernel object has the platform object as one of its parameters.
Next to this, the kernel object has a method to create each particular default gate.

See also :ref:`input_external_representation`.

Further details [TBD].
