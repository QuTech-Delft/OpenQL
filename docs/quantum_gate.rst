Quantum Gates
=============

Gates in OpenQL are the constructs that refer to operations to be executed somehow on the quantum computing platform.

A gate refers to an operation and to zero or more operands.

Gates are organized in circuits as vectors of gates, i.e. linear sequences of gates.
A circuit defines the operation of a kernel.
And a program consists of multiple kernels.

Gates can be subdivided in several kinds.
This is useful in the description of the passes below.


First, gates can be subdivided according to where their execution has effect:

- quantum gates; these have at least one qubit as (implicit or explicit) operand and execute in the quantum computing hardware

- classical gates; these don't have any qubit as operand, only zero or more classical registers and execute in classical hardware

- directives; these look like gates but don't influence execution, e.g. the display gate; these execute neither in quantum nor in classical hardware


Quantum gates can be subdivided in several ways from the number of operands they take; this becomes relevant when gates are mapped on the quantum computing platform:

- one-qubit gates; quantum gates operating on one qubit

- two-qubit gates; quantum gates operating on two qubits;
  these are the main objective in mapping since two-qubit gates require their qubit operands to be connected in the hardware

- multi-qubit gates; quantum gates operating (implicitly or explicitly) on more than two qubits;
  these must be decomposed to one-qubit and two-qubit gates because more-qubit primitive gates are not supported by the quantum platform


Quantum gates can also be subdivided seen from the state of a qubit:

- preparation gates; (usually one-qubit) gates taking qubits in an undefined state and bringing them in a particular defined state

- rotation gates; gates that perform unitary rotations on the state of the operand qubits;
  examples are identity, x, rx(pi), cnot, swap, and toffoli.

- measurement gates; gates that measure out the operand qubits, leaving them in a base state

- scheduling gates; gates that only influence execution timing regarding the operand qubits;
  they provide a cycle window for the qubit state to be operated upon before further use;
  examples are the wait and barrier gates


Particular classes of quantum gates can be further recognized:

- primitive gates; quantum gates natively supported by instructions of the quantum computing platform

- pauli gates; the Identity, X, Y and Z rotation gates

- clifford gates;
  a group of quantum gates (of 24 elements / equivalence classes)
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


Classical gates can be subdivided in:

- simple gates; a classical gate that cannot change control flow; these are almost all classical gates

- control gates; a classical gate that may change control flow;
  examples are branch, stop and conditional branch instructions



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

+---------------+-----------+-----------------+------------------+------------+---------------------+
| Attribute     | kind      | example         | used by          | updated by | C++ type            |
+===============+===========+=================+==================+============+=====================+
| name          | structural| "CZ q0,q1"      | all passes       | never      | std::string         |
+---------------+           +-----------------+                  +            +---------------------+
| operands      |           | [q0,q1]         |                  |            | std::vector<size_t> |
+---------------+           +-----------------+                  +            +---------------------+
| creg_operands |           | [r23]           |                  |            | std::vector<size_t> |
+---------------+           +-----------------+                  +            +---------------------+
| angle         |           | 3.14152         |                  |            | double              |
+---------------+           +-----------------+                  +            +---------------------+
| type          |           | __cphase_gate__ |                  |            | gate_type_t         |
+---------------+-----------+-----------------+------------------+            +---------------------+
| duration      | semantic  | 80              | schedulers, etc. |            | size_t              |
+---------------+           +-----------------+------------------+            +---------------------+
| mat           |           |                 | optimizer pass   |            | cmat_t              |
+---------------+-----------+-----------------+------------------+------------+---------------------+
| cycle         | result    | 4               | code generation  | scheduler  | size_t              |
+---------------+-----------+-----------------+------------------+------------+---------------------+

Custom gates have an additional set of attributes,
primarily supporting the initialization of the gate attributes from configuration file parameters.

Some further notes on the gate attributes:

- the name of a gate includes the string representations of their operands in case of a specialized gate;
  so in general, when given a name, one has to take care to isolate the operation from it;
  one may assume that the operation is a single identifier optionally followed by white space and the operands

- gates are most directly distinguished by their name although distinguishing by their type would be better;
  the latter conveys the semantic definition and is independent of the representation
  (e.g. "mrx90", "mx90", "Rmx90" all could be names of a -90 degrees X rotation);
  but the enumeration type of type can never include all possible gates
  (e.g. those with arbitrary angles, any number of operands, etc.)

- qubit and classical operands are represented by unsigned valued indices starting from 0 in their respective register spaces;
  angle is in radians;
  duration is in nanoseconds;
  the matrix is a two-dimensional complex double valued matrix with dimensions equal to twice the number of operands;

- cycle is in units of cycle_time as defined in the platform; the undefined value is std::numeric_limits<int>::max().
  A gate's cycle attribute gets defined by applying a scheduler or a mapper pass,
  and remains defined until any pass is done that invalidates the cycle attribute.
  As long as the gate's cycle attribute is defined (and until it is invalidated),
  the gates must be ordered in the circuit in non-decreasing cycle order.
  Also, there is then a derived internal circuit representation, the bundles representation, stored in a kernel's attribute.
  This internal bundles representation is used during QISA generation instead of the original circuit.
  The cycle attribute invalidation generally is the result of gate creation, or any optimization or decomposition pass.

- type is an enumeration type; the following table enumerates the possible types and their characteristics:

+---------------------+----------------------------+--------------+
| type                | operands                   | kind         |
+=====================+============================+==============+
| __identity_gate__   | 1 qubit                    | rotation     |
+---------------------+----------------------------+--------------+
| __hadamard_gate__   | 1 qubit                    | rotation     |
+---------------------+----------------------------+--------------+
| __pauli_x_gate__    | 1 qubit                    | rotation     |
+---------------------+----------------------------+--------------+
| __pauli_y_gate__    | 1 qubit                    | rotation     |
+---------------------+----------------------------+--------------+
| __pauli_z_gate__    | 1 qubit                    | rotation     |
+---------------------+----------------------------+--------------+
| __phase_gate__      | 1 qubit                    | rotation     |
+---------------------+----------------------------+--------------+
| __phasedag_gate__   | 1 qubit                    | rotation     |
+---------------------+----------------------------+--------------+
| __t_gate__          | 1 qubit                    | rotation     |
+---------------------+----------------------------+--------------+
| __tdag_gate__       | 1 qubit                    | rotation     |
+---------------------+----------------------------+--------------+
| __rx90_gate__       | 1 qubit                    | rotation     |
+---------------------+----------------------------+--------------+
| __mrx90_gate__      | 1 qubit                    | rotation     |
+---------------------+----------------------------+--------------+
| __rx1 qubite__      | 1 qubit                    | rotation     |
| __ry90_gate__       | 1 qubit                    | rotation     |
| __mry90_gate__      | 1 qubit                    | rotation     |
| __ry180_gate__      | 1 qubit                    | rotation     |
+---------------------+----------------------------+--------------+
| __rx_gate__         | 1 qubit, 1 angle           | rotation     |
| __ry_gate__         | 1 qubit, 1 angle           | rotation     |
| __rz_gate__         | 1 qubit, 1 angle           | rotation     |
+---------------------+----------------------------+--------------+
| __prepz_gate__      | 1 qubit                    | rotation     |
+---------------------+----------------------------+--------------+
| __cnot_gate__       | 2 qubits                   | rotation     |
+---------------------+----------------------------+--------------+
| __cphase_gate__     | 2 qubits                   | rotation     |
+---------------------+----------------------------+--------------+
| __toffoli_gate__    | 3 qubits                   | rotation     |
+---------------------+----------------------------+--------------+
| __custom_gate__     | defined by config file     |              |
+---------------------+----------------------------+--------------+
| __composite_gate__  | defined by config file     |              |
+---------------------+----------------------------+--------------+
| __measure_gate__    | 1 qubit                    | measurement  |
+---------------------+----------------------------+--------------+
| __display__         | 0 or more qubits           | directive    |
+---------------------+----------------------------+--------------+
| __display_binary__  | 0 or more qubits           | directive    |
+---------------------+----------------------------+--------------+
| __nop_gate__        | none                       | scheduling   |
+---------------------+----------------------------+--------------+
| __dummy_gate__      | none                       | scheduling   |
+---------------------+----------------------------+--------------+
| __swap_gate__       | 2 qubits                   | rotation     |
+---------------------+----------------------------+--------------+
| __wait_gate__       | 0 or more qubits, duration | scheduling   |
+---------------------+----------------------------+--------------+
| __classical_gate__  | 0 or more classical regs.  | classical    |
+---------------------+----------------------------+--------------+

There is an API for each of the above gates types using default gates.
Some notes:

- the wait gate waits for all its (qubit) operands to be ready;
  then it takes a duration of the given number of cycles for each of its qubit operands to execute;
  in external representations it is usually possible to not specify operands, it then applies to all qubits of the program;
  the barrier gate is sometimes found in external representations
  but is identical to a wait with 0 duration on its operand qubits (or all when none were specified)

- the nop gate is identical to "wait 1", i.e. a one cycle execution duration applied to all program qubits

- dummy gates are SOURCE and SINK; these gates don't have an external representation;
  these are internal to the scheduler

- custom and composite gates are fully specified in the configuration file;
  these shouldn't have this type because it doesn't serve a purpose
  but have a type that reflects its semantics



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
"k.gate(name, qubit operand vector, classical operand vector, duration, angle)",
in which particular operands can be empty or 0 depending on the particular kind of gate that is created.
Gate creation upon a call to this API goes through the following steps to create the internal representation:

# the qubit and/or classical register operand indices are checked for validity,
  i.e. to be in the range of 0 to the number specified in the program creation API minus 1

# if the configuration file contains a definition for a specialized composite gate matching it, it is taken;
  the parameter substitution in the gates of the decomposition specification is done;
  each resulting gate must be available as (specialized or parameterized, and non-composite) custom gate,
  or as a default gate; the decomposition is applied and all resulting gates are created and added to the circuit

# otherwise, if a parameterized composite gate is available, take it;
  the parameter substitution in the gates of the decomposition specification is done;
  each resulting gate must be available as (specialized or parameterized, and non-composite) custom gate,
  or as a default gate; the decomposition is applied and all resulting gates are created and added to the circuit

# otherwise, if a specialized custom gate is available, take it;

# otherwise, if a parameterized custom gate is available, take it;

# otherwise, if a default gate (predefined internally in OpenQL) is available, take it;

# otherwise, it is an error




Output external representation
------------------------------

There are two closely related output external representations supported, both dialects of QASM 1.0:

- sequential QASM

- bundled QASM

When the gate's cycle attribute is still undefined,
the sequential QASM representation is the only possible external QASM representation.
Gates are specified one by one, each on a separate line.
A gate meant to execute after another gate should appear on a later line than the latter gate,
i.e. the gates are topologically sorted with respect to their intended execution order.
Kernels start with a label which names the kernel and serves as branch target in control gates.
Kernels optionally end with a control gate.

Once the gate's cycle attribute has been defined (and until it is invalidated),
in addition to the sequential QASM representation above (ignoring the cycle attribute values),
the bundled QASM representation can be generated that does reflect the cycle attribute values.

Each line in the bundled QASM representation
represents the gates that start execution in one particular cycle
in a curly bracketed list with vertical bar separators.
Each subsequent line represents a subsequent cycle.
When there isn't a gate that starts execution in a particular cycle,
a wait gate is specified instead with as integral argument the number of cycles to wait.
As with the sequential QASM representation,
kernels start with a label which names the kernel and serves as branch target in control gates,
and kernels optionally end with a control gate.
