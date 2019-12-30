.. _kernel:

Kernel
======

A kernel in its root form models a circuit ending in one or measurements.

In OpenQL a ``kernel`` is an object; it has a name, a type,
and a circuit as its main structural attributes.
This circuit is a vector of gates.

The type of a kernel with a non-empty circuit with gates is ``STATIC``.
During execution, all gates of such a circuit are executed from the start to the end.
After executing the last gate,
control will be transferred to the next kernel in the vector of kernels
that is an attribute of the governing program object.

Kernels of other types are used to represent control flow.
They usually have an empty circuit.
The kernel attributes ``type``, ``iterations``, and ``br_condition`` describe their meaning:

- ``type`` is ``STATIC``, the kernel's circuit is meant to be executed sequentially from start to end; after executing the last gate, control is transferred to the next kernel in the vector of kernels

- ``type`` is ``FOR_START``, the kernel sets up a loop with ``iterations`` specifying the iteration count, of which the loop body starts with the next kernel, and of which the loop body ends with the first kernel with type ``FOR_END``

- ``type`` is ``FOR_END``, the kernel takes care of control transfer to the start of the loop by decrementing the iteration counter and conditionally branching to the start of the loop body as long as the counter is not ``0``

- ``type`` is ``DO_WHILE_START``, the kernel sets up a conditional loop of do-while type, of which the loop body starts with the next kernel, and of which the loop body ends with a matching kernel with type ``DO_WHILE_END``

- ``type`` is ``DO_WHILE_END``, the kernel takes care of conditional control transfer to the start of the loop by checking the specified branch condition ``br_condition`` and conditionally branching to the start of the loop body as long as it evaluates to ``true``

- ``type`` is ``IF_START``, the kernel takes care of checking the specified branch condition ``br_condition`` and conditionally branching to a matching kernel with type ``IF_END`` when it evaluates to ``false``

- ``type`` is ``IF_END``, the kernel signals a merge of control flow from an ``IF_START`` type kernel

- ``type`` is ``ELSE_START``, the kernel takes care of checking the specified branch condition ``br_condition`` and conditionally branching to a matching kernel with type ``ELSE_END`` when it evaluates to ``true``

- ``type`` is ``ELSE_END``, the kernel signals a merge of control flow from an ``ELSE_START`` type kernel

The kernel's name functions as a label to be used in control transfers.

:Note: There aren't gates for control flow (*control gates*), only kernel attributes.

:Note: Control flow gates cannot be configured in the platform configuration file, and cannot be scheduled.

:Note: Code generation of control flow, i.e. the mapping from the internal representation to the target platform's instruction set and to QASM requires code that is at a different place than the mapping of gates; it is differently organized; it follows a different model of translation.

:Note: Scheduling around control flow, i.e. defining durations, dependences, relation to resources, is irregularly organized as well; a property of scheduling is that once scheduling of the main code has been done, all later additional scheduling must not disturb the first schedule, and thus that usually to accomplish this, more strict constraints are applied with less optimal code as result; and any attempt is error-prone as well.  It also means that the number of cycles to transfer control flow from one kernel to the next kernel is not modeled and that loop scheduling and other forms of inter-kernel scheduling are unnecessarily hard to support.

In OpenQL this kernel object also supports adding gates to its circuit using the kernel API.
To that end, a kernel object has attributes such as ``qubit_count``, and ``creg_count``
to check validity of the operands of the gates that are to be created.
And it needs to know the platform configuration file that is to be used to create custom gates;
for this, the API that creates a kernel object has the platform object as one of its parameters.
Next to this, the kernel object has a method to create each particular default gate.

See also :ref:`input_external_representation`.

Further details [TBD].
