.. _mapping:

Mapping
-------

The circuits of all kernels are transformed
such that for any two-qubit gate the operand qubits are connected
(are NN, Nearest Neighbor) in the platform's topology;
this is done by a kernel-level initial placement pass and when it fails, by a subsequent heuristic;
the heuristic essentially transforms each circuit from start to end;
doing this, it maintains a map from virtual (program) qubits to real qubits (``v2r``);
each time that it encounters a two-qubit gate that in the current map is not NN,
it inserts swap gates before this gate that gradually make the operand qubits NN;
when inserting a swap, it updates the ``v2r`` map accordingly.
There are many refinements to this algorithm that can be controlled through options and the configuration file.

The implementation is not complete:
In the presence of multiple kernels with control flow among them,
the ``v2r`` at the start of each kernel must match the ``v2r`` at the end of all predecessor kernels: this is not implemented.
Instead, the ``v2r`` at the start of each kernel is re-initialized freshly, independently of the ``v2r`` at the end of predecessor kernels.
The current implementation thus assumes that at the end of each kernel all qubits don't hold a state that must be preserved for a subsequent kernel.

.. _mapping_entry_points:

Entry points
^^^^^^^^^^^^

Mapping is implemented by a class ``Mapper`` with the support of many private other classes
among which the scheduler class for obtaining the dependence graph.  The following entry points are supported:

- ``Mapper()``
  Constructs a new mapper to be used for the whole program. Initialization is left to the ``Init`` method.

- ``Mapper.Init(platform)``
  Initialize the mapper for the given platform but independently of a particular kernel and circuit. This includes checking
  and initializing the mapper's representation of the platform's topology from the platform's configuration file.

- ``Mapper.Map(kernel)``
  Perform mapping on the kernel, i.e. replace the kernel's circuit by an equivalent but mapped circuit.
  Each kernel is mapped independently of any other kernel.
  Of each gate the ``cycle`` attribute is assigned, and the resulting circuit is scheduled;
  which constraints are obeyed in this schedule depends on the mapping strategy (the value of the ``mapper`` attribute).
  In the argument ``kernel`` object, the ``qubit_count`` attribute is updated from the number of virtual qubits
  of the kernel to the number of real qubits as specified by the platform; this is done because in the mapped circuit
  the qubit operands of all gates will be real qubit indices of which the values
  should be in the range of the valid real qubit indices of the platform.
  Furthermore, some reporting of internal mapper statistics is done into attributes of the ``Mapper`` object,
  which can be retrieved by the caller and converted to readable texts to report to the user:

  - ``nswapsadded``
    Number of swaps inserted.

  - ``nmovesadded``
    Number of moves inserted.

  - ``v2r_in``
    Vector with for each virtual qubit index its mapping to a real qubit index
    (or ``UNDEFINED_QUBIT`` represented by a very large value),
    after initialization of the mapper and before initial placement and/or the heuristics.

  - ``rs_in``
    Vector with for each real qubit index its state
    (no quantum state contained that should be preserved, initialized in a base state, or computed quantum state)
    after initialization of the mapper and before initial placement and/or the heuristics.
    
  - ``v2r_ip``
    Vector with for each virtual qubit index its mapping to a real qubit index
    (or ``UNDEFINED_QUBIT`` represented by a very large value),
    after initial placement and before the heuristics.

  - ``rs_ip``
    Vector with for each real qubit index its state
    (no quantum state contained that should be preserved, initialized in a base state, or computed quantum state)
    after initial placement and before the heuristics.
    
  - ``v2r_out``
    Vector with for each virtual qubit index its mapping to a real qubit index
    (or ``UNDEFINED_QUBIT`` represented by a very large value), after initialization of the mapper,
    after the heuristics.

  - ``rs_out``
    Vector with for each real qubit index its state
    (no quantum state contained that should be preserved, initialized in a base state, or computed quantum state)
    after the heuristics.
  

Input and output intermediate representation
^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^

The mapper expects kernels with or without a circuit.
When with a circuit, the ``cycle`` attribute need not be valid.
Gates that are supported on input are one-qubit ``measure``, no-operand ``display``, any classical gate,
``cnot``, ``cz``/``cphase``, and any other quantum and scheduling gate.
The mapper refuses multi-qubit quantum gates as input with more than two quantum operands.

The mapper produces a circuit with the same gates but then mapped (see below),
with the real qubit operands of two-qubit gates nearest-neighbor in the platform's topology,
and with additional quantum gates inserted to implement the swapping or moving of qubit states.
The mapping of a gate entails replacing the virtual qubit operand indices by the real qubit operand indices
corresponding to the mapping of virtual to real qubit indices applicable at the time of execution of the gate;
furthermore the gate itself is optionally replaced at the time of its mapping
by one or more gates as specified by the platform's configuration file,
by creating a gate with the name of the original gate with ``_real`` appended. Note that when this created gate is specified in
the configuration file in the ``gate_decomposition`` section, the net effect is that the specified decomposition is done.
When a swap or move gate is created to be inserted in the circuit, first a ``swap_real`` (or ``move_real``) is attempted
to be created instead before creating a ``swap`` or ``move``; this also allows the gate to be decomposed to more primitive
gates during mapping.

When a kernel's circuit has been mapped, an optional final decomposition of the mapped gates is done:
each gate is optionally replaced by one or more gates as specified by the platform's configuration file,
by creating a gate with the name of the original gate with ``_prim`` appended. Note that when this created gate is specified in
the configuration file in the ``gate_decomposition`` section, the net effect is that the specified decomposition is done.
When in the mapped circuit, ``swap`` or ``move`` gates were inserted and ``swap_prim`` or ``move_prim`` are specified
in the configuration file, these are also used to replace the ``swap`` or ``move``  at this time.

The ``cycle`` attribute of each gate has got a valid value.
The gates in the circuit are ordered with non-decreasing cycle value.
The cycle values are consistent with the constraints that are imposed during mapping; these are specified by the ``mapper`` option.

The above implies that non-quantum gates are accepted on input and are passed unchanged to output.

.. _mapping_options:

Options
^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^

The options that control the mapper are described: these are the proper mapper options and a subset of the scheduler options.
The subset of the scheduler options applies because the mapper uses the dependence graph created by the initialization method of the scheduler.
Also see :ref:`scheduling_options`.

Most if not all options can be combined to compose a favorite mapping strategy, i.e. the options are largely independent.

The options, their values and their effect are as follows:

- ``mapinitone2one:``
  Definition of the initialization of the ``v2r`` map at the start of the mapping of each kernel; this ``v2r`` will apply at the start of initial placement.

  - ``no:``
    there is no initial mapping of virtual to real qubits; each virtual qubit is allocated to the first free real qubit on the fly, when it is mapped

  - ``yes:``
    the initial mapping is 1 to 1: a virtual qubit with index ``qi`` is mapped to its real ``qi`` counterpart (so: same index)

- ``initialplace:``
  Definition of initial placement operation. Initial placement, when run, may be 100% successful (all two-qubit gates were made NN); be moderately successful (not all two-qubit gates were made NN, only some) or fail to find a solution:

  - ``no:``
    no initial placement is attempted

  - ``yes:``
    do initial placement starting from the initial ``v2r`` mapping; since initial placement employs an Integer Linear Programming model as the base of implementation, finding an initial placement may take quite a while.

  - ``1s, 10s, 1m, 10m, 1h:``
    do initial placement as with ``yes`` but limit execution time to the indicated maximum (one second, 10 seconds, one minute, etc.); when it is not successfull in this time, it fails, and subsequently the heuristics is done, which cannot fail.

  - ``1sx, 10sx, 1mx, 10mx, 1hx:``
    do initial placement as with ``yes`` but limit execution time to the indicated maximum (one second, 10 seconds, one minute, etc.); when it is not successfull in this time, it fails, and subsequently the compiler fails as well.

- ``initialplace2qhorizon:``

- ``mapper:``
  The basic mapper strategy that is employed:

  - ``no:``
    no mapping is done. The output circuit is identical to the input circuit. Other options don't have effect.

  - ``base:``
    map the circuit: use as metric just the length of the paths between the mapped operands of each two-qubit gate, and minimize this length for each two-qubit gate that is mapped

  - ``minextend:``
    map the circuit: use as metric the extension of the circuit by each of the shortest paths between the mapped operands of each two-qubit gate, and minimize this circuit extension for each two-qubit gate that is mapped

  - ``minextendrc:``
    map the circuit: as ``minextend``, but taking resource constraints into account when evaluating circuit extension

- ``scheduler_commute:``
  Because the mapper uses the dependence graph also generated for the scheduler, the variations made available by commutation of CZs/CNOTs are available to the mapper:

  - ``no:``
    don’t allow two-qubit gates to commute (CZ/CNOT) in the dependence graph; they are kept in original circuit order and presented to the mapper in this order

  - ``yes:``
    allow commutation of two-qubit CZ/CNOT gates; e.g. when one later one is already nearest-neighbor, allow it to be mapped before an earlier one which isn’t nearest-neighbor

- ``maplookahead:``
  How does the mapper exploit the lookahead offered by the dependence graph constructed from the input circuit?

  - ``no:``
    the mapper ignores the dependence graph and takes the gates to be mapped one by one from the circuit

  - ``critical:``
    gates that by definition do not need routing, are mapped first; these include the classical gates, wait gates, and the single qubit quantum gates; and of the remaining (two qubit) quantum gates the most critical gate is selected first, i.e. the one behind which most cycles are expected until the end of the circuit

  - ``noroutingfirst:``
    those two qubit quantum gates of which the operands are neighbors in the current mapping are mapped first, also when these are not critical; and when none such are left, only then take the most critical one

  - ``all:``
    as with noroutingfirst but don't select the most critical one; instead, for all remaining (two qubit non-NN) gates generate variations and find the best from these according to the strategy above

- ``maprecNN2q:``

- ``mappathselect:``
  when generating variations of shortest paths between two real qubits:

  - ``all:``
    select all possible variations

  - ``borders:``
    only select those variations that correspond to following the borders of the rectangle spanning between the two extreme real qubits

- ``mapselectmaxlevel:``

- ``mapselectmaxwidth:``

- ``maptiebreak:``
  when multiple variations remain for a particular strategy with the same best evaluation value, decide how to select the best single one:

  - ``first:``
    select the first of the set

  - ``last:``
    select the last of the set

  - ``random:``
    select in a random way from the set

- ``mapusemoves:``
  use move instead of swap where possible:

  - ``no:``
    don't

  - ``yes:``
    do, when swapping with an ancillary qubit which is known to be in the initial state ($|+>$ for moves with 2 CNOTs); when not in the initial state, insert a move\_init sequence (prepz followed by hadamard) when it doesn't additionally extend the circuit; when a move\_init sequence would extend the circuit, don't insert the move

  - ``0, 1, 2, 3, 4, 5, 6, 7, 8, 9, 10, 11, 12, 13, 14, 15, 16, 17, 18, 19, 20:``
    yes, and insert a move\_init sequence to get the ancillary qubit in the initial state, if needed; but only when the number of cycles of circuit extension that this move\_init causes, is less-equal than 0, 1, $...$ 20 cycles. Please note that later it was decided and implemented to assume that all real qubits start off in the initial state; this increases the likelihood that moves are inserted, and makes all these considerations of only inserting a move when a move\_init can bring the ancillary qubit in the initial state somehow without additional circuit extension, of no use.

- ``mapassumezeroinitstate:``

- ``mapprepinitstate:``

- ``mapselectswaps:``

- ``mapreverseswap:``
  reverse operand real qubits of swap when beneficial:

  - ``no:``
    don't

  - ``yes:``
    when scheduling a swap, exploiting the knowledge that the execution of a swap for one of the qubits starts one cycle later, a reversal of the real qubit operands might allow scheduling it one cycle earlier

- ``clifford_premapper:``
  do clifford gate sequence optimization before running the mapper

- ``clifford_postmapper:``
  do clifford gate sequence optimization before running the final resource-constrained scheduler, i.e. after running the mapper

Function
^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^

The basic mapper maps/routes one circuit at a time. In general, a quantum program may consist of multiple kernels, each with a single circuit.
In this a kernel corresponds to a basic block in classical compilers, i.e. a classical flow operation such as a jump can only be found at the end and the target of such jumps (a label) can only be found at its start; all code in between is free of control flow change and is sequentially executed from start to end. Apart from control flow operations, an OpenQL quantum program can also contain classical computation operations interspersed between the quantum operations. Preparation and measurement gates interface between the quantum and classical operations.

To simplify the mapper, it can only map two-qubit and single-qubit gates. More-qubit gates are usually not among the primitive gates supported by a quantum computer implementation and must be decomposed to primitives anyhow. Also several quantum computer implementations have grids with qubits as nodes with a very limited number of neighbor qubits (e.g. the IBM Poughkeepsie of which most qubits only have two neighbors) which thus makes mapping more-than-two-qubit gates cumbersome. So the mapper expects circuit decomposition to have been done before it.

After mapping the program adheres to the NN constraint but still has to deal with the other limitations imposed by the implementation, most notably the resource constraints (RC) that encode the control concurrency limitations together with the duration of the individual gates. This is done by a resource-constraints aware scheduler. Its objective is to achieve the shortest latency circuit and the highest instruction-level parallelism (ILP). The scheduler fills in the cycle field of each gate of the circuit with the cycle in which it is scheduled to start; the gates are ordered with non-decreasing cycle value in the circuit. After the scheduler, bundles are constructed: each bundle consists of the gates that start in the same processor cycle. Bundling is followed by the generation of assembly instructions.

In OpenQL the target platform including its grid with its topology, the instructions (elementary gate set) with their duration and the resource constraints are described in a configuration file encoded in json. These parameterize the mapper, scheduler and other OpenQL passes with respect to the target platform. The mapper, scheduler, etc. are meant to be reusable for a different underlying quantum technology. As compilation progresses from initial passes to scheduling and assembly, the original program is transformed gradually to adhere to all target platform limitations and to execute somehow optimally on that platform.

Steps:

- After pre-scheduling, the compiler will create an initial map of virtual qubits to ``real`` qubits: this can be in order (one to one: virtual q2 to real q2, etc.), or an empty map. Such a map is called the initial qubit \textbf{VP-map}.

- Then, the \textbf{initial placement} module will check whether the current VP-map maps the pair of operands of each two-qubit gate in the circuit to nearest neighbours in the ``chip topology``.  If not, it will use an Integer Linear Programming (ILP) algorithm to find an initial placement which requires a minimal number of qubit movements.

- Afterwards, the \textbf{router} (Purple dashed circle in Figure~\ref{fig:map-flow}) will route the qubits using a heuristic algorithm; see below.

- Using the resource constraints described in the configuration file, the \textbf{RC-scheduler} schedules the routed circuit in an ALAP manner because ALAP gives higher fidelity.  The output circuit is written in \textbf{cQASM} format specifying mapping statistics as comment with each kernel, and then transformed to QISA for execution on the quantum device.


The mapper itself operates as follows:

- it assumes a predefined initial mapping (\textbf{VP-map}; when multiple kernels are present in the OpenQL program with control flow among them, a kernel/circuit's initial mapping is computed from the output mapping of its predecessor kernels in the control-flow graph of kernels

- it attempts initial placement using the Integer Linear Programming model of Lingling; when it times out, initial placement is stopped; depending on options, this stopping is considered a failure for initial placement only or for the whole compiler; when it is a failure of initial placement only, the mapper continues with the heuristics starting from the initial mapping; when initial placement was succesfull, the mapper continues with the heuristics starting from the computed mapping

- it starts the heuristics by selecting one or more gates from the input DAG/dependence graph (which is the same as the scheduler uses); when a classical or single-qubit gate is encountered, it is mapped before any available two-qubit gate; when only two-qubit gates remain, prefer those that are already nearest neighbor (NN) in the current mapping; when then only non-NN two-qubit gates remain the currently best strategy is to take the one that is most critical in the remaining dependence graph (i.e. has the highest likelihood to extend the circuit when mapped in the wrong way or when delayed). But choosing an other option, all available gates in the dependence graph can be taken instead of only the most critical one. The following heuristics probably cannot beat taking the most critical one and need improvement.

- the heuristics select variations for all gates selected above; for each two-qubit gate it selects all shortest paths as variations and for each generates all variations of putting the two-qubit gate somewhere along the path; so always all variations have the least number of swaps/moves; optionally only the border paths are taken (when seeing the path end-point qubits as diagonal of a rectangle in the grid, the borders are the paths along the edges of this rectangle) are taken as initial variations.

- depending  on the mapper strategy, of these variations those are selected that minimally extend (in terms of cycles) the circuit without or with resource constraints taken into account; in this, there is variation to use swap gates (3 CNOTs) or move gates (2 CNOTs) when one of the qubits is an ancillary; gates are scheduled ASAP in a representation of the already mapped gates to evaluate how much the additional set of swaps/moves extends the circuit to optimize interleaving of swaps mutually and interleaving of swaps and mapped quantum code sequences (i.e. improving the ILP and thereby reducing the resulting circuit's depth)

- in doing this, either the swaps/moves are inserted as primitives or their decompositions to CNOTs are inserted, or their decomposition to primitives are inserted; insertion of swaps/mores produces more readable result code; insertion of sequences of primitives results in more final scheduler opportunities, i.e. more exact/better scheduling

- when still multiple variations remain with best evaluation, a tiebreak selects which one is taken; for the taken one, the swaps are inserted, scheduled in and the mapping updated

- when all gates have been mapped, optionally all non-primitive gates can still be decomposed, and the result is subject to the final ALAP resource-constrained scheduler

- and finally all results and statistics are gathered and some of these also included in the output files as comment (depth of circuit, numbers of inserted swaps/moves, etc.)

The mapper relies on the scheduler and inherits from the scheduler in the following way:

- It reuses the dependence graph, including the commutation support for CNOT/CZs.

- The list of available gates of the list scheduler is reused as set of gates to choose from to map next.

- The mapper uses a simple ASAP scheduling policy to optimize interleaving of gates and to find the minimal extension of a set of swaps implementing the required mapping of a 2q gate.

- It uses the resource manager to take resource constraints into account in the latter.

The mapper thus draws a lot on the scheduler and so scheduler development is regarded a part of mapper development.

The above detailed the mapping flow inside the OpenQL compiler. It left out other tasks performed by the compiler such as \textbf{optimization} (which may be applied repeatedly during the compilation) and further actions that are required to map to the actual hardware. The latter includes insertions of various kinds of delays (cable lengths and circuit switching times), generation of ``bundles`` that combine gates that start in the same cycle, generation of ``QISA instructions`` from these bundles, etc.
