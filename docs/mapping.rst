.. _mapping:

Mapping
-------

The circuits of all kernels are transformed
such that after mapping for any two-qubit gate the operand qubits are connected
(are NN, Nearest Neighbor) in the platform's topology;
this is done by a kernel-level initial placement pass and when it fails, by a subsequent heuristic pass.
Both maintain a map from virtual (program) qubits to real qubits (``v2r``)
and a map from each real qubit index to its state (``rs``);
both are available after each of the two mapping passes.

- ``initial placement``
  This module attempts to find a single mapping of the virtual qubits of a circuit to the real qubits (``v2r`` map) of the platform's qubit topology,
  that minimizes the sum of the distances between the two mapped operands of all two-qubit gates in the circuit.
  The distance between two real qubits is the minimum number of swaps that is required to move the state of one of the two qubits to the other.
  It employs a Mixed Integer Linear Programming (MIP) algorithm to solve the initial placement that is modelled as a Quadratic Assignment Problem.
  The module can find a mapping that is optimal for the whole circuit,
  but because its time-complexity is exponential with respect to the size of the circuit, this may take quite some computer time.
  Also, the result is only really useful when in the mapping found all mapped operands of two-qubit gates are NN.
  So, there is no guarantee for success: it may take too long and the result may not be optimal.

- ``heuristic``
  This module essentially transforms each circuit in a linear scan over the circuit,
  from start to end, maintaining the ``v2r`` and ``rs`` maps.
  Each time that it encounters a two-qubit gate that in the current map is not NN,
  it inserts swap gates before this gate that make the operand qubits NN; when inserting a swap,
  it updates the ``v2r`` and ``rs`` maps accordingly.
  There are many refinements to this algorithm that can be controlled through options and the configuration file.
  The module will find the minimum number of swaps to make the mapped operands of each two-qubit gate NN in the mapping that applies just before it.
  In the most basic version, it has a linear time-complexity with respect to circuit size and number of qubits.
  With advanced search options set, the algorithm may become cubic with respect to number of qubits.
  So, it is still scalable and is guaranteed to find a solution.

The implementation is not complete:

- In the presence of multiple kernels with control flow among them,
  the ``v2r`` at the start of each kernel must match the ``v2r`` at the end of all predecessor kernels: this is not implemented.
  Instead, the ``v2r`` at the start of each kernel is re-initialized freshly, independently of the ``v2r`` at the end of predecessor kernels.
  The current implementation thus assumes that at the end of each kernel all qubits don't hold a state
  that must be preserved for a subsequent kernel.

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
  of the kernel to the number of real qubits as specified by the platform;
  this is done because in the mapped circuit the qubit operands of all gates will be real qubit indices
  of which the values should be in the range of the valid real qubit indices of the platform.

  Furthermore, some reporting of internal mapper statistics is done into attributes of the ``Mapper`` object.
  These can be retrieved by the caller of ``Map``:

  - ``nswapsadded``
    Number of swaps and moves inserted.

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
  

.. _mapping_input_and_output_intermediate_representation:

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

Options and Function
^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^

The options and corresponding function of the mapper are described.

The options include the proper mapper options and a subset of the scheduler options.
The subset of the scheduler options applies because the mapper uses the dependence graph created by the initialization method of the scheduler.
Also see :ref:`scheduling_options`.

Most if not all options can be combined to compose a favorite mapping strategy, i.e. the options are largely independent.

With the options, also the effects that they have on the function of the mapper are described.

The options and function are described in the order of their virtual encountering by a particular gate that is mapped.
Please remember that the heuristic essentially performs a linear scan over the gates of the circuit to map and transform the gates.

Initialization and configuration
""""""""""""""""""""""""""""""""""""

The ``Init`` method initializes the mapper for the given platform but independently of a particular kernel and circuit.
This includes sanity checking and initializing the mapper's representation
of the platform's topology from the platform's configuration file;
see :ref:`Configuration_file_definitions_for_mapper_control` for the description of the platform's topology.

The topology's edges define the neighborhood/connection map of the real qubits.
Floyd-Warshall is used to compute a distance matrix that contains for each real qubit pair the shortest distance between them.
This makes the mapper applicable to arbitrary formed connection graphs but at the same time less scalable in number of qubits.
For NISQ systems this is no problem.
For larger and more regular connection grids, the implementation contains a provision to replace this by a distance function.

Subsequently, ``Map`` is called for each kernel/circuit in the program.
It will attempt ``Initial Placement'' and then the ``heuristics``.
Before anything else, for each kernel again, the ``v2r`` and ``rs`` are initialized, each under control of an option:

- ``mapinitone2one:``
  Definition of the initialization of the ``v2r`` map at the start of the mapping of each kernel; this ``v2r`` will apply at the start of initial placement.

  - ``no:``
    there is no initial mapping of virtual to real qubits; each virtual qubit is allocated to the first free real qubit on the fly, when it is mapped

  - ``yes:``
    the initial mapping is 1 to 1: a virtual qubit with index ``qi`` is mapped to its real ``qi`` counterpart (so: same index)


- ``mapassumezeroinitstate:``
  Definition of the initialization of the ``rs`` map at the start of the mapping of each kernel; this ``rs`` will apply at the start of initial placement. Values can be: ``rs_nostate`` (no useful state), ``rs_wasinited`` (zero state), and ``rs_hasstate`` (useful but unknown state).

  - ``no:``
    each real qubit is assumed not to contain any useful state nor is it known that it is in a particular base state;
    this corresponds to the state with value ``rs_nostate``.

  - ``yes:``
    each real qubit is assumed to be in a zero state (e.g. ``|0>``) that allows a SWAP with it to be replaced by a (cheaper) MOVE;
    this corresponds to the state with value ``rs_wasinited``.

Then ``Initial Placement`` is started. See the start of :ref:`mapping` of a description of initial placement.
Since initial placement may take a lot of computer time, provisions have been implemented to time it out;
this comes in use during benchmark runs.
Initial placement is run under the control of two options:

- ``initialplace:``
  Definition of initial placement operation.
  Initial placement, when run, may be 100% successful (all two-qubit gates were made NN);
  be moderately successful (not all two-qubit gates were made NN, only some) or fail to find a solution:

  - ``no:``
    no initial placement is attempted

  - ``yes:``
    do initial placement starting from the initial ``v2r`` mapping; since initial placement employs an Integer Linear Programming model as the base of implementation, finding an initial placement may take quite a while.

  - ``1s, 10s, 1m, 10m, 1h:``
    do initial placement as with ``yes`` but limit execution time to the indicated maximum (one second, 10 seconds, one minute, etc.);
    when it is not successfull in this time, it fails, and subsequently the heuristics is started, which cannot fail.

  - ``1sx, 10sx, 1mx, 10mx, 1hx:``
    do initial placement as with ``yes`` but limit execution time to the indicated maximum (one second, 10 seconds, one minute, etc.);
    when it is not successfull in this time, it fails, and subsequently the compiler fails as well.

- ``initialplace2qhorizon:``
  The initial placement algorithm considers a number of initial two-qubit gates in the circuit to determine a mapping.
  This limits computer time but also may make a suboptimal result more useful.
  Option values are:

  - ``0, 10, 20, 30, 40, 50, 60, 70, 80, 90, 100:``
    The initial placement algorithm considers only this number of initial two-qubit gates in the circuit to determine a mapping.
    When ``0`` is specified as option value, there is no limit; this is the default.
    
This concludes ``Initial Placement``.
The ``v2r`` and ``rs`` at this time are stored in attributes for retrieval by the caller of the ``Map`` method.
See :ref:`mapping_input_and_output_intermediate_representation`.

Then the ``Heuristics`` start for the kernel given in the ``Map`` method call.
The mapper optionally uses the dependence graph representation of the circuit to enlarge
the number of alternatives it can consider, and to make use of the ``criticality`` of gates in the decision which one to map next.
To this end, it calls the scheduler's ``init`` method, and sets up the availability list of gates as set of gates
to choose from which one to map next: initially it contains just the ``SOURCE`` gates. See :ref:`scheduling`.
The scheduler listens to some options:

- ``scheduler_commute:``
  Because the mapper uses the dependence graph also generated for the scheduler,
  the alternatives made available by commutation of CZs/CNOTs are available to the mapper:

  - ``no:``
    don’t allow two-qubit gates to commute (CZ/CNOT) in the dependence graph;
    they are kept in original circuit order and presented to the mapper in this order

  - ``yes:``
    allow commutation of two-qubit CZ/CNOT gates;
    e.g. when one later one is already nearest-neighbor, allow it to be mapped before an earlier one which isn’t nearest-neighbor

- ``print_dot_graphs``
  When it has the value ``yes``, the mapper produces in the output directory
  in multiple files each with as name the name of the kernel followed by ``_mapper.dot``
  a ``dot`` representation of the dependence graph of the kernel's circuit at the start of the mapper heuristics,
  in which the gates are ordered along a timeline according to their cycle attribute.

With the dependence graph available to the mapper,
the availability list of it is used just as in the scheduler.
The list at each time contains those gates that can be mapped now.
Each time a gate has been mapped, the successor gates become available for being mapped,
and the availability list is updated.

This system is used to look-ahead, to find which two-qubit to map next, to make a selection from all that are available
or take just the most critical one, to try multiple ones and evaluate each alternative to map it, comparing those alternatives against
one of the metrics (see later), and even go into recursion, i.e. looking further ahead to see what the effects on subsequent two-qubit gates are when mapping the current one.

Deciding for the next two-qubit gate to map, is done based on the following options:

HERE

- ``maplookahead:``
  How does the mapper exploit the lookahead offered by the dependence graph constructed from the input circuit?

  - ``no:``
    the mapper ignores the dependence graph and takes the gates to be mapped one by one from the circuit

  - ``critical:``
    gates that by definition do not need routing, are mapped first; these include the classical gates, wait gates, and the single qubit quantum gates; and of the remaining (two qubit) quantum gates the most critical gate is selected first, i.e. the one behind which most cycles are expected until the end of the circuit

  - ``noroutingfirst:``
    those two qubit quantum gates of which the operands are neighbors in the current mapping are mapped first, also when these are not critical; and when none such are left, only then take the most critical one

  - ``all:``
    as with noroutingfirst but don't select the most critical one; instead, for all remaining (two qubit non-NN) gates generate alternatives and find the best from these according to the strategy above

- ``maprecNN2q:``

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

- ``mappathselect:``
  when generating alternatives of shortest paths between two real qubits:

  - ``all:``
    select all possible alternatives

  - ``borders:``
    only select those alternatives that correspond to following the borders of the rectangle spanning between the two extreme real qubits

- ``mapselectmaxlevel:``

- ``mapselectmaxwidth:``

- ``maptiebreak:``
  when multiple alternatives remain for a particular strategy with the same best evaluation value, decide how to select the best single one:

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
    do, when swapping with an ancillary qubit which is known to be in the initial state (``|+>`` for moves with 2 CNOTs); when not in the initial state, insert a ``move_init`` sequence (prepz followed by hadamard) when it doesn't additionally extend the circuit; when a ``move_init`` sequence would extend the circuit, don't insert the move

  - ``0, 1, 2, 3, 4, 5, 6, 7, 8, 9, 10, 11, 12, 13, 14, 15, 16, 17, 18, 19, 20:``
    yes, and insert a ``move_init`` sequence to get the ancillary qubit in the initial state, if needed; but only when the number of cycles of circuit extension that this ``move_init`` causes, is less-equal than 0, 1, ``...`` 20 cycles. Please note that later it was decided and implemented to assume that all real qubits start off in the initial state; this increases the likelihood that moves are inserted, and makes all these considerations of only inserting a move when a ``move_init`` can bring the ancillary qubit in the initial state somehow without additional circuit extension, of no use.

- ``mapprepinitstate:``

- ``mapselectswaps:``

- ``mapreverseswap:``
  reverse operand real qubits of swap when beneficial:

  - ``no:``
    don't

  - ``yes:``
    when scheduling a swap, exploiting the knowledge that the execution of a swap for one of the qubits starts one cycle later, a reversal of the real qubit operands might allow scheduling it one cycle earlier


- it starts the heuristics by selecting one or more gates from the input DAG/dependence graph (which is the same as the scheduler uses);

    - It reuses the dependence graph, including the commutation support for CNOT/CZs.

    - The list of available gates of the list scheduler is reused as set of gates to choose from to map next.

    - The mapper uses a simple ASAP scheduling policy to optimize interleaving of gates and to find the minimal extension of a set of swaps implementing the required mapping of a 2q gate.

    - It uses the resource manager to take resource constraints into account in the latter.

when a classical or single-qubit gate is encountered, it is mapped before any available two-qubit gate; when only two-qubit gates remain, prefer those that are already nearest neighbor (NN) in the current mapping; when then only non-NN two-qubit gates remain the currently best strategy is to take the one that is most critical in the remaining dependence graph (i.e. has the highest likelihood to extend the circuit when mapped in the wrong way or when delayed). But choosing an other option, all available gates in the dependence graph can be taken instead of only the most critical one. The following heuristics probably cannot beat taking the most critical one and need improvement.


- the heuristics select alternatives for all gates selected above; for each two-qubit gate it selects all shortest paths as alternatives and for each generates all alternatives of putting the two-qubit gate somewhere along the path; so always all alternatives have the least number of swaps/moves; optionally only the border paths are taken (when seeing the path end-point qubits as diagonal of a rectangle in the grid, the borders are the paths along the edges of this rectangle) are taken as initial alternatives.

- depending  on the mapper strategy, of these alternatives those are selected that minimally extend (in terms of cycles) the circuit without or with resource constraints taken into account; in this, there is alternative to use swap gates (3 CNOTs) or move gates (2 CNOTs) when one of the qubits is an ancillary; gates are scheduled ASAP in a representation of the already mapped gates to evaluate how much the additional set of swaps/moves extends the circuit to optimize interleaving of swaps mutually and interleaving of swaps and mapped quantum code sequences (i.e. improving the ILP and thereby reducing the resulting circuit's depth)

- in doing this, either the swaps/moves are inserted as primitives or their decompositions to CNOTs are inserted, or their decomposition to primitives are inserted; insertion of swaps/mores produces more readable result code; insertion of sequences of primitives results in more final scheduler opportunities, i.e. more exact/better scheduling

- when still multiple alternatives remain with best evaluation, a tiebreak selects which one is taken; for the taken one, the swaps are inserted, scheduled in and the mapping updated

- when all gates have been mapped, optionally all non-primitive gates can still be decomposed, and the result is subject to the final ALAP resource-constrained scheduler

- and finally all results and statistics are gathered and some of these also included in the output files as comment (depth of circuit, numbers of inserted swaps/moves, etc.)



..  _Configuration_file_definitions_for_mapper_control:

Configuration file definitions for mapper control
^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^

The configuration file contains the following sections that are recognized by the mapper:

- ``hardware_settings``
   the number of real qubits in the platform, and the cycle time in nanoseconds to convert instruction duration into cycles used by the various scheduling actions are taken from here

- ``instructions``
   the mapper assumes that the OpenQL circuit was read in and that gates were created according to the specifications of these in the configuration file: the name of each encountered gate is looked up in this section and, if not found, in the gate_decomposition section; if found, that gate (or those gates) are created; the duration field specifies the duration of each gate in nanoseconds; the type and various cc_light fields of each instruction are used as parameters to select applicable resource constraints in the resource-constrained scheduler

- ``gate_decomposition``
   when creating a gate matching an entry in this section, the set of gates specified by the decomposition description of the entry is created instead; the mapper exploits the decomposition support that the configuration file offers by this section in the following way:

   - ``reading the circuit``
     When a gate specified as a composite gate is created in an OpenQL program, its decomposition is created instead. So a CNOT in the OpenQL program but specified as two unary gate with a CZ in the middle, is input by the mapper as this latter sequence.

   - ``swap support``
     A swap is a composite gate, usually consisting of 3 CNOTs; those CNOTs usually are decomposed to a sequence of gates itself. The mapper supports generating swap as a primitive; or generating its shallow decomposition (e.g. to CNOTs); or generating its full decomposition (e.g. to the primitive gate set). The former leads to a more readable intermediate qasm file; the latter to more precise evaluation of the mapper selection criteria. Relying on the configuration file, when generating a swap, the mapper first attempts to create a gate with the name ``swap_real``, and when that fails, create a gate with the name ``swap``. The same machinery is used to create a move.

   - ``making gates real``
     Each gate input to the mapper is a virtual gate, defined to operate on virtual qubits. After mapping, the output gates are real gates, operating on real qubits. Making gates real is the translation from the former to the latter. This is usually done by replacing the virtual qubits by their corresponding real qubits. But support is provided to also replace the gate itself: when a gate is made real, the mapper first tries to create a gate with the same name but with ``_real`` appended to its name (and using the mapped, real qubits); if that fails, it keeps the original gate and uses that (with the mapped, real qubits) in the result circuit.

   - ``ancilliary initialization``
     For a move to be done instead of a swap, the target qubit must be in a particular state. For CC-LIGHT this is the ``|+>`` state. To support other target platforms, the ``move_init`` gate is defined to prepare a qubit in that state for the particular target platform. It decomposes to a PREPZ followed by a Hadamard for CC-LIGHT.

   - ``making all gates primitive``
     After mapping, the output gates will still have to undergo a final schedule with resource constraints before code can be generated for them. Best results are obtained when then all gates are primitive. The mapper supports a decomposition step to make that possible and this is typically used to decompose leftover swaps and moves to primitives: when a gate is made primitive, the mapper first tries to create a gate with the same name but with ``_prim`` appended to its name; if that fails, it keeps the original gate and uses that in the result circuit that is input to the scheduler.

- ``topology``
  A qubit grid's topology is defined by the neighbor relation among its qubits. Each qubit has an ``id`` (its index, used as a gate operand and in the resources descriptions) in the range of ``0`` to the number of qubits in the platform minus 1. Qubits are connected by directed pairs, called edges. Each edge has an ``id`` (its index, also used in the resources descriptions) in some contiguous range starting from ``0``, a source qubit and a destination qubit. Two grid forms are supported: the ``xy`` form and the ``irregular`` form. In grids of the ``xy`` form, there must be two additional attributes: ``x_size`` and ``y_size``, and the qubits have in addition an ``x`` and a ``y`` coordinate: these coordinates in the X (Y) direction are in the range of ``0`` to ``x_size-1`` (``y_size-1``).

- ``resources``
  See the scheduler's documentation.

