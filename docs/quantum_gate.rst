.. _quantum_gates:

Quantum Gates
=============

Gates in OpenQL are the constructs that refer to operations to be executed somehow on the computing platform.

A gate refers to an operation and to zero or more operands.

Gates are organized in circuits as vectors of gates, i.e. linear sequences of gates.
A circuit defines the operation of a kernel.
And a program consists of multiple kernels.

Gates can be subdivided in several kinds.
This is useful in the description of the passes below.


First, gates can be subdivided according to where their execution has effect:

- quantum gates; these gates execute in the quantum computing hardware; these gates have at least one qubit as (implicit or explicit) operand; these gates can have classical registers as operand as well and may rely on some execution capability in classical hardware

- classical gates; these gates execute in classical hardware only; these don't have any qubit as operand, only zero or more classical registers

- directives; these gates execute neither in quantum nor in classical hardware; these look like gates but don't influence execution, e.g. the display gate


Quantum gates can also be subdivided seen from the state of a qubit:

- preparation gates; (usually one-qubit) gates taking qubits in an undefined state and bringing them in a particular defined state

- rotation gates; gates that perform unitary rotations on the state of the operand qubits;
  examples are identity, x, rx(pi), cnot, swap, and toffoli.

- measurement gates; gates that measure out the operand qubits, leaving them in a base state; the measurement result can be left in a classical register

- scheduling gates; gates that only influence execution timing regarding the operand qubits;
  they provide a cycle window for the qubit state to be operated upon before further use;
  examples are the wait and barrier gates


Quantum gates can further be subdivided from the number of operands they take; this becomes relevant when gates are mapped on a quantum computing platform that only supports two-qubit rotation gates when the operand qubits are physically adjacent, as is the case in CC-Light:

- one-qubit gates; quantum gates operating on one qubit

- two-qubit gates; quantum gates operating on two qubits;
  e.g. two-qubit rotation gates are the main objective in the current mapping pass since these gates require their qubit operands to be connected in CC-Light

- multi-qubit gates; quantum gates operating (implicitly or explicitly) on more than two qubits;
  e.g. multi-qubit rotation gates must be decomposed to one-qubit and two-qubit gates because more-qubit primitive rotation gates are not supported by CC-Light


Particular classes of quantum gates can be further recognized; these definitions are given
mainly to refer to from other chapters of this documentation, especially from the compiler passes chapter and the quantum gate chapter:

- primitive gates; quantum gates natively supported by instructions of the quantum computing platform

- pauli gates; the Identity, X, Y and Z rotation gates

- clifford gates;
  the one-qubit clifford gates form a group of 24 elements / equivalence classes
  each composed from a sequence of one or more rotations by a multiple of 90 degrees in one dimension (X, Y or Z)

- default gates; quantum gates predefined by OpenQL

- custom gates; quantum gates defined by the platform configuration file

- composite gates; custom gates that are decomposed to their component gates when created

- specialized gates; custom gates with a definition in the configuration file
  that is specific for the particular qubit operands that are specified in it;
  the semantic attributes of several specialized gates
  with the same quantum operation but different qubit operands may differ
  (in-line with the purpose of a gate being specialized)

- parameterized gates; custom gates that are not specialized,
  i.e. with a definition that is not specific for particular qubit operands;
  all gates created (usually for different qubits) from the same parameterized gate in the platform configuration file
  have the same semantic attributes



.. _quantum_gate_attributes_in_the_internal_representation:

Quantum gate attributes in the internal representation
------------------------------------------------------

Quantum gate attributes can be subdivided in the following kinds:

- structural attribute;
  these attributes define the gate, and are mandatory;
  key examples are operation name and operands.
  These attributes are taken from the OpenQL program or the QASM external representation of a gate.
  These never change after creation and usually are identical over multiple compilations.

- semantic attribute; these attributes define more of the semantics of the gate, usually for a specific purpose;
  their value fully depends on and is derived from the gate's structural attributes.
  In OpenQL they are defined in the configuration file.
  Furthermore, these attributes usually don't change during compilation,
  although that would be possible when done in a consistent way over all gates.
  The latter is consistent with changing the configuration file with respect to the values of the semantic attributes.

- result attribute; the value of these attributes is computed during compilation.
  Usually there is a choice from various strategies and platform parameters how to compute these
  and so result attributes are seen as an independent kind of attributes.
  A key example is the cycle attribute as computed by the scheduler.
  At the start of compilation, their value is undefined.

Below the OpenQL gate attributes are summarized in a table together with their key characteristics.

+---------------+-----------+-----------------+------------+------------+----------------+
| Attribute     | kind      | example         | used by    | updated by | C++ type       |
+===============+===========+=================+============+============+================+
| name          | structural| "CZ q0,q1"      | all passes | never      | string         |
+---------------+           +-----------------+            +            +----------------+
| operands      |           | [q0,q1]         |            |            | vector<size_t> |
+---------------+           +-----------------+            +            +----------------+
| creg_operands |           | [r23]           |            |            | vector<size_t> |
+---------------+           +-----------------+            +            +----------------+
| angle         |           | numpy.pi        |            |            | double         |
+---------------+           +-----------------+            +            +----------------+
| type          |           | __t_gate__      |            |            | gate_type_t    |
+---------------+-----------+-----------------+------------+            +----------------+
| duration      | semantic  | 80              | schedulers,|            | size_t         |
|               |           |                 | etc.       |            |                |
+---------------+           +-----------------+------------+            +----------------+
| mat           |           |                 | optimizer  |            | cmat_t         |
|               |           |                 | pass       |            |                |
+---------------+-----------+-----------------+------------+------------+----------------+
| cycle         | result    | 4               | code       | scheduler  | size_t         |
|               |           |                 | generation |            |                |
+---------------+-----------+-----------------+------------+------------+----------------+

Custom gates have an additional set of attributes,
primarily supporting the initialization of the gate attributes from configuration file parameters.

Some further notes on the gate attributes:

- the name of a gate includes the string representations of its qubit operands in case of a specialized gate;
  so in general, when given a name, one has to take care to isolate the operation from it;
  one may assume that the operation is a single identifier optionally followed by white space and the operands

- gates are most directly distinguished by their name

:Note: Distinguishing gates internally in the compiler by their name is problematic; distinguishing by their type (see the table below) would be better; the latter conveys the semantic definition and is independent of the representation (e.g. ``mrx90``, ``mx90``, and ``Rmx90`` all could be names of a -90 degrees X rotation); furthermore, a name is something of the external representation and is mapped to the internal representation using the platform configuration file; however, the enumeration type of type can never include all possible gates (e.g. those with arbitrary angles, any number of operands, etc.) so the type inevitably can be imprecise; but it can be precise when the type refers to the operation only, i.e. excluding the operands

- qubit and classical operands are represented by unsigned valued indices starting from 0 in their respective register spaces

- ``angle`` is in radians; it specifies the value of the arbitrary angle of those operations that need one; it is initialized only from an explicit specification as parameter value of a gate creation API

- ``duration`` is in nanoseconds, just as the timing specifications in the platform configuration file; scheduling-like passes divide it (rounding up) by the cycle_time to compute the number of cycles that an operation takes; it is initialized implicitly when the gate is a default gate or a custom gate, or explicitly from a parameter value of a gate creation API

- ``mat`` is of a two-dimensional complex double valued matrix type with dimensions equal to twice the number of operands; it is only used by the optimizer pass; it is initialized implicitly when the gate is a default gate or a custom gate

- ``cycle`` is in units of cycle_time as defined in the platform;
  the undefined value is ``std::numeric_limits<int>::max()`` also known as ``INT_MAX``.
  A gate's cycle attribute gets defined by applying a scheduler or a mapper pass,
  and remains defined until any pass is done that invalidates the cycle attribute.
  As long as the gate's cycle attribute is defined (and until it is invalidated),
  the gates must be ordered in the circuit in non-decreasing cycle order.
  Also, there is then a derived internal circuit representation, the bundled representation.
  See :ref:`circuits_and_bundles_in_the_internal_representation`.
  The cycle attribute invalidation generally is the result of adding a gate to a circuit,
  or any optimization or decomposition pass.

- type is an enumeration type; the following table enumerates the possible types and their characteristics:

+---------------------+----------------------------+------------------------+--------------+
| type                | operands                   | example in QASM        | kind         |
+=====================+============================+========================+==============+
| __identity_gate__   | 1 qubit                    | i q[0]                 | rotation     |
+---------------------+                            +------------------------+              +
| __hadamard_gate__   |                            | h q[0]                 |              |
+---------------------+                            +------------------------+              +
| __pauli_x_gate__    |                            | x q[0]                 |              |
+---------------------+                            +------------------------+              +
| __pauli_y_gate__    |                            | y q[0]                 |              |
+---------------------+                            +------------------------+              +
| __pauli_z_gate__    |                            | z q[0]                 |              |
+---------------------+                            +------------------------+              +
| __phase_gate__      |                            | s q[0]                 |              |
+---------------------+                            +------------------------+              +
| __phasedag_gate__   |                            | sdag q[0]              |              |
+---------------------+                            +------------------------+              +
| __t_gate__          |                            | t q[0]                 |              |
+---------------------+                            +------------------------+              +
| __tdag_gate__       |                            | tdag q[0]              |              |
+---------------------+                            +------------------------+              +
| __rx90_gate__       |                            | rx90 q[0]              |              |
+---------------------+                            +------------------------+              +
| __mrx90_gate__      |                            | xm90 q[0]              |              |
+---------------------+                            +------------------------+              +
| __rx180_gate__      |                            | x q[0]                 |              |
+---------------------+                            +------------------------+              +
| __ry90_gate__       |                            | ry90 q[0]              |              |
+---------------------+                            +------------------------+              +
| __mry90_gate__      |                            | ym90 q[0]              |              |
+---------------------+                            +------------------------+              +
| __ry180_gate__      |                            | y q[0]                 |              |
+---------------------+----------------------------+------------------------+              +
| __rx_gate__         | 1 qubit, 1 angle           | rx q[0],3.14           |              |
+---------------------+                            +------------------------+              +
| __ry_gate__         |                            | ry q[0],3.14           |              |
+---------------------+                            +------------------------+              +
| __rz_gate__         |                            | rz q[0],3.14           |              |
+---------------------+----------------------------+------------------------+              +
| __cnot_gate__       | 2 qubits                   | cnot q[0],q[1]         |              |
+---------------------+                            +------------------------+              +
| __cphase_gate__     |                            | cz q[0],q[1]           |              |
+---------------------+                            +------------------------+              +
| __swap_gate__       |                            | swap q[0],q[1]         |              |
+---------------------+----------------------------+------------------------+              +
| __toffoli_gate__    | 3 qubits                   | toffoli q[0],q[1],q[2] |              |
+---------------------+----------------------------+------------------------+--------------+
| __prepz_gate__      |                            | prepz q[0]             | preparation  |
+---------------------+                            +------------------------+--------------+
| __measure_gate__    | 1 qubit                    | measure q[0]           | measurement  |
+---------------------+----------------------------+------------------------+--------------+
| __nop_gate__        | none                       | nop                    | scheduling   |
+---------------------+                            +------------------------+              +
| __dummy_gate__      |                            | sink                   |              |
+---------------------+----------------------------+------------------------+              +
| __wait_gate__       | 0 or more qubits, duration | wait 1                 |              |
+---------------------+----------------------------+------------------------+--------------+
| __display__         | 0 or more qubits           | display                | directive    |
+---------------------+                            +------------------------+              +
| __display_binary__  |                            | display_binary         |              |
+---------------------+----------------------------+------------------------+--------------+
| __classical_gate__  | 0 or more classical regs.  | add r[0],r[1]          | classical    |
+---------------------+----------------------------+------------------------+--------------+
| __custom_gate__     | defined by config file                                             |
+---------------------+                                                                    +
| __composite_gate__  |                                                                    |
+---------------------+----------------------------+------------------------+--------------+

The example column shows in the form of an example the QASM representation of the gate.
For custom gates, the QASM representation is the gate name followed by the representation of the operands,
as with the default gates.


There is an API for each of the above gate types using default gates.

Some notes on the semantics of these gates:

- the wait gate waits for all its (qubit) operands to be ready;
  then it takes a duration of the given number of cycles for each of its qubit operands to execute;
  in external representations it is usually possible to not specify operands, it then applies to all qubits of the program;
  the ``barrier`` gate is sometimes found in external representations
  but is identical to a wait with 0 duration on its operand qubits (or all when none were specified)

- the nop gate is identical to ``wait 1``, i.e. a one cycle execution duration applied to all program qubits

- dummy gates are SOURCE and SINK; these gates don't have an external representation;
  these are internal to the scheduler

- custom and composite gates are fully specified in the configuration file;
  these shouldn't have this type because it doesn't serve a purpose
  but have a type that reflects its semantics


.. _circuits_and_bundles_in_the_internal_representation:

Circuits and bundles in the internal representation
---------------------------------------------------

A circuit of one kernel is represented by a vector of gates in the internal representation,
and is a structural attribute of the kernel object.
The gates in this vector are assumed to be executed from the first to the last in the vector.

During a scheduling pass, the ``cycle`` attribute of each gate gets defined.
See its definition in :ref:`quantum_gate_attributes_in_the_internal_representation`.
The gates in the vector then are ordered in non-decreasing cycle order.

The schedulers also produce a ``bundled`` version of each circuit.
The circuit is then represented by a list of bundles
in which each bundle represents the gates that are to be started in a particular cycle.
Each bundle is structured as a list of sections and each section as a list of gates (actually gate pointers).
The gates in each section share the same operation but have different operands, obviously.
The latter prepares for code generation for a SIMD instruction set
in which a single instruction with one operation can have multiple operands.
Each bundle has two additional attributes:

- ``start_cycle`` representing the cycle in which all gates of the bundle start

- ``duration_in_cycles`` representing the maximum duration in cycles of the gates in the bundle

This internal bundles representation is used during QISA generation instead of the original circuit.


.. _input_external_representation:

Input external representation
-----------------------------

OpenQL supports as input external representation currently only the OpenQL program, written in C++ and/or Python.
This is an API-level interface based on platform, program, kernel and gate objects and their methods.
Calls to these methods transfer the external representation into the internal representation
(also called intermediate representation or IR) as sketched above:
a program (object) consisting of a vector of kernels,
each containing a single circuit,
each circuit being a vector of gates.

Gates are created using an API of the general form:

.. code::

    k.gate(name, qubit operand vector, classical operand vector, duration, angle)

in which particular operands can be empty or 0 depending on the particular kind of gate that is created.
Gate creation upon a call to this API goes through the following steps to create the internal representation:

#. the qubit and/or classical register operand indices are checked for validity,
   i.e. to be in the range of 0 to the number specified in the program creation API minus 1

#. if the configuration file contains a definition for a specialized composite gate matching it, it is taken;
   the qubit parameter substitution in the gates of the decomposition specification is done;
   each resulting gate must be available as (specialized or parameterized, and non-composite) custom gate,
   or as a default gate; the decomposition is applied and all resulting gates are created and added to the circuit

#. otherwise, if a parameterized composite gate is available, take it;
   the parameter substitution in the gates of the decomposition specification is done;
   each resulting gate must be available as (specialized or parameterized, and non-composite) custom gate,
   or as a default gate; the decomposition is applied and all resulting gates are created and added to the circuit

#. otherwise, if a specialized custom gate is available, create it with the attributes specified as parameter of the API call above

#. otherwise, if a parameterized custom gate is available, create it with the attributes specified as parameter of the API call above

#. otherwise, if a default gate (predefined internally in OpenQL) is available, create it with the attributes specified as parameter of the API call above

#. otherwise, it is an error




.. _output_external_representation:

Output external representation
------------------------------

There are two closely related output external representations supported, both dialects of QASM 1.0:

- sequential QASM

- bundled QASM

In both representations,
the QASM representation of a single gate is as defined in the *example in QASM* column in the table above.

When the gate's cycle attribute is still undefined,
the sequential QASM representation is the only possible external QASM representation.
Gates are specified one by one, each on a separate line.
A gate meant to execute after another gate should appear on a later line than the latter gate,
i.e. the gates are topologically sorted with respect to their intended execution order.
Kernels start with a label which names the kernel and serves as branch target in control transfers.

Once the gate's cycle attribute has been defined (and until it is invalidated),
and in addition to the sequential QASM representation above (that ignores the cycle attribute values),
the bundled QASM representation can be generated that instead reflects the cycle attribute values.

Each line in the bundled QASM representation
represents the gates that start execution in one particular cycle
in a curly bracketed list with vertical bar separators.
Each subsequent line represents a subsequent cycle.
When there isn't a gate that starts execution in a particular cycle,
a wait gate is specified instead with as integral argument the number of cycles to wait.
As with the sequential QASM representation,
kernels start with a label which names the kernel and serves as branch target in control transfers.
