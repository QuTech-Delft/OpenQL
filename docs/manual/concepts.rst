Concepts
========

This section goes over some key concepts that you should understand before
doing anything with OpenQL.

OpenQL versus other compilers
-----------------------------

A key difference between the OpenQL compiler and traditional compilers is that
OpenQL is a library rather than an application. That means that you can't just
invoke OpenQL on the command line given some input file. Instead, your input
file is (usually) a Python script that imports the OpenQL module, builds a
representation of the algorithm that OpenQL understands as it runs, and
eventually tells OpenQL to compile that algorithm representation somehow. The
output of the compilation process is then usually written to output files,
though the behavior of the compiler depends entirely on its configuration.

Also different compared to most compilers is that OpenQL is inherently
retargetable. Whereas with for instance ``gcc`` the target architecture is
built right into it, with OpenQL you can compile code for many different kinds
of control architectures and quantum devices. This target architecture is
described via the platform configuration structure.

Platform configuration
----------------------

In OpenQL, the *platform configuration* is what determines what quantum device
and control architecture will be compiled for, also known as the compilation
*target*. It ultimately defines the subset of describable quantum circuits that
can actually be executed on the target by way of a set of constraints and
reduction rules. Here are some examples of things described in the structure.

 - The primitive instruction set, along with decomposition rules for common
   gates that cannot be directly represented.
 - The number of usable qubits within the device and their connectivity.
 - Control and instrument constraints on available gate parallelism.

The goal for the compiler is to take the user-specified algorithm and convert
it to a behaviorally equivalent circuit within this set, preferably the most
optimal one it can find.

As of version 0.9, OpenQL has
:ref:`a bunch of default target descriptions built into it<ref_architectures>`.
You can use them directly if they're good enough for your use case, or you can
use them as a baseline for making your own. The complete configuration
structure is defined :ref:`here<ref_platform_configuration>`.

Quantum algorithm representation
--------------------------------

.. note::
   This is *not* a description of the current implementation of the
   intermediate representation of the compiler, but rather an overview of what
   it behaves like from a user perspective.

OpenQL models a quantum algorithm as follows:

 - a complete algorithm is referred to as a *program*;
 - a program consists of one or more *kernels*; and
 - each kernel consists of one or more "statically-scheduled" *gates* (a.k.a.
   instructions) without control-flow within the kernel.

Typically, most of the Python or C++ program using the OpenQL compiler consists
of building an algorithm using this model, although it's also possible to build
it from a cQASM file using the cQASM reader pass.

Depending on your background, "static," "scheduled," and "control-flow" may
require further explanation.

 - "Static" just means "known by the compiler," or equivalently, "not dependent
   on information only known at runtime." In the world of quantum computing,
   this typically just means "not dependent on measurement results."
 - The "schedule" is what defines when a gate is applied, in this context
   relative to the start of the kernel. So, "statically-scheduled" means that
   (if a gate is applied) that gate must always be applied at the same time
   with respect to the start of the kernel.
 - "Control-flow" is *almost* anything to do with conditional statements (like
   ``if``) and loops. More formally, anything that results in a classical
   branch instruction is considered to be control-flow. Note that OpenQL also
   supports a special case for ``if``-like constructs called
   *conditional gate execution* that does not rely on control-flow; we'll get
   to that.

The result of the above is that a kernel behaves just like how you would
traditionally draw a quantum circuit, with time on the X-axis and the qubits
and classical bits on the Y-axis in the form of horizontal lines.

More complex algorithms that include control-flow can be specified using
multiple kernels. Say, for instance, that you have an initialization circuit,
then a circuit that you want to repeat until some qubit measures as 1, followed
by a circuit that does some final measurements. The first and last circuit
would then be added to the program as a normal kernel, while the second would
be added as a do-while kernel.

.. warning::
   Most architectures and parts of OpenQL don't fully support nonstandard
   kernel types yet. Some (mapping) don't support multiple kernels in any form.
   For now, you have to check the documentation of the passes used by the
   particular compiler configuration that you intend to use to see what's
   supported and what isn't.

Control-flow based on measurement results tends to be a costly operation in
most architectures, because the time from sending a measurement gate to the
instruments to being able to act on the measurement result tends to be quite
long compared to the coherence time of NISQ-era qubits. However, sometimes
part of this pipeline can be avoided. Say, for instance, that you want to
apply an X gate on some qubit only if some other qubit measured as 1. If the
instruments themselves (or at least a deeper part of the control architecture)
are capable of turning an X gate into an identity/no-op gate based on a
measurement, this and subsequent gates can already be queued up before the
measurement has actually taken place. This is the conditional gate execution
we alluded to earlier. Using this scheme, the condition for whether the gate
is executed or not is encoded as part of the gate, instead of being part of
the program's control-flow.

Gate representation
-------------------

Gates in OpenQL fundamentally consist of a name, some set of operands, and a
condition. The gate names available for use are defined within the platform
configuration file, along with some of their semantics, such as the gate
duration.

.. warning::
   As of version 0.9, OpenQL also still assigns semantics and makes
   assumptions based on the name of a gate however. For example, an ``x`` gate
   is assumed to commute with an ``x90`` gate, and both are assumed to have a
   single qubit operand and nothing else, or things will probably break. This
   behavior is unfortunately largely undocumented, so you'll have to search
   through the code for it. Obviously this is not an ideal situation, and thus
   this is something that we want to get rid of. All semantics needed by OpenQL
   should, down the line, be specified in the platform configuration, or, for
   backward compatibility, be inferred from the gate name in a documented way.

The operand set for each gate consists of the following:

 - zero or more qubit operands;
 - zero or more creg operands;
 - zero or more breg operands;
 - zero or one literal integer operand*; and
 - zero or one angle operand.

Here, "cregs" refer to classical integer registers, and "bregs" refer to
classical bit registers. The former are used for loops and other control flow,
while the latter are used for conditional execution.

Finally, the gate's condition consists of a boolean function applied to zero,
one, or two bregs. Unconditional gates are simply modelled using a unit-one
boolean function acting on zero bregs.

Configuring the compilation process
-----------------------------------

We've now described the way in which you specify the input and the target for
the compiler, but there's one more thing OpenQL must know: *how* to compile
for the given target. This is also known as the compilation *strategy*. When
the strategy is incorrect or insufficient, the resulting circuits may not
actually be completely valid for the target, unless the incoming algorithm is
carefully written such that constraints not dealt with by OpenQL have already
been met.

Generally, the compilation process consists of the following steps:

 - decomposition;
 - optimization;
 - mapping;
 - scheduling; and
 - code generation.

Decomposition is the act of converting gates that cannot be executed using a
single instruction in the target gateset into a list of gates that have the
same behavior. For example, a SWAP gate may be decomposed into three CNOT
gates.

Optimization tries to reduce the algorithm to a more compact form. This is
particularly relevant after decomposition, as the decomposition rules may
lead to sequences of gates that trivially cancel each other out.

Mapping is the act of changing the qubit indices in the circuit such that the
connectivity constraints of the target device are met. For complex circuits,
no single mapping will suffice (or it may be too time-consuming to compute,
as this is an NP problem); in this case, SWAP gates will be inserted to route
non-nearest-neighbor qubits toward each other.

Scheduling is the act of assigning cycle numbers to each gate in a kernel.
This can of course be done trivially by assigning monotonously increasing
cycle numbers to each gate in the order in which they were written by the
user, but this is highly inefficient; instead, heuristics and commutation
rules are used to try to find a more optimal solution that makes efficient
use of the parallelism provided by the control architecture.

Finally, code generation takes the completed program and converts it to the
assembly or machine-code format that the architecture-specific tools expect
at their input.

A strategy consists of a list of :ref:`passes<ref_passes>`, along with
pass-specific configuration options for each pass. OpenQL provides default
pass lists for the available architectures, as listed in the
:ref:`architecture reference<ref_architectures>`. You can modify this default
strategy using API calls prior to compilation if need be, or you can override
the defaults entirely by writing a
:ref:`compiler configuration file<ref_compiler_configuration>`.
