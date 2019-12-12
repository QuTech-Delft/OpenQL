Overview
========

OpenQL is a framework for high-level quantum programming in C++/Python.
The framework provides a compiler for compiling and optimizing quantum code.
Getting an overview of the structure of this compiler, is the topic of this section.
Next sections will explain how to install it and how to make a first program.

Further sections will introduce you to the basic components of an OpenQL compiler,
and to the available APIs.

OpenQL compiler structure
-------------------------

An OpenQL compiler reads a quantum program written in some external representation,
performs some analysis and transformation passes on it, and prints the result out to an external representation again.
Internally in the compiler the passes operate on a common internal representation of the program, equal to all passes.

Understanding this internal representation is key to understanding the operation of an OpenQL compiler.
It is structured as an attributed tree of objects.

At the top one finds the (internal representation of the) program.
Its main component is the vector of kernels.
Each kernel (object) contains a single circuit which basically is a vector of gates.
Gates in OpenQL are the constructs that refer to operations to be executed somehow on the computing platform.
These can be quantum gates as well as sequential gates; the latter deal with measurement results and control flow.
A circuit of a kernel is always executed from start to end.
After the last gate control is transferred to a next kernel.
When this is not the next kernel in the vector, the last gate must be a control gate,
i.e. a sequential gate that represents a (conditional) branch.

All passes operate at the program level.
Each performs its work on all kernels before it completes and another pass can run.
The order of the passes is predefined by OpenQL, but there are ways to enable/disable individual passes.
The effect of a pass is to update the internal representation, IR for short.
This can amount to computing attributes, replacing gates by other ones, rearranging gates, and so on.

The objective of an OpenQL compiler is to produce an output external representation of the input program
that satisfies the needs of what comes next.
What comes next is represented in OpenQL by the (target) platform.
These platforms can be software simulators or architectures targetting hardware quantum computers.

To the compiler this platform is described by a *platform configuration file*, a file in JSON format,
which contains several sections with descriptions of attributes of the platform.
Examples of these are the number of qubits,
the supported set of primitive gates with their attributes,
the connection graph between the qubits (also called the topology of the grid),
and the classical control constraints imposed by the control electronics of the hardware of the platform.
It also specifies for which hardware platform family it contains the configuration.
These hardware platform families (called architectures) are built-in into the OpenQL compiler,
and the compiler, after having executed some platform independent passes,
will enter the architecture-specific part of the compiler where it executes several platform dependent passes.
When compiling for a hardware quantum computer target, the last ones of these will generate some form
of low-level assembly code corresponding to the particular instruction set of the platform.

Running an OpenQL compiler
--------------------------
In the OpenQL framework, the quantum program (including kernels and gates)
is created by API calls which are contained in a C++ or Python program.
But before this is done, the internal representation of the platform is created
by an API call that takes the name of the platform configuration file as one of its parameters.
This platform configuration file is consulted by the APIs creating the program, kernels and gates
to generate the matching internal representation of each gate.
...
