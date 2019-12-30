Overview
========

OpenQL is a framework for high-level quantum programming in C++/Python.
The framework provides a compiler for compiling and optimizing quantum code.

This document
-------------

The first three chapters introduce OpenQL,
help to install it,
and show how to create a first OpenQL program.
They are here for people who want to get going with OpenQL as quickly as possible.
For people just wanting an overview of OpenQL, these, except for the installation chapter, are a must read.

Further chapters introduce to the basic concepts of OpenQL.
They contain a lot of conceptual texts, and inevitable for a good understanding of the system.
What kind of gates does OpenQL support, which are the internal and which are the external representations?
What is a program, what is a kernel and to which extent are classical instructions supported?
Omni-present in OpenQL is the platform, literally in the form of the platform configuration file
that parameterizes most passes on the supported platform.
And finally the compiler passes, in a summary as well as in an extensive description with functional description
and sets of options listened too.

The document concludes with lists of APIs and indices.

OpenQL compiler structure
-------------------------

An OpenQL compiler reads a quantum program written in some external representation,
performs several analysis and transformation passes on it,
and prints the result to an external representation again.
Internally in the compiler the passes operate on a common internal representation of the program,
IR for short, which is equal to all passes.

Understanding this internal representation is key
to understanding the operation of an OpenQL compiler.
It is structured as an attributed tree of objects.

At the top one finds the (internal representation of the) program.
Its main component is the vector of kernels.
Each ordinary kernel (object) contains a single circuit which basically is a vector of gates.
Gates in OpenQL are the constructs
that refer to operations to be executed somehow on the computing platform.
These can be quantum gates as well as classical gates;
the latter deal with classical arithmetic and measurement results.
A circuit of a kernel is always executed from start to end.
There are special kernels without a circuit that take care of control flow between kernels.
But for ordinary kernels 
after the last gate control is transferred to the next kernel.

All passes operate at the program level.
Each performs its work on all kernels before it completes and another pass can run.
The order of the passes is predefined by OpenQL,
but there are ways to enable/disable individual passes.
The effect of a pass is to update the internal representation.
This can amount to computing attributes, replacing gates by other ones,
rearranging gates, and so on.

The objective of an OpenQL compiler is
to produce an output external representation of the input program
that satisfies the needs of what comes next.
What comes next is represented in OpenQL by the (target) platform.
These platforms can be software simulators or architectures targetting hardware quantum computers.

To the compiler this platform is described by a *platform configuration file*,
a file in JSON format,
which contains several sections with descriptions of attributes of the platform.
Examples of these are the number of qubits,
the supported set of primitive gates with their attributes,
the connection graph between the qubits (also called the topology of the grid),
and the classical control constraints imposed by the control electronics
of the hardware of the platform.
It also specifies for which hardware platform family it contains the configuration.
These hardware platform families (called architectures) are built-in into the OpenQL compiler,
and the compiler, after having executed some platform independent passes,
will enter the architecture-specific part of the compiler
where it executes several platform dependent passes.
When compiling for a hardware quantum computer target,
the last ones of these will generate some form
of low-level assembly code corresponding to the particular instruction set of the platform.
