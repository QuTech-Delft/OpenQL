Compiler Passes
===============

Most of the passes in their function and implementation are platform independent,
deriving their platform dependent information from options and/or the configuration file.
This holds also for scheduling and mapping, although one wouldn't think so first.
All passes like this are mentioned below and described platform independently:
- their API, including their name: in general, apart from their name, they take all parameters from the ``program`` context which includes the configuration file and the platform, the options, and the vector of kernels with their circuits
- the Intermediate Representation (IR) they expect as input and what they update in the IR; in general, they should accept any IR, so all types of gates, quantum as well as classical
- the particular options they listen to; there usually is an option to disable it; also there are ways to dump the IR before and/or after it although this is not generally possible yet.

Other passes, of which the function and implementation is platform dependent, can be found with the platforms.
An example of the latter passes is QISA (i.e. instruction) writing in CC-Light.

Passes have some facilities available to them; these are not passes themselves.
Such facilities are:
- Writing the IR out in an external representation (QASM 1.0) to a file;
	the name of the file should relate to the point in compilation relative to the called passes where
	the IR is written
- Writing a report out to a file containing a summary of the IR;
	e.g. the number of kernels,
	the numer of one-qubit, two-qubit and more-qubit gates,
	which qubits were uses and which not,
	the wall clock time that compilation took until this point,
	etc.
- Writing an internal data structure to a file for off-line inspection such as
	the gate dependence graph (in dot format) during scheduling

These facilities are described elsewhere.

Writing the IR out to a file in a form suitable for a particular subsequent tool such as quantumsim
is considered code generation for the quantumsim platform and is therefore considered a pass.

:Note: A compiler pass is not something defined in OpenQL. It should be. Passes then have a standard API, standard intermediate representation dumpers before and after them, a standard way to include them in the compiler. We could have the list of passes to call be something defined in the configuration file.

Compiler passes in OpenQL are the compiler elements that, when called one after the other,
gradually transform the OpenQL input program to some platform defined output program.
The following passes are available and usually called in this order:
- program reading: not a real pass now;
	it covers the code that for a particular program
	sets its options,
	connects it to a platform,
	defines its program parameters such as number of qubits,
	defines its kernels,
	and defines its gates;
	in the current OpenQL implementation this is all code upto and including the call to ``p.compile()``.
- optimize: attempts to find contigous sequences of quantum gates
	that are equivalent to identity (within some small epsilon which currently is 10 to the power -4)
	and then take those sequences out of the circuit;
	this relies on the function of each gate to be defined in its ``mat`` field as a matrix.
- decompose_toffoli: each toffoli gate in the IR is replaced by a gate sequence with at most two-qubit gates;
	depending on the value of the equally named option, it does this in the Neilsen and Chuang way (``NC``),
	or in the way as in https://arxiv.org/pdf/1210.0974,pdf (``AM``).
- scheduler: of each kernel's circuit the gates are scheduled at a particular cycle starting from 0
	(by filling in the gate's ``cycle`` attribute) that matches the gates' dependences, their duration,
	the constraints imposed by their resource use, the buffer values defined for the platform, and
	the latency value defined for each gate; multiple gates may start in the same cycle;
	in the resulting circuits (which are vectors of pointers to gate) the gates are ordered by their cycle value.
	The schedulers also produce a ``bundled`` version of each circuit:
	the circuit is then represented by a vector of bundles
	in which each bundle lists the gates that are to be started in the same cycle;
	each bundle further contains sublists that combines gates with the same operation but with different operands.
	The resource-constrained and non-constrained versions of the scheduler have different entry points (currently).
	The latter only considers the gates' dependences and their duration, which is sufficient as input to QX.
	Next to the above necessary constraints, the remaining freedom is defined by a scheduling strategy
	which is defined by the ``scheduler`` option value: ``ASAP``, ``ALAP`` and some other options.
- mapper: the circuits of all kernels are transformed
	such that for any two-qubit gate the operand qubits are connected
	(are NN, Nearest Neighbor) in the platform's topology;
	this is done by a kernel-level initial placement pass and when it fails, by a subsequent heuristic;
	the heuristic essentially transforms each circuit from start to end;
	doing this, it maintains a map from virtual (program) qubits to real qubits (``v2r``);
	each time that it encounters a two-qubit gate that in the current map is not NN,
	it inserts SWAP gates before this gate that gradually make the operand qubits NN;
	when inserting the SWAP, it updates the v2r map accordingly.
	There are many refinements to this algorithm that can be controlled through options and the configuration file.
	The mapper pass is not generally available yet; it is in the OpenQL mapper branch.
	It is not complete in the sense that it ignores transfer of the v2r map between kernels.
- opcode and control store file generation: currently disabled as not used by CC-Light
- decomposition before scheduling: classical non-primitive gates are decomposed to primitives
	(e.g. ``eq`` is transformed to ``cmp`` followed by an empty cycle and an ``fbr_eq``);
	after measurements an ``fmr`` is inserted provided the measurement had a classical register operand.
- rcscheduler: see scheduler
- decomposition after scheduling: two-qubit flux gates are decomposed to a series of one-qubit flux gates
	of the form ``sqf q0`` to be executed in the same cycle;
	this is done only when the ``cz_mode`` option has the value ``auto``;
	such a gate is generated for each operand and for all qubits that need to be detuned;
	see the detuned_qubits resource description in the CC-Light platform configuration file for details.
- bundles2qisa: ...
- mask instruction generation: ...
- QISA generation: ...

:Note: Some passes are called from the platform independent compiler, other ones from the back-end compiler. That is a platform dependent issue and therefore described with the platform.

.. include:: decomposition.rst
.. include:: optimization.rst
.. include:: scheduling.rst
.. include:: mapping.rst
