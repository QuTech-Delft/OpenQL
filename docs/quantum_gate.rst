Quantum Gates
=============

Gates in OpenQL are the constructs that refer to operations.
They are organized in circuits as vectors of gates, and by that define the operation of a kernel.
Gates is can be subdivided in several kinds.
This is useful in the description of the passes below.

First, gates can be subdivided in:
- quantum gates; these have at least one qubit as (implicit or explicit) operand

- classical gates; these don't have any qubit as operand, only zero or more classical registers

- non-executing gates; these look like gates but don't influence execution, e.g. the display gate

Quantum gates can be subdivided in several ways:
- one-qubit gates; quantum gates operating on one qubit

- two-qubit gates; quantum gates operating on two qubits;
  these are the main objective in mapping since two-qubit gates require their qubit operands to be connected in the hardware

- more-qubit gates; quantum gates operating on more than two qubits;
  these must be decomposed to one-qubit and two-qubit gates because more-qubit primitive gates are not supported


Quantum gates can also be subdivided into:
- preparation gates; one-qubit gates taking a qubit in an undefined state and bringing it in a particular defined state

- rotation gates; gates that perform rotations on the state of the operand qubits

- measurement gates; gates that measure out the operand qubits, leaving them in a base state


Particular classes of quantum gates can be further recognized:
- primitive gates; quantum gates natively supported by instructions of the quantum hardware

- pauli gates; the identity, X, Y and Z rotation gates

- clifford gates;
  a group of quantum gates each doing only a rotation of a multiple of 90 degrees in one dimension (X, Y or Z)

- default gates; quantum gates defined internally by OpenQL

- custom gates; quantum gates defined in the platform configuration file

- composite gates; custom gates that are decomposed to their component gates when created

- specialized gates; custom gates with a definition that includes the specification of the qubit operands;
  the semantic attributes of several specialized gates
  with the same quantum operation but different qubit operands may differ

- generalized gates; custom gates with a definition that doesn't include a specification of qubit operands;
  when creating a custom gate with particular operands, when there is not specialized gate that matches,
  a generalized gate will be created; all gates stemming from the same generalized gate in the platform configuration file
  have the same semantic attributes


Classical gates
- simple gate
- control gate
- directive

Gate attributes
- structural attribute
- semantic attribute
- result attribute
