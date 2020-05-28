.. _compiler_passes:

Compiler Passes
===============

Most of the passes in their function and implementation are platform independent,
deriving their platform dependent information from options and/or the configuration file.
This holds also for mapping, although one wouldn't think so first,
since it is called from the platform dependent part of the compiler now.
All passes like this are summarized below first and are described extensively platform independently later in this section.

:Note: Some passes are called from the platform independent compiler, other ones from the back-end compiler. That is a platform dependent issue and therefore described with the platform.

Their description includes:

- their API, including their name: in general, apart from their name, they take all parameters from the ``program`` context which includes the configuration file and the platform, the options, and the vector of kernels with their circuits
- the Intermediate Representation (IR) they expect as input and what they update in the IR; in general, they should accept any IR, so all types of gates, quantum as well as classical
- the particular options they listen to; there usually is an option to disable it; also there are ways to dump the IR before and/or after it although this is not generally possible yet
- the function they perform, in terms of the IR and the options

Other passes, of which the implementation (i.e. source code etc.) is platform dependent,
can be found with the platforms.
An example of the latter passes is QISA (i.e. instruction) generation in CC-Light.
In the lists below, these passes are indicated to be platform dependent.

Passes have some general facilities available to them; these are not passes themselves since they don't transform the IR.
Examples of such facilities are:

- ``ql::report::report_qasm(prog_name, kernels, platform, relplacename, passname)``:
    Writing the IR out in an external representation (QASM 1.0) to a file
    when option ``write_qasm_files`` has the value ``yes``.
    The file is stored in the default output directory;
    the name of the file is composed from the program name (``prog_name``), the place relative to the pass (``relplacename``),
    and the pass name (``passname``), all separated by ``_``, and the result suffixed by ``.qasm``.
    The pass indicates before or after which the IR is written to file.
    The place relative to the pass indicates e.g. ``in`` or ``out``, meaning before or after the pass, respectively.
    In this way, multiple qasm files can be written per compile, and be easily related to the point in compilation
    where the writing was done.

- ``ql::report::report_bundles(prog_name, kernels, platform, relplacename, passname)``:
    Identical to ``report_qasm`` but the QASM is written as bundles.

- ``ql::report::report_statistics(prog_name, kernels, platform, relplacename, passname, prefix)``:
    Identical to ``report_qasm`` but the IR itself is not written but a summary of it,
    e.g. the number of kernels,
    the numer of one-qubit, two-qubit and more-qubit gates,
    which qubits were used and which not,
    the wall clock time that compilation took until this point,
    etc.
    This is done for each kernel separately and for the whole program;
    additional interfaces are available for making the individual reports
    and adding pass specific lines to the reports.
    The ``prefix`` string is prepended to each line in the report file, e.g. to make it qasm comment.
    Furthermore the suffix is ``.report``.
    And writing the report is only done
    when option ``write_report_files`` has the value ``yes``.

- ``ql::utils::write_file(filename, contentstring)``:
    Writing a content string to the file with given ``filename``
    in the default output directory for off-line inspection.
    An example is writing (in dot format) the gate dependence graph which is a scheduling pass internal data structure.
    The writing to a file of a string is a general facility
    but the generation of the string representation of the internal data structure is pass dependent.
    The options controlling this are also pass specific.

Writing the IR out to a file in a form suitable for a particular subsequent tool such as quantumsim
is considered code generation for the quantumsim platform and is therefore considered a pass.

:Note: A compiler pass is not something defined in OpenQL. It should be. Passes then have a standard API, standard intermediate representation dumpers before and after them, a standard way to include them in the compiler. We could have the list of passes to call be something defined in the configuration file, perhaps with the places where we want to have dumps and reports.

.. _summaries_of_compiler_passes:

Summary of compiler passes
----------------------------

Compiler passes in OpenQL are the compiler elements that, when called one after the other,
gradually transform the OpenQL input program to some platform defined output program.
The following passes are available and usually called in this order.
More detailed information on each can be found in the sections below.

When it is indicated that a pass is CC-Light (or any other platform) dependent,
it means that its implementation with respect to source code is platform dependent.
A pass of which the source code is platform independent, can behave platform dependently
by its parameterization by the platform configuration file.

- program reading
	not a real pass now;
	it covers the code that for a particular program
	sets its options,
	connects it to a platform,
	defines its program parameters such as number of qubits,
	defines its kernels,
	and defines its gates;
	in the current OpenQL implementation this is all code upto and including the call to ``p.compile()``.
	See :ref:`input_external_representation` and :ref:`creating_your_first_program`.

- optimize
	attempts to find contigous sequences of quantum gates
	that are equivalent to identity (within some small epsilon which currently is 10 to the power -4)
	and then take those sequences out of the circuit;
	this relies on the function of each gate to be defined in its ``mat`` field as a matrix.
	See :ref:`optimization`.

- decompose_toffoli
	each toffoli gate in the IR is replaced by a gate sequence with at most two-qubit gates;
	depending on the value of the equally named option; it does this in the Neilsen and Chuang way (``NC``),
	or in the way as in https://arxiv.org/pdf/1210.0974,pdf (``AM``).
	See :ref:`decomposition`.

- unitary decomposition
	the unitary decomposition pass is not generally available yet; it is in some private OpenQL branch.
	See :ref:`decomposition`.

- scheduling
	of each kernel's circuit the gates are scheduled at a particular cycle starting from 0
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
	See :ref:`scheduling`.

- decomposition before scheduling (CC-Light dependent)
	classical non-primitive gates are decomposed to primitives
	(e.g. ``eq`` is transformed to ``cmp`` followed by an empty cycle and an ``fbr_eq``);
	after measurements an ``fmr`` is inserted provided the measurement had a classical register operand.
	See :ref:`decomposition`.

- clifford optimization
	dependency chains of one-qubit clifford gates operating on the same qubit
	are replaced by equivalent sequences of primitive gates when the latter leads to a shorter execution time.
	Clifford gates are recognized by their name and use is made of the property
	that clifford gates form a group of 24 elements.
	Clifford optimization is called before and after the mapping pass.
	See :ref:`optimization`.

- mapping
	the circuits of all kernels are transformed
	such that for any two-qubit gate the operand qubits are connected
	(are NN, Nearest Neighbor) in the platform's topology;
	this is done by a kernel-level initial placement pass and when it fails, by a subsequent heuristic;
	the heuristic essentially transforms each circuit from start to end;
	doing this, it maintains a map from virtual (program) qubits to real qubits (``v2r``);
	each time that it encounters a two-qubit gate that in the current map is not NN,
	it inserts swap gates before this gate that gradually make the operand qubits NN;
	when inserting a swap, it updates the v2r map accordingly.
	There are many refinements to this algorithm that can be controlled through options and the configuration file.
	It is not complete in the sense that it ignores transfer of the v2r map between kernels.
	See :ref:`mapping`.

- rcscheduler
	resource constraints are taken into account; the result reflects the timing required during execution,
	i.e. also taking into account any further non-OpenQL passes and run-time stages such as (for CC_Light):

	* QISA assembly
	* classical code execution (from here on these passes are executed as run-time stages)
	* quantum microcode generation
	* micro operation to signal and microwave conversion
	* execution unit reprogramming and inter operation reset times
	* signal communication line delays
	* execution time and feed-back delays

	The resulting circuit is stored in the usual manner and as a sequence of bundles.
	See :ref:`scheduling`.

- decomposition after scheduling (CC-Light dependent)
	two-qubit flux gates are decomposed to a series of one-qubit flux gates
	of the form ``sqf q0`` to be executed in the same cycle;
	this is done only when the ``cz_mode`` option has the value ``auto``;
	such a gate is generated for each operand and for all qubits that need to be detuned;
	see the detuned_qubits resource description in the CC-Light platform configuration file for details.
	See :ref:`decomposition`.

- opcode and control store file generation (CC-Light dependent)
	currently disabled as not used by CC-Light

- write_quantumsim_program
    writes the current IR as a python script that interfaces with quantumsim

- write_qsoverlay_program
    writes the current IR as a python script that interfaces with the qsoverlay module of quantumsim

- QISA generation (CC-Light dependent)

	* bundle to QISA translation

		* deterministic sorting of gates per bundle
		* instruction prefix and wait instruction insertion
		* classical gate to QISA classical instruction translation
		* SOMQ generation and mask to mask register assignment (should include mask instruction generation)
		* insertion of wait states between meas and fmr (should be done by scheduler)

	* mask instruction generation
	* QISA file writing

	See :ref:`platform`.

.. include:: decomposition.rst
.. include:: optimization.rst
.. include:: scheduling.rst
.. include:: mapping.rst
