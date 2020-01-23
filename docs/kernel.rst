.. _kernel:

Kernel
======

A kernel conventionally models a circuit with quantum gates ending in one or more measurements.
In OpenQL, this has been extended with:

- control flow that can jump at the end of a kernel to the start of another kernel; see this section
- classical gates mixed with quantum gates (including measurements) in a single circuit with the design objective to support control flow changes; see :ref:`classical_instructions`; measurements are just gates with classical results and can be anywhere, bridging quantum code to classical code; a kernel doesn't necessarily have to contain a measurement

In OpenQL a ``kernel`` is an object; it has a name, a type,
and a circuit as its main structural attributes.
This circuit is a vector of gates.

The type of a kernel with a non-empty circuit with gates is ``STATIC``.
During execution, all gates of such a circuit are executed from the start to the end.
After executing the last gate,
control will be transferred to the next kernel in the vector of kernels.
This vector of kernels is an attribute of the governing program object.

You saw a first kernel which was a ``STATIC`` one being created in the example program in :ref:`program`.

Kernels of other types are used to represent control flow.
This is the topic of the remainder of this section.
If you are not interested in this now, you can read this later.

Let us first look at some example Python OpenQL code (adapted from *tests/test_hybrid.py*):

.. code:: python

   num_qubits = 5
   num_cregs = 10

   p = ql.Program('test_classical', platform, num_qubits, num_cregs)
   kfirst = ql.Kernel('First', platform, num_qubits, num_cregs)

   # create classical registers
   rs1 = ql.CReg()
   rs2 = ql.CReg()

   # if (rs1 == rs2) then Thenpart else Elsepart endif
   kfirst.classical(rs1, ql.Operation(...))
   kfirst.classical(rs2, ql.Operation(...))
   kthen = ql.Kernel('Thenpart', platform, num_qubits, num_cregs)
   kthen.gate('x', [0])
   kelse = ql.Kernel('Elsepart', platform, num_qubits, num_cregs)
   kelse.gate('y', [0])
   p.add_if_else(kthen, kelse, ql.Operation(rs1, '==', rs2))

   # loop 10 times over Loopbody endloop;
   kloopbody = ql.Kernel('Loopbody', platform, num_qubits, num_cregs)
   kloopbody.gate('x', [0])
   p.add_for(kloopbody, 10)
   # Afterloop
   kafterloop = ql.Kernel('Afterloop', platform, num_qubits, num_cregs)
   kafterloop.gate('y', [0])
   p.add_kernel(kafterloop)

   # do Dowhileloopbody while (rs1 < rs2)
   kdowhileloopbody = ql.Kernel('Dowhileloopbody', platform, num_qubits, num_cregs)
   kdowhileloopbody.gate('x', [0])
   kdowhileloopbody.classical(rs1, ql.Operation(...))
   kdowhileloopbody.classical(rs2, ql.Operation(...))
   p.add_do_while(kdowhileloopbody, ql.Operation(rs1, '<', rs2))
   # Afterdowhile
   kafterdowhile = ql.Kernel('Afterdowhile', platform, num_qubits, num_cregs)
   kafterdowhile.gate('y', [0])
   p.add_kernel(kafterdowhile)

   p.compile()

These are three examples in one:

- the first creates an if-then-else construct under the condition that ``rs1`` equals ``rs2``
- the second creates a for loop with 10 iterations
- the third one creates a do-while construct executing the ``Dowhileloopbody`` as long as ``rs1`` is less than ``rs2``

We see that ordinary kernels are created and filled with a single gate; these are the ``STATIC`` kernels.
These kernels serve as the thenpart, elsepart, loopbody, etc.
And then we see three examples of the creation of a control-flow construct, with the ordinary kernels as parameters.

After this, we'll have the following 15 kernels in the ``kernels`` vector of program ``test_classical``
(some are named after their ``type``, see below):
``First``, ``IF_START``, ``Thenpart``, ``IF_END``, ``ELSE_START``, ``Elsepart``, ``ELSE_END``,
``FOR_START``, ``Loopbody``, ``FOR_END``, ``Afterloop``,
``DO_WHILE_START``, ``Dowhileloopbody``, ``DO_WHILE_END``, ``Afterdowhile``.


.. _control_flow_in_the_internal_representation:

Control flow in the internal representation
-------------------------------------------

The classical gates in :ref:`classical_instructions` deal with classical computation.
Control flow is represented in the internal representation as kernels of a special type, with their special attributes.

The relevant kernel attributes are ``type``, ``name``, ``iterations``, and ``br_condition``.
How these relate, is summarized in the next table:

+----------------+----------------------------+---------+--------------+------------+-------------------------------------+
| type           | name                       | circuit | br_condition | iterations | example OpenQL creating this kernel |
+================+============================+=========+==============+============+=====================================+
| STATIC         | label                      | gates   |              |            | p.add(ql.kernel(label, ...))        |
+----------------+----------------------------+---------+--------------+------------+-------------------------------------+
| FOR_START      | body.name+'for_start'      |         |              | loopcount  | p.add_for(body, loopcount)          |
+----------------+----------------------------+         +              +            +                                     +
| FOR_END        | body.name+'for_end'        |         |              |            |                                     |
+----------------+----------------------------+         +--------------+------------+-------------------------------------+
| DO_WHILE_START | body.name+'do_while_start' |         | loopcond     |            | p.add_do_while(body, loopcond)      |
+----------------+----------------------------+         +              +            +                                     +
| DO_WHILE_END   | body.name+'do_while'       |         |              |            |                                     |
+----------------+----------------------------+         +--------------+------------+-------------------------------------+
| IF_START       | then.name+'if'             |         | thencond     |            | p.add_if(then, thencond)            |
+----------------+----------------------------+         +              +            +                                     +
| IF_END         | then.name+'if_end'         |         |              |            |                                     |
+----------------+----------------------------+         +              +            +-------------------------------------+
| ELSE_START     | else.name+'else'           |         |              |            | p.add_if_else(then, else, thencond) |
+----------------+----------------------------+         +              +            +                                     +
| ELSE_END       | else.name+'else_end'       |         |              |            |                                     |
+----------------+----------------------------+---------+--------------+------------+-------------------------------------+

The example OpenQL in the last column shows how a kernel of the type is created.
The table also shows how the parameters of the OpenQL call creating the kernel are used to initialize the kernel's attributes.

Further information on these attributes:

- ``name`` is unique among the other names of kernels and is often used to construct a label before the first gate of the circuit;
  for non-``STATIC`` kernels it is generated in a systematic way from the name of the first kernel of the body (or then or else part)
  and from the kernel type to make it easy to generate the conditional branches to the respective label; the ``name`` column suggests a way
  but in practice this can more complicated in the presence of nested constructs (then additional counts are needed)
  or in the presence of multiple kernels (a ``program`` object) constituting the body (or then or else part)

- ``circuit`` (the real kernel attribute name is ``c`` but this is very non-descriptive) contains the gates and is empty for non-``STATIC`` kernels

- ``br_condition`` is an expression that is created by a call to an ``Operation()`` method
  (see :ref:`classical_instructions`); it represents a condition so it must be of ``RELATIONAL`` type;
  this attribute stores the condition under which the (first) body of the conditional construct is executed;
  the latter is the kernel referenced by ``then`` in case of an if or an if-else;
  and it is the kernel representing the loop's body in case of a do-while.
  ``body``, ``then``, and ``else`` all stand for references to the other kernels in the respective constructs.
  Similarly, ``loopcond``, and ``thencond`` stand for the expressions representing the condition.

``loopcount`` and ``iterations`` are of type ``size_t`` and so are non-negative and are assumed to have a value of at least 1.


The semantics of a kernel with respect to control flow is described next, separately for each kernel type:

- ``type`` is ``STATIC``:
  the kernel's circuit is meant to be executed sequentially from start to end;
  after executing the last gate, control is transferred to the next kernel in the vector of kernels

- ``type`` is ``FOR_START``:
  the kernel sets up a loop with ``iterations`` specifying the iteration count,
  of which the loop body starts with the next kernel,
  and of which the loop body ends with the first kernel with type ``FOR_END``

- ``type`` is ``FOR_END``:
  the kernel takes care of control transfer to the start of the loop by decrementing the iteration counter and conditionally branching to the start of the loop body as long as the counter is not ``0``

- ``type`` is ``DO_WHILE_START``:
  the kernel sets up a conditional loop of do-while type,
  of which the loop body starts with the next kernel,
  and of which the loop body ends with a matching kernel with type ``DO_WHILE_END``

- ``type`` is ``DO_WHILE_END``:
  the kernel takes care of conditional control transfer to the start of the loop
  by checking the specified branch condition ``br_condition``
  and conditionally branching to the start of the loop body as long as it evaluates to ``true``

- ``type`` is ``IF_START``:
  the kernel takes care of checking the specified branch condition ``br_condition``
  and conditionally branching to a matching kernel with type ``IF_END`` when it evaluates to ``false``

- ``type`` is ``IF_END``:
  the kernel signals a merge of control flow from an ``IF_START`` type kernel

- ``type`` is ``ELSE_START``:
  the kernel takes care of checking the specified branch condition ``br_condition``
  and conditionally branching to a matching kernel with type ``ELSE_END`` when it evaluates to ``true``

- ``type`` is ``ELSE_END``:
  the kernel signals a merge of control flow from an ``ELSE_START`` type kernel

The kernel's ``name`` functions as a label to be used in control transfers.

:Note: There aren't gates for control flow (*control gates*), only kernel attributes.

:Note: Control flow gates cannot be configured in the platform configuration file.

:Note: Control flow instructions/gates cannot be scheduled.

:Note: Code generation of control flow, i.e. the mapping from the internal representation to the target platform's instruction set and to QASM requires code inside the OpenQL compiler that is at a different place than the mapping of gates in the internal representation to the target platform's instruction set or QASM; that there have to be these parallel pieces of code inside the OpenQL compiler complicates the compiler unnecessarily.

:Note: Scheduling around control flow, i.e. defining durations, dependences, relation to resources, is irregularly organized as well; a property of scheduling is that once scheduling of the main code has been done, all later additional scheduling must not disturb the first schedule, and thus that usually to accomplish this, more strict constraints are applied with less optimal code as result; and any attempt is error-prone as well.  It also means that the number of cycles to transfer control flow from one kernel to the next kernel is not modeled and that loop scheduling and other forms of inter-kernel scheduling are unnecessarily hard to support.


Control flow in the output external representation
--------------------------------------------------

As explained above in :ref:`kernel`, the kernels in the ``kernels`` vector of a program by default execute
in the order of appearance in this vector, i.e. at the end of each kernel, control is transferred to the next kernel
in the vector. This holds for kernels of ``type`` ``STATIC``, the type of kernels that store the gates.

When generating control flow,
before the start and/or after the end of a kernel additional code is generated, depending on the kernel's ``type``.
The code before the start of a kernel is called ``prologue``.
The code of the kernel itself is called ``body``.
The code after the end of a kernel is called ``epilogue``.

In this, frequently a QASM conditional branch or the conditional branch with the condition inversed is generated.
The following table shows by example which conditional branch and inversed conditional branch is generated 
for a particular ``br_condition``, ``operands``, and ``target label``:

+--------------+----------+--------------+---------------------+-----------------------+
| br_condition | operands | target label | QASM cond. branch   | QASM inv. cond branch |
+==============+==========+==============+=====================+=======================+
| "eq"         | rs1, rs2 | label        | beq rs1, rs2, label | bne rs1, rs2, label   |
+--------------+          +              +---------------------+-----------------------+
| "ne"         |          |              | bne rs1, rs2, label | beq rs1, rs2, label   |
+--------------+          +              +---------------------+-----------------------+
| "lt"         |          |              | blt rs1, rs2, label | bge rs1, rs2, label   |
+--------------+          +              +---------------------+-----------------------+
| "gt"         |          |              | bgt rs1, rs2, label | ble rs1, rs2, label   |
+--------------+          +              +---------------------+-----------------------+
| "le"         |          |              | ble rs1, rs2, label | bgt rs1, rs2, label   |
+--------------+          +              +---------------------+-----------------------+
| "ge"         |          |              | bge rs1, rs2, label | blt rs1, rs2, label   |
+--------------+----------+--------------+---------------------+-----------------------+

The following is generated for a QASM prologue:

- the ``name`` of the kernel as label
- in case of ``IF_START``: an inverse conditional branch for the given ``br_condition`` over the ``then`` part to the corresponding IF_END kernel
- in case of ``ELSE_START``: a conditional branch for the given ``br_condition`` over the ``else`` part to the corresponding ELSE_END kernel
- in case of ``FOR_START``: the initialization using ``ldi``s of r29, r30 and r31 with ``iterations``, 1 and 0, respectively, in which r30 is the increment, and r31 the loop counter

The following is generated for a QASM epilogue:

- the ``name`` of the kernel as label
- in case of ``DO_WHILE_END``: a conditional branch for the given ``br_condition`` back over the ``body`` part to the corresponding ``DO_WHILE_START`` kernel 
- in case of ``FOR_END``: an "add" to r31 of r30 (which increments the loop counter by 1), and a conditional branch as long as r31 is less than r29, the number of iterations, to the loop body

API
---

In OpenQL this kernel object also supports adding gates to its circuit using the kernel API.
To that end, a kernel object has attributes such as ``qubit_count``, and ``creg_count``
to check validity of the operands of the gates that are to be created.
And it needs to know the platform configuration file that is to be used to create custom gates;
for this, the API that creates a kernel object has the platform object as one of its parameters.
Next to this, the kernel object has a method to create each particular default gate.

[TBD]
