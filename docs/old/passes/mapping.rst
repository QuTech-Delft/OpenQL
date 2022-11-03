.. _mapping:

Mapping
-------

The circuits of all kernels are transformed
such that after mapping for any two-qubit gate the operand qubits are connected
(are NN, Nearest Neighbor) in the platform's topology;
this is done by a kernel-level initial placement and when it fails, by subsequent heuristic routing and mapping.
Both maintain a map from virtual (program) qubits to real qubits (``v2r``)
and a map from each real qubit index to its state (``rs``);
both are available after each of the two mapping subpasses.

- *initial placement*
  This module attempts to find a single mapping of the virtual qubits of a circuit to the real qubits (``v2r`` map)
  of the platform's qubit topology,
  that minimizes the sum of the distances between the two mapped operands of all two-qubit gates in the circuit.
  The distance between two real qubits is the minimum number of swaps that is required to move the state of one of the two qubits to the other.
  It employs a Mixed Integer Linear Programming (MIP) algorithm to solve the initial placement
  that is modelled as a Quadratic Assignment Problem.
  The module can find a mapping that is optimal for the whole circuit,
  but because its time-complexity is exponential with respect to the size of the circuit,
  this may take quite some computer time.
  Also, the result is only really useful when in the mapping found all mapped operands of two-qubit gates are NN.
  So, there is no guarantee for success: it may take too long and the result may not be optimal.

- *heuristic routing and mapping*
  This module essentially transforms each circuit in a linear scan over the circuit,
  from start to end, maintaining the ``v2r`` and ``rs`` maps.
  Each time that it encounters a two-qubit gate that in the current map is not NN,
  it inserts ``swap`` gates before this gate that make the operand qubits NN (this is called *routing* the qubits);
  when inserting a ``swap``, it updates the ``v2r`` and ``rs`` maps accordingly.
  There are many refinements to this algorithm that can be controlled through options and the configuration file.
  The module will find the minimum number of swaps to make the mapped operands of each two-qubit gate NN
  in the mapping that applies just before it.
  In the most basic version, it has a linear time-complexity with respect to circuit size and number of qubits.
  With advanced search options set, the algorithm may become cubic with respect to number of qubits.
  So, it is still scalable and is guaranteed to find a solution.

The implementation is not complete:

- In the presence of multiple kernels with control flow among them,
  the ``v2r`` at the start of each kernel must match the ``v2r`` at the end of all predecessor kernels:
  this is not implemented.
  Instead, the ``v2r`` at the start of each kernel is re-initialized freshly,
  independently of the ``v2r`` at the end of predecessor kernels.
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
  Perform mapping on the kernel, i.e. replace the kernel's circuit by an equivalent but *mapped* circuit.
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
    Number of ``swap``\ s and ``move``\ s inserted.

  - ``nmovesadded``
    Number of ``move``\ s inserted.

  - ``v2r_in``
    Vector with for each virtual qubit index its mapping to a real qubit index
    (or ``UNDEFINED_QUBIT`` represented by ``INT_MAX``,
    indicating that the virtual qubit index is not mapped to a real qubit),
    after initialization and before initial placement and/or heuristic routing and mapping.

  - ``rs_in``
    Vector with for each real qubit index its state.
    This vector shows the state after initialization of the mapper and before initial placement and/or heuristic routing and mapping.
    State values can be:
    
    - ``rs_nostate``:
      no statically known quantum state and no dynamically useful quantum state to preserve
      
    - ``rs_wasinited``:
      known to be in zero base state (``|0>``)

    - ``rs_hasstate``:
      useful but statically unknown quantum state; must be preserved
    
  - ``v2r_ip``
    Vector with for each virtual qubit index its mapping to a real qubit index
    (or ``UNDEFINED_QUBIT`` represented by ``INT_MAX``,
    indicating that the virtual qubit index is not mapped to a real qubit),
    after initial placement but before heuristic routing and mapping.

  - ``rs_ip``
    Vector with for each real qubit index its state (see ``rs_in`` above for the values),
    after initial placement but before heuristic routing and mapping.
    
  - ``v2r_out``
    Vector with for each virtual qubit index its mapping to a real qubit index
    (or ``UNDEFINED_QUBIT`` represented by ``INT_MAX``,
    indicating that the virtual qubit index is not mapped to a real qubit),
    after heuristic routing and mapping.

  - ``rs_out``
    Vector with for each real qubit index its state (see ``rs_in`` above for the values),
    after heuristic routing and mapping.
  

.. _mapping_input_and_output_intermediate_representation:

Input and output intermediate representation
^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^

The mapper expects kernels with or without a circuit.
When with a circuit, the ``cycle`` attributes of the gates need not be valid.
Gates that are supported on input are one-qubit ``measure``, no-operand ``display``, any classical gate,
``cnot``, ``cz``/``cphase``, and any other quantum and scheduling gate.
The mapper refuses multi-qubit quantum gates as input with more than two quantum operands.

The mapper produces a circuit with the same gates but then *mapped* (see below),
with the real qubit operands of two-qubit gates made nearest-neighbor in the platform's topology,
and with additional quantum gates inserted to implement the swapping or moving of qubit states.
The *mapping* of any (quantum, classical, etc.) gate
entails replacing the virtual qubit operand indices by the real qubit operand indices
corresponding to the mapping of virtual to real qubit indices applicable at the time of execution of the gate;
furthermore the gate itself (when a quantum gate) is optionally replaced at the time of its mapping
by one or more gates as specified by the platform's configuration file:
if the configuration file contains a definition for a gate with the name of the original gate with ``_real`` appended,
then that one is created and replaces the original gate.
Note that when this created gate is defined in the ``gate_decomposition`` section,
the net effect is that the specified decomposition is done.
When a ``swap`` or ``move`` gate is created to be inserted in the circuit, first a ``swap_real`` (or ``move_real``) is attempted
to be created instead before creating a ``swap`` or ``move``; this also allows the gate to be decomposed to more primitive
gates during mapping.

When a kernel's circuit has been mapped, an optional final decomposition of the mapped gates is done:
each gate is optionally replaced by one or more gates as specified by the platform's configuration file,
by creating a gate with the name of the original gate with ``_prim`` appended,
if defined in the configuration file, and replacing the original gate by it.
Note that when this created gate is specified in
the configuration file in the ``gate_decomposition`` section, the net effect is that the specified decomposition is done.
When in the mapped circuit, ``swap`` or ``move`` gates were inserted and ``swap_prim`` or ``move_prim`` are specified
in the configuration file, these are also used to replace the ``swap`` or ``move``  at this time.

The ``cycle`` attribute of each gate is assigned a valid value.
The gates in the circuit are ordered with non-decreasing cycle value.
The cycle values are consistent with the constraints that are imposed during mapping;
these are specified by the ``mapper`` option.

The above implies that non-quantum gates are accepted on input and are passed unchanged to output.

.. _mapping_options:

Options and Function
^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^

The options and corresponding function of the mapper are described.

The options include the proper mapper options and a few scheduler options.
The subset of the scheduler options
applies because the mapper uses the dependence graph created by the initialization method of the scheduler.
Also see :ref:`scheduling_options`.

Most if not all options can be combined to compose a favorite mapping strategy, i.e. the options are largely independent.

With the options, also the effects that they have on the function of the mapper are described.

The options and function are described in the order of their virtual encountering by a particular gate that is mapped.
Please remember that heuristic routing and mapping essentially performs a linear scan over the gates of the circuit
to route the qubits, map and transform the gates.

Initialization and configuration
%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%

The ``Init`` method initializes the mapper for the given platform but independently of a particular kernel and circuit.
This includes sanity checking and initializing the mapper's representation
of the platform's topology from the platform's configuration file;
see :ref:`Configuration_file_definitions_for_mapper_control` for the description of the platform's topology.

The topology's edges define the neighborhood/connection map of the real qubits.
Floyd-Warshall is used to compute a distance matrix
that contains for each real qubit pair the shortest distance between them.
This makes the mapper applicable to arbitrary formed connection graphs
but at the same time less scalable in number of qubits.
For NISQ systems this is no problem.
For larger and more regular connection grids,
the implementation contains a provision to replace this by a distance function.

Subsequently, ``Map`` is called for each kernel/circuit in the program.
It will attempt initial placement and then heuristic routing and mapping.
Before anything else, for each kernel again, the ``v2r`` and ``rs`` are initialized, each under control of an option:

- ``mapinitone2one``:
  Definition of the initialization of the ``v2r`` map at the start of the mapping of each kernel;
  this ``v2r`` will apply at the start of initial placement.

  - ``no``:
    there is no initial mapping of virtual to real qubits;
    each virtual qubit is allocated to the first free real qubit on the fly, when it is mapped

  - ``yes`` (default for back-ward compatibility):
    the initial mapping is 1 to 1:
    a virtual qubit with index ``qi`` is mapped to its real ``qi`` counterpart (so: same index)


- ``mapassumezeroinitstate``:
  Definition of the initialization of the ``rs`` map at the start of the mapping of each kernel;
  this ``rs`` will apply at the start of initial placement.
  Values can be: ``rs_nostate`` (no useful state), ``rs_wasinited`` (zero state),
  and ``rs_hasstate`` (useful but unknown state).

  - ``no`` (default for back-ward compatibility):
    each real qubit is assumed not to contain any useful state nor is it known that it is in a particular base state;
    this corresponds to the state with value ``rs_nostate``.

  - ``yes`` (best):
    each real qubit is assumed to be in a zero state (e.g. ``|0>``)
    that allows a ``swap`` with it to be replaced by a (cheaper) ``move``;
    this corresponds to the state with value ``rs_wasinited``.

Initial Placement
%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%

After initialization and configuration, initial placement is started.
See the start of :ref:`mapping` of a description of initial placement.
Since initial placement may take a lot of computer time, provisions have been implemented to time it out;
this comes in use during benchmark runs.
Initial placement is run under the control of two options:

- ``initialplace``:
  Definition of initial placement operation.
  Initial placement, when run, may be 100% successful (all two-qubit gates were made NN);
  be moderately successful (not all two-qubit gates were made NN, only some) or fail to find a solution:

  - ``no`` (default):
    no initial placement is attempted

  - ``yes`` (best, optimal result):
    do initial placement starting from the initial ``v2r`` mapping;
    since initial placement employs an Integer Linear Programming model as the base of implementation,
    finding an initial placement may take quite a while.

  - ``1s, 10s, 1m, 10m, 1h`` (best, limit time, still a result):
    put a soft time limit on the execution time of initial placement;
    do initial placement as with ``yes``
    but limit execution time to the indicated maximum (one second, 10 seconds, one minute, etc.);
    when it is not successfull in this time, it fails, and subsequently heuristic routing and mapping is started, which cannot fail.

  - ``1sx, 10sx, 1mx, 10mx, 1hx``:
    put a hard time limit on the execution time of initial placement;
    do initial placement as with ``yes``
    but limit execution time to the indicated maximum (one second, 10 seconds, one minute, etc.);
    when it is not successfull in this time, it fails, and subsequently the compiler fails as well.

- ``initialplace2qhorizon``:
  The initial placement algorithm considers only a specified
  number of two-qubit gates from the start of the circuit (a ``horizon``) to determine a mapping.
  This limits computer time but also may make a suboptimal result more useful.
  Option values are:

  - ``0`` (default, optimal result):
    When ``0`` is specified as option value, there is no limit; all two-qubit gates of the circuit are taken into account.
    
  - ``10, 20, 30, 40, 50, 60, 70, 80, 90, 100``:
    The initial placement algorithm considers only this number of initial two-qubit gates in the circuit
    to determine a mapping.
    
Best result would be obtained by running initial placement optionally twice (this is not implemented):

- Once with a modified model in which only the result with all two-qubit gates NN is successful.
  When it succeeds, mapping has completed.
  Depending on the resources one wants to spend on this, a soft time limit could be set.

- Otherwise, attempt to get a good starting mapping by running initial placement
  with a soft time limit (of e.g. 1 minute) and with a two-qubit horizon (of e.g. 10 to 20 gates).
  What ever the result is, run heuristic routing and mapping afterwards.

This concludes initial placement.
The ``v2r`` and ``rs`` at this time are stored in attributes for retrieval by the caller of the ``Map`` method.
See :ref:`mapping_input_and_output_intermediate_representation`.

Heuristic Routing and Mapping
%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%

Subsequently heuristic routing and mapping starts for the kernel given in the ``Map`` method call.

- The scheduler's dependence graph is used to feed heuristic routing and mapping with gates to map and to look-ahead:
  see :ref:`mapping_dependence_graph`.

- To map a non-NN two-qubit gate, various routing alternatives, to be implemented by ``swap``/``move`` sequences, are generated:
  see :ref:`mapping_generating_routing_alternatives`.

- Depending on the metric chosen, the alternatives are evaluated:
  see :ref:`mapping_comparing_alternatives`.

- When minimizing circuit latency extension, ILP is maximized by maintaining a scheduled circuit representation:
  see :ref:`mapping_look_back`.

- Looking farther ahead beyond the mapping of the current two-qubit gate,
  the router recurses considering the effects of its mapping on subsequent two-qubit gates:
  see :ref:`mapping_looking_farther_ahead`.

- Finally, the evaluations of the alternatives are compared,
  the best one selected and the two-qubit gate routed and mapped:
  see :ref:`mapping_deciding_for_the_best`.

.. _mapping_dependence_graph:

Dependence Graph and Look-Ahead, Which Gate(s) To Map Next
%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%

The mapper optionally uses the dependence graph representation of the circuit to enlarge
the number of alternatives it can consider,
and to make use of the *criticality* of gates in the decision which one to map next.
To this end, it calls the scheduler's ``init`` method, and sets up the availability list of gates as set of gates
to choose from which one to map next: initially it contains just the ``SOURCE`` gates.
See :ref:`scheduling`, and below for more information on the availability list's properties.
The mapper listens to the following scheduler options:

- ``scheduler_commute``:
  Because the mapper uses the dependence graph that is also generated for the scheduler,
  the alternatives that are made available by commutation of ``cz``\ s/``cnot``\ s, can be made available to the mapper:

  - ``no`` (default for backward-compatibility):
    don’t allow two-qubit gates to commute (``cz``/``cnot``) in the dependence graph;
    they are kept in original circuit order and presented to the mapper in this order

  - ``yes`` (best):
    allow commutation of two-qubit ``cz``/``cnot`` gates;
    e.g. when one isn't nearest-neighbor
    but one that comes later in the circuit but commutes  with the earlier one is NN now,
    allow the later one to be mapped before the earlier one

- ``print_dot_graphs``:
  When it has the value ``yes``, the mapper produces in the output directory
  in multiple files each with as name the name of the kernel followed by ``_mapper.dot``
  a ``dot`` representation of the dependence graph of the kernel's circuit at the start of heuristic routing and mapping,
  in which the gates are ordered along a timeline according to their cycle attribute.

With the dependence graph available to the mapper,
its availability list is used just as in the scheduler:

- the list at each moment contains those gates that have not been mapped but can be mapped now

- the availability list forms a kind of *cut* of the dependence graph:
  all predecessors of the gates in the list and recursively all their predecessors have been mapped,
  all other gates have not been mapped (the *cut* is really the set of dependences between
  the set of mapped and the set of non-mapped gates)

- each moment a gate has been mapped, it is taken out of the availability list;
  those of its successor dependence gates of which all predecessors have been mapped,
  become available for being mapped, i.e. are added to the availability list

This dependence graph is used to look-ahead,
to find which two-qubit to map next, to make a selection from all that are available
or take just the most critical one,
to try multiple ones and qubit_nr each alternative to map it, comparing those alternatives against
one of the metrics (see later), and even go into recursion (see later as well),
i.e. looking farther ahead to see what the effects on subsequent two-qubit gates are when mapping the current one.

In this context the *criticality* of a gate is an important property of a gate:
the *criticality* of a gate is the length of the longest dependence path from the gate to the ``SINK`` gate
and is computed in a single linear backward scan over the dependence graph (Dijkstra's algorithm).

Deciding for the next two-qubit gate to map, is done based on the following option:

- ``maplookahead``:
  How does the mapper exploit the lookahead offered by the dependence graph constructed from the input circuit?

  - ``no``:
    the mapper ignores the dependence graph and takes the gates to be mapped one by one from the input circuit

  - ``critical``:
    gates that by definition do not need routing, are mapped first (and kind of flushed):
    these include the classical gates, scheduling gates (such as ``wait``), and the single qubit quantum gates;
    and of the remaining (only two qubit) quantum gates
    the most critical gate is selected first to be routed and mapped next;
    the rationale of taking the most critical gate is
    that after that one the most cycles are expected until the end of the circuit,
    and so a wrong routing decision of a critical gate is likely to have most effect on the mapped circuit's latency;
    so criticality has higher priority to select the one to be mapped next,
    than NN (see ``noroutingfirst`` for the opposite approach)

  - ``noroutingfirst`` (default, best):
    gates that by definition do not need routing, are mapped first (and kind of flushed):
    these include the classical gates, scheduling gates (such as ``wait``), and the single qubit quantum gates;
    in this, this ``noroutingfirst`` option has the same effect as ``critical``;
    but those two qubit quantum gates of which the operands are neighbors in the current mapping
    are selected to be mapped first,
    not needing routing, also when these are not critical;
    and when none such are left, only then take the most critical one;
    so NN has higher priority to select the one to be mapped next, than criticality

  - ``all`` (promising in combination with recursion):
    as with ``noroutingfirst`` but don't select the most critical one, select them all;
    so at each moment gates that do not need routing, are mapped first (and kind of flushed);
    these thus include the NN two-qubit gates;
    this mapping and flushing stops when only non-NN two-qubit gates remain;
    instead of selecting one of these to be routed/mapped next, all of these are selected, the decision is postponed;
    i.e. for all remaining (two qubit non-NN) gates generate alternatives
    and find the best from these according to the chosen metric
    (see the ``mapper`` option below); and then select that best one to route/map next

.. _mapping_generating_routing_alternatives:

Generating Routing Alternatives
%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%

Having selected one (or more) two-qubit gates to map next, for each two-qubit gate the routing alternatives are explored.
Subsequently, those alternatives will be compared using the selected metric and the best one selected; see further below.

But first the routing alternatives have to be generated.
When the mapped operands of a two-qubit gate are not NN, they must be made NN by swapping/moving one or both
over nearest-neighbor connections in the target platform's grid topology towards each other.
Only then the two-qubit gate can be executed;
the mapper will insert those ``swap``\ s and ``move``\ s before the two-qubit gate in the circuit.

There are usually many routes between the qubits.
The current implementation only selects the ones with the shortest distance, and these can still be many.
In a perfectly rectangular grid,
the number of routes is similar to a Fibonaci number depending on the distance decomposed in the x and y directions,
and is maximal when the distances in the x and y directions are equal.
All shortest paths between two qubits in such a grid stay within
a rectangle in the grid with the mapped qubit operands at opposite sides of the diagonal.

A shortest distance leads to a minimal number of ``swap``\ s and ``move``\ s.
For each route between qubits at a distance *d*,
there are furthermore *d* possible places in the route where to do the two-qubit gate;
the other *d-1* places in the route will be a ``swap`` or a ``move``.

The implementation supports an arbitrarily formed connection graph, so not only a rectangular grid.
All that matter are the distances between the qubits.
Those have been computed using Floyd-Warshall from the qubit neighbor relations during initialization of the mapper.
The shortests paths are generated in a brute-force way by only navigating to those neighbor qubits
that will not make the total end-to-end distance longer.
Unlike other implementations that only minimize the number of swaps and for which the routing details are irrelevant,
this implementation explicitly generates all alternative paths to allow the more complicated metrics that are supported,
to be computed.

The generation of those alternatives is controlled by the following option:

- ``mappathselect``:
  When generating alternatives of shortest paths between two real qubits:

  - ``all`` (default, best):
    select all possible alternatives:
    those following all possible shortest paths and in each path each possible placement of the two-qubit gate

  - ``borders``:
    only select those alternatives
    that correspond to following the borders of the rectangle spanning between the two extreme real qubits;
    so on top of the at most two paths along the borders, there still are all alternatives of
    the possible placements of the two-qubit gate along each path

It is thus not supported to turn off to generate alternatives
for the possible placements of the two-qubit gate along each path.

The alternatives are ordered; this is relevant for the ``maptiebreak`` option below.
The alternatives are ordered:

- first by the *two-qubit gate* for which they are an alternative; the most critical two-qubit gate is first;
  remember that there can be more than one two-qubit gate when ``all`` was selected for the ``maplookahead`` option.

- then by the *followed path*; each path is represented by
  a sequence of transitions from the mapped first operand qubit to the mapped second operand qubit.
  The paths are ordered such that of any set of paths with a common prefix
  these are ordered by a clock-wise order of the successor qubits as seen from the last qubit of the common prefix.

- and then by the *placement* of the two-qubit gate; the placements are ordered from start to end of the path.

So, the first alternative will be the one that clock-wise follows the border and has the two-qubit gate placed
directly at the qubit that is the mapped first operand of the gate;
the last alternative will be the one that anti-clock-wise follows the border and has the two-qubit gate placed
directly at the qubit that is the mapped last operand of the gate.

.. _mapping_comparing_alternatives:

Comparing Alternatives, Which Metric To Use
%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%

With all alternatives available, it is time to compare them using the defined metric.
The metric to use is defined by the ``strategy`` option, called for historic reasons ``mapper``.
What needs to be done when multiple alternatives compare equal, is specified later.

- ``mapper``:
  The basic mapper strategy (metric of mapper result optimization) that is employed:

  - ``no`` (default for back-ward compatibility):
    no mapping is done. The output circuit is identical to the input circuit.

  - ``base``:
    map the circuit:
    use as metric just the length of the paths between the mapped operands of each two-qubit gate,
    and minimize this length for each two-qubit gate that is mapped;
    with only alternatives for one two-qubit gate, all alternatives have the same shortest path,
    so all alternatives qualify equally;
    with alternatives for multiple two-qubit gates, those two-qubit gates
    are preferred that lead to the least ``swap``\ s and ``move``\ s.

  - ``minextend`` (best):
    map the circuit:
    use as metric the extension of the circuit by each of the shortest paths
    between the mapped operands of each two-qubit gate,
    and minimize this circuit extension by evaluating all alternatives;
    the computation of the extension relies on scheduling-in the required swaps and moves in the circuit
    and just subtracting the depths before and after doing that;
    the various options controlling this scheduling-in, will be specified later below.

  - ``minextendrc``:
    map the circuit:
    as in ``minextend``, but taking resource constraints into account when scheduling-in the ``swap``\ s and ``move``\ s.

.. _mapping_look_back:

Look-Back, Maximize Instruction-Level Parallelism By Scheduling
%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%

To know the circuit's latency extension of an alternative,
the mapped gates are represented as a scheduled circuit, i.e. with gates with a defined ``cycle`` attribute,
and the gates ordered in the circuit with non-decreasing ``cycle`` value.
In case the ``mapper`` option has the ``minextendrc`` value, also the state of all resources is maintained.
When a ``swap`` or ``move`` gate is added, it is ASAP scheduled (optionally taking the resource constraints into account)
into the circuit and the corresponding cycle value is assigned to the ``cycle`` attribute of the added gate.
Note that when ``swap`` or ``move`` is defined by a composite gate, the decomposed sequence is scheduled-in instead.

The objective of this is to maximize the parallel execution of gates and especially of ``swap``\ s and ``move``\ s.
Indeed, the smaller the latency extension of a circuit, the more parallelism was created,
i.e. the more the ILP was enlarged.
When ``swap``\ s and ``move``\ s are not inserted as primitive gates
but the equivalent decomposed sequences are inserted, ILP will be improved even more.

This scheduling-in is done separately for each alternative: for each alternative,
the ``swap``\ s or ``move``\ s are added
and the end-result evaluated.

This scheduling-in is controlled by the following options:

- ``mapusemoves``:
  Use ``move`` instead of ``swap`` where possible.
  In the current implementation, a ``move``
  is implemented as a sequence of two ``cnot``\ s
  while a ``swap`` is implemented
  as a sequence of three ``cnot``\ s.

  - ``no``:
    don't

  - ``yes`` (default, best):
    do, when swapping with an ancillary qubit which is known to be in the zero state (``|0>``
    for moves with 2 ``cnot``\ s);
    when not in the initial state,
    insert a ``move_init`` sequence (when defined in the configuration file, the defined sequence,
    otherwise a prepz followed by a hadamard) when it doesn't additionally extend the circuit;
    when a ``move_init`` sequence would extend the circuit, don't insert the ``move``

  - ``0, 1, 2, 3, 4, 5, 6, 7, 8, 9, 10, 11, 12, 13, 14, 15, 16, 17, 18, 19, 20``:
    yes, and insert a ``move_init`` sequence to get the ancillary qubit in the initial state, if needed;
    but only when the number of cycles of circuit extension that this ``move_init`` causes,
    is less-equal than 0, 1, ``...`` 20 cycles.

    Please note that the ``mapassumezeroinitstate`` option defines whether the implementation of the mapper
    can assume that each qubit starts off in the initial state;
    this increases the likelihood that moves are inserted,
    and makes all these considerations of only inserting a ``move``
    when a ``move_init`` can bring the ancillary qubit in the initial state somehow
    without additional circuit extension, of no use.

- ``mapprepinitsstate``:
  Does a ``prepz`` initialize the state, i.e. leave the state of a qubit in the ``|0>`` state?
  When so, this can be reflected in the ``rs`` map.

  - ``no`` (default, playing safe):
    no, it doesn't; a ``prepz`` during mapping will, as any other quantum gate,
    set the state of the operand qubits to ``rs_hasstate`` in the ``rs`` map

  - ``yes`` (best):
    a ``prepz`` during mapping will set the state of the operand qubits to ``rs_wasinited``;
    any other gate will set the state of the operand qubits to ``rs_hasstate``

- ``mapselectswaps``:
  When scheduling-in ``swap``\ s
  and ``move``\ s at the end for the best alternative found,
  this option selects that potentially not all required ``swap``\ s
  and ``move``\ s are inserted.
  When not all are inserted but only one, the distance of the mapped operand qubits of the two-qubit gate
  for which the best alternative was generated, will be one less, and after insertion
  heuristic routing and mapping starts over generating alternatives for the new situation.

  Please note that during evaluation of the alternatives, all ``swap``\ s
  and ``move``\ s are inserted.
  So the alternatives are compared with all ``swap``\ s
  and ``move``\ s inserted
  but only during the final real insertion after having selected the best alternative, just one is inserted.

  - ``all`` (best, default):
    insert all ``swap``\ s
    and ``move``\ s as usual

  - ``one``:
    insert only one ``swap`` or ``move``; take the one swapping/moving the mapped first operand qubit

  - ``earliest``:
    insert only one ``swap`` or ``move``; take the one that can be scheduled earliest
    from the one swapping/moving the mapped first operand qubit
    and the one swapping/moving the mapped second operand qubit

- ``mapreverseswap``:
  Since ``swap`` is symmetrical in effect (the states of the qubits are exchanged)
  but not in implementation (the gates on the second operand start one cycle earlier and end one cycle later),
  interchanging the operands may cause a ``swap`` to be scheduled at different cycles.
  Reverse operand real qubits of ``swap`` when beneficial:

  - ``no``:
    don't

  - ``yes`` (best, default):
    when scheduling a ``swap``,
    exploiting the knowledge that the execution of a ``swap`` for one of the qubits starts one cycle later,
    a reversal of the real qubit operands might allow scheduling it one cycle earlier


.. _mapping_looking_farther_ahead:

Looking Farther Ahead, Recurse To Find Best Alternative
%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%

Looking farther ahead beyond the mapping of the current two-qubit gate,
the router recurses considering the effects of its mapping on subsequent two-qubit gates.

After having evaluated the metric for each alternative, multiple alternatives may remain, all with the best value.
For the ``minextend`` and ``minextendrc`` strategies, there are options to select from these by looking ahead farther,
i.e. beyond the metric evaluation of this alternative for mapping one two-qubit gate.
This *recursion* assumes that the current alternative is selected, its ``swap``\ s
and ``move``\ s are added to the circuit
the ``v2r`` map is updated, and the availability set is updated.
And then in this new situation the implementation recurses
by selecting one or more two-qubit gates to map next, generating alternatives, evaluating these alternatives
against the metric, and deciding which alternatives are the best.
This recursion can go deeper and deeper until a particular depth has been reached.
Then of the resulting tree of alternatives, for all the leaves representing the deepest alternatives,
the metric is computed from the root to the leaf and compared to each other.
In this way suboptimalities of individual choices can be balanced to a more optimal combination.
From these leaves, the best is taken; when multiple alternatives compare equally well from root to leaf,
the ``maptiebreak`` option decides which one to take, as usual; see below there.

The following options control this recursion:

- ``mapselectmaxlevel``:
  Looking farther ahead beyond the mapping of the current two-qubit gate,
  the router recurses considering the effects of its mapping on subsequent two-qubit gates.
  The level specifies the recursion depth: how many two-qubits in a row are considered beyond the current one.
  This generates a tree of alternatives.

  - ``0`` (default, back-ward compatible):
    no recursion is done

  - ``1, 2, 3, 4, 5, 6, 7, 8, 9, 10``:
    the indicated number of recursions is done;
    initial experiments show that a value of ``3`` produces reasonable results,
    and that recursion depth of ``5`` and higher are infeasible because of resource demand explosion

  - ``inf``:
    there is no limit to the number of recursions;
    this makes the resource demand of heuristic routing and mapping explode

- ``mapselectmaxwidth``:
  Not all alternatives are equally promising, so only some best are selected to recurse on.
  The width specifies the recursion width: for how many alternatives the recursion is actually done.
  The specification of the width is done relative to the number of alternatives
  that came out as best at the current recursion level.
  
  - ``min`` (default):
    only recurse on those alternatives that came out as best at this point

  - ``minplusone``:
    only recurse on those alternatives that came out as best at this point, plus one second-best

  - ``minplushalfmin`` (best combination of optimality and resources:
    only recurse on those alternatives that came out as best at this point, plus some number of second-bests:
    half the number more than the number of best ones

  - ``minplusmin``:
    only recurse on those alternatives that came out as best at this point, plus some number of second-bests:
    twice the number of best ones

  - ``all``:
    don't put a limit on the recursion width

- ``maprecNN2q``:
  In ``maplookahead`` with value ``all``, as with ``noroutingfirst``, two-qubit gates which are already NN,
  are immediately mapped, kind of flushing them.
  However, in recursion this creates an imbalance:
  at each level optionally several more than just one two-qubit gate are mapped and this makes the results of
  the alternatives largely incomparable.
  Comparision would be easier to understand when at each level only one two-qubit gate would be mapped.
  This option specifies independently of the ``maplookahead`` option that is chosen and that is applied before
  going into recursion, whether in the recursion this immediate mapping/flushing of NN two-qubit gates is done.
  
  - ``no`` (default, best):
    no, NN two-qubit gates are not immediately mapped and flushed until only non-NN two-qubit gates remain;
    at each recursion level exactly one two-qubit gate is mapped

  - ``yes``:
    yes, NN two-qubit gates are immediately mapped and flushed until only non-NN two-qubit gates remain;
    this makes recursion more greedy but makes interpreting the evaluations of the alternatives harder

.. _mapping_deciding_for_the_best:

Deciding For The Best, Committing To The Best
%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%

With or without recursion, for the ``base`` strategy as well as for the ``minextend`` and ``minextendrc`` strategies,
when at the end multiple alternatives still compare equally well, a decision has to be taken which two-qubit gate
to route and map.
This selection is made based on the value of the following option:

- ``maptiebreak``:
  When multiple alternatives remain for a particular strategy with the same best evaluation value,
  decide how to select the best single one:

  - ``first``:
    select the first of the set

  - ``last``:
    select the last of the set

  - ``random`` (default, best, non-deterministic):
    select in a random way from the set;
    when testing and comparing mapping strategies, this option introduces non-determinism and non-reproducibility,
    which precludes reasoning about the strategies unless many samples are taken and statistically analyzed

  - ``critical`` (deterministic, second best):
    select the first of the alternatives generated for the most critical two-qubit gate (when there were more)

Having selected a single best alternative, the decision has been made to route and map its corresponding two-qubit gate.
This means, scheduling in the result circuit the ``swap``\ s
and ``move``\ s that route the mapped operand qubits,
updating the ``v2r`` and ``rs`` maps on the fly; 
see :ref:`mapping_look_back` for the details of this scheduling.
And then map the two-qubit gate;
see :ref:`mapping_input_and_output_intermediate_representation` for what mapping involves.

After this, in the dependence graph a next gate is looked for to map next
and heuristic routing and mapping starts over again.

..  _Configuration_file_definitions_for_mapper_control:

Configuration file definitions for mapper control
^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^

The configuration file contains the following sections that are recognized by the mapper:

- ``hardware_settings``
   the number of real qubits in the platform,
   and the cycle time in nanoseconds to convert instruction duration
   into cycles used by the various scheduling actions are taken from here

- ``instructions``
   the mapper assumes that the OpenQL circuit was read
   in and that gates were created according to the specifications of these in the configuration file:
   the name of each encountered gate is looked up in this section and,
   if not found,
   in the ``gate_decomposition`` section;
   if found,
   that gate (or those gates) are created;
   the ``duration`` field specifies the duration of each gate in nanoseconds;
   the ``type`` and various ``cc_light`` fields of each instruction
   are used as parameters to select applicable resource constraints in the resource-constrained scheduler

- ``gate_decomposition``
   when creating a gate matching an entry in this section,
   the set of gates specified by the decomposition description of the entry is created instead;
   the mapper exploits the decomposition support
   that the configuration file offers by this section in the following way:

   - *reading the circuit*
     When a gate specified as a composite gate is created in an OpenQL program,
     its decomposition is created instead.
     So a ``cnot`` in the OpenQL program that is specified in the ``gate_decomposition``
     section as e.g. two ``hadamard``\ s with a ``cz`` in the middle,
     is input by the mapper as this latter sequence.

   - *swap support*
     A ``swap`` is a composite gate,
     usually consisting of 3 ``cnot``\ s;
     those ``cnot``\ s usually are decomposed to a sequence of primitive gates itself.
     The mapper supports generating ``swap`` as a primitive;
     or generating its shallow decomposition (e.g.  to ``cnot``\ s);
     or generating its full decomposition (e.g.  to the primitive gate set).
     The former leads to a more readable intermediate qasm file;
     the latter to more precise evaluation of the mapper selection criteria.
     Relying on the configuration file, when generating a ``swap``,
     the mapper first attempts to create a gate with the name ``swap_real``,
     and when that fails, create a gate with the name ``swap``.
     The same machinery is used to create a ``move``.

   - *making gates real*
     Each gate input to the mapper is a virtual gate,
     defined to operate on virtual qubits.
     After mapping, the output gates are real gates, operating on real qubits.
     *Making gates real* is the translation from the former to the latter.
     This is usually done by replacing the virtual qubits by their corresponding real qubits.
     But support is provided to also replace the gate itself:
     when a gate is made real, the mapper first tries to create a gate with the same name 
     but with ``_real`` appended to its name (and using the mapped, real qubits);
     if that fails, it keeps the original gate and uses that (with the mapped,
     real qubits) in the result circuit.

   - *ancilliary initialization*
     For a ``move`` to be done instead of a ``swap``, the target qubit must be in a particular state.
     For CC-Light this is the ``|+>`` state.  To support other target platforms,
     the ``move_init`` gate is defined to prepare a qubit in that state for the particular target platform.
     It decomposes to a ``prepz`` followed by a ``Hadamard`` for CC-Light.

   - *making all gates primitive*
     After mapping, the output gates will still have to undergo a final schedule 
     with resource constraints before code can be generated for them.
     Best results are obtained when then all gates are primitive.
     The mapper supports a decomposition step 
     to make that possible and this is typically used to decompose leftover ``swap``\ s
     and ``move``\ s to primitives:
     when a gate is made primitive,
     the mapper first tries to create a gate with the same name but with ``_prim`` appended to its name;
     if that fails,
     it keeps the original gate and uses that in the result circuit that is input to the scheduler.

- ``topology``
  A qubit grid's topology is defined by the neighbor relation among its qubits.
  Each qubit has an ``id`` (its index, used as a gate operand and in the resources descriptions) 
  in the range of ``0`` to the number of qubits in the platform minus 1.
  Qubits are connected by directed pairs, called *edge*\ s.
  Each edge has an ``id`` (its index,
  also used in the resources descriptions) in some contiguous range starting from ``0``,
  a source qubit and a destination qubit.
  Two grid forms are supported:
  the ``xy`` form and the ``irregular`` form.
  In grids of the ``xy`` form,
  there must be two additional attributes:
  ``x_size`` and ``y_size``,
  and the qubits have in addition an X and a Y coordinate:
  these coordinates in the X (Y) direction are in the range of ``0`` to ``x_size-1`` (``y_size-1``).

- ``resources``
  See the scheduler's documentation.

