.. _scheduling:

Scheduling
----------

Of each kernel's circuit the gates are scheduled at a particular cycle starting from 0
(by filling in the gate's ``cycle`` attribute) that matches the gates' dependences, their duration,
the constraints imposed by their resource use, the buffer values defined for the platform, and
the latency value defined for each gate; multiple gates may start in the same cycle;
in the resulting circuits (which are vectors of pointers to gate) the gates are ordered by their cycle value.
The schedulers also produce a ``bundled`` version of each circuit;
see :ref:`circuits_and_bundles_in_the_internal_representation`.

The resource-constrained and non-constrained versions of the scheduler have different entry points (currently).
The latter only considers the gates' dependences and their duration, which is sufficient as input to QX.
Next to the above necessary constraints, the remaining freedom is defined by a scheduling strategy
which is defined by the ``scheduler`` option value: ``ASAP``, ``ALAP`` and some other options.


.. _scheduling_entry_points:

Entry points
^^^^^^^^^^^^

The following two entry points are supported,
one for the non-constrained and one for the resource-constrained scheduler:

- ``p.schedule()``
  In the context of ``program`` object ``p``, this method schedules the circuits of the kernels of the program,
  according to a strategy specified by the scheduling options,
  but without taking resource constraints, buffers and latency compensation of the platform into account.

- ``bundles = cc_light_schedule_rc(circuit, platform, num_qubits, num_creg)``
  In the context of the ``cc_light_eqasm_compiler``, a derived class of the ``eqasm_compiler`` class,
  in its ``compile(prog_name, kernels, platform)`` method, inside a loop over the specified kernels,
  the resource-constrained scheduler is called to schedule the specified circuit,
  according to a strategy specified by the scheduling options,
  and taking resource constraints, buffers and latency compensation of the platform into account.
  It creates a bundled version of the IR and returns it.

:Note: These entry points need to be harmonized to fit in the generalized pass model: same class, program-level interface, no result except in IR, buffer and latency compensation split off to separate passes.

The above entry points each create a ``sched``  object of class ``Scheduler`` and call a selection of its methods:

- ``sched.init(circuit, platform, num_qubits, num_creg)``
  A dependence graph representation of the specified circuit is constructed.
  This graph is a Directed Acyclic Graph (DAG).
  In this graph, the nodes represent the gates and the directed edges the dependences.
  The top of the graph is a newly created SOURCE gate, the bottom is a newly created SINK gate.
  With respect to dependences,
  the SOURCE and SINK gates behave as if they update all qubits and classical registers with 0 duration.
  Gates are added in the order of presence in the circuit
  and linked in dependence chains according to their operation and operands.

  The nodes have as attributes (apart from the gate's attributes):

  - ``name`` with the qasm string representation of the gate (such as ``cnot q[1],q[2]``)

  The edges have as attributes:

  - ``weight`` representing the number of cycles needed from the start of execution of the gate at the source of the edge, to the start of execution of the gate at the target of the edge; this value is initialized from the ``duration`` attribute of the gate

  - ``cause`` representing the qubit or classical register causing the dependence

  - ``depType`` representing the type of the dependence

  The latter two attributes are currently only used internally in the dependence graph construction.

  This ``sched.init`` method is called by both entry points for each circuit of the program.

- ``bundles = sched.schedule_asap(sched_dot)``
  The cycle attributes of the gates are initialized consistent with an ASAP (i.e. downward) walk over the dependence graph.
  Subsequently, the gates in the circuit are sorted by their cycle value;
  and the ``bundler`` called to produce a bundled version of the IR to return.

  This method is called by ``p.schedule()`` for each circuit of the program when non-uniform ASAP scheduling.

- ``bundles = sched.schedule_alap(sched_dot)``
  The cycle attributes of the gates are initialized consistent with an ALAP (i.e. upward) walk over the dependence graph.
  Subsequently, the gates in the circuit are sorted by their cycle value;
  and the ``bundler`` called to produce a bundled version of the IR to return.

  This method is called by ``p.schedule()`` for each circuit of the program when non-uniform ALAP scheduling.

- ``bundles = sched.schedule_alap_uniform()``
  The cycle attributes of the gates are initialized consistent with a uniform ALAP schedule:
  this modified ALAP schedule aims to have an equal number of gates starting in each non-empty bundle.
  Subsequently, the gates in the circuit are sorted by their cycle value;
  and the ``bundler`` called to produce a bundled version of the IR to return.

  This method is called by ``p.schedule()`` for each circuit of the program when uniform and ALAP scheduling.

- ``bundles = sched.schedule_asap(resource_manager, platform, sched_dot)``

  This method is called by ``cc_light_schedule_rc`` after calling ``sched.init``,
  and creation of the resource manager
  for each circuit of the program when non-uniform ASAP scheduling.
  See :ref:`scheduling_function` for a more extensive description.


- ``bundles = sched.schedule_alap(resource_manager, platform, sched_dot)``
  
  This method is called by ``cc_light_schedule_rc`` after calling ``sched.init``,
  and creation of the resource manager
  for each circuit of the program when non-uniform ALAP scheduling.
  See :ref:`scheduling_function` for a more extensive description.

In the ``sched_dot`` parameter of the methods above
a ``dot`` representation of the dependence graph of the kernel's circuit is constructed,
in which the gates are ordered along a timeline according to their cycle attribute.


Input and output intermediate representation
^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^

The schedulers expect kernels with or without a circuit.
When with a circuit, the ``cycle`` attribute need not be valid.
Gates that are supported on input are one-qubit ``measure``, no-operand ``display``, any classical gate,
``cnot``, ``cz``/``cphase``, and any other quantum and scheduling gate.

They produce a circuit with the same gates (but potentially differently ordered).
The ``cycle`` attribute of each gate has been defined.
The gates in the circuit are ordered with non-decreasing cycle value.
The cycle values are consistent with all constraints imposed during scheduling
and with the scheduling strategy that has been specified through the options or by selection of the entry point.

:Note: There are no gates for control flow; so these are not defined in the configuration file; these are not scheduled in the usual way; these are not translated to QASM and external representations in the usual way. See :ref:`kernel`.

.. _scheduling_options:

Options
^^^^^^^

The following options are supported:

- ``scheduler``
  With the value ``ASAP``, the scheduler creates a forward As Soon As Possible schedule of the circuit.
  With the value ``ALAP``, the scheduler creates a backward As Soon As Possible schedule
  which is equivalent to a forward As Late As Possible schedule of the circuit.
  Default value is ``ALAP``.

- ``scheduler_uniform``
  With the value ``yes``, the scheduler creates a uniform schedule of the circuit.
  With the value ``no``, it doesn't.
  Default value is ``no``.

- ``scheduler_commute``
  With the value ``yes``, the scheduler exploits commutation rules for ``cnot``, and ``cz``/``cphase``
  to have more scheduling freedom to aim for a shorter latency circuit.
  With the value ``no``, it doesn't.
  Default value is ``no``.

- ``output_dir``
  The value is the name of the directory which should be present in the current directory during
  execution of OpenQL, where all output and report files of OpenQL are created.
  Default value is ``test_output``.

- ``write_qasm_files``
  When it has the value ``yes``, ``p.schedule`` produces in the output directory
  a bundled QASM (see :ref:`output_external_representation`) of all kernels
  in a single file with as name the name of the program followed by ``_scheduled.qasm``.

- ``print_dot_graphs``
  When it has the value ``yes``, ``p.schedule`` produces in the output directory
  in multiple files each with as name the name of the kernel followed by ``_dependence_graph.dot``
  a ``dot`` representation of the dependence graph of the kernel's circuit.
  Furthermore it produces in the output directory
  in multiple files each with as name the name of the kernel
  followed by the value of the ``scheduler`` option and ``_scheduled.dot``
  a ``dot`` representation of the dependence graph of the kernel's circuit,
  in which the gates are ordered along a timeline according to their cycle attribute.

:Note: The options don't discriminate between the prescheduler and the rcscheduler although these could desire different option values. Also there is not an option to skip this pass.

.. _scheduling_function:

Function
^^^^^^^^

Scheduling of a circuit starts with creation of the dependence graph;
see :ref:`scheduling_entry_points` for its definition.

Gates that are supported on input are one-qubit ``measure``, no-operand ``display``, any classical gate,
``cnot``, ``cz``/``cphase``, and any other quantum and scheduling gate.
With respect to dependence creation,
the latter ones are assumed to use and update each of their operands during the operation;
and the former ones each have a specific definition regarding the use and update of their operands:

- ``measure`` also updates its corresponding classical register;

- ``display`` and the classical gates use/update all qubits and classical registers (so these act as barriers);

- ``cnot`` uses and doesn't update its control operand, and it commutes with ``cnot``/``cz``/``cphase`` with equal control operand; ``cnot`` uses and updates its target operand, it commutes with ``cnot`` with equal target operand;

- ``cz``/``cphase`` commutes with ``cnot``/``cz``/``cphase`` with equal first operand, and it commutes with ``cz``/``cphase`` with equal second operand.  This commutation is exploited to aim for a shorter latency circuit when the ``scheduler_commute`` option is in effect.

When scheduling without resource constraints
the cycle attributes of the gates are initialized consistent with an ASAP (i.e. downward/forward)
or ALAP (i.e. upward/backward) walk over the dependence graph.
Subsequently, the gates in the circuit are sorted by their cycle value;
and the ``bundler`` called to produce a bundled version of the IR to return.

The remaining part of this subsection describes scheduling with resource constraints.

The implementation of this list scheduler is parameterized on doing a forward or a backward schedule.
The former is used to create an ASAP schedule and the latter is used to create an ALAP schedule.
We here describe the forward case because that is easier to grasp and later come back on the backward case.

A list scheduler maintains at each moment a list of gates that are available for being scheduled
because they are not blocked by dependences on non-scheduled gates.
Not all gates that are available (not blocked by dependences on non-scheduled gates) can actually be scheduled.
It must be made sure in addition that
those scheduled gates that it depends on, actually have completed their execution (using its ``duration``)
and that the resources are available for it.
Furthermore, making a selection from the gates that remain after ignoring these,
determines the optimality of the scheduling result.
The implemented list scheduler is a critical path scheduler,
i.e. it prefers to schedule the most critical gate first.
The criticality of a gate estimates
the effect that delaying scheduling the gate has on the latency of the resulting circuit,
and is determined by computing the length of the longest dependence chain from the gate to the SINK gate;
the higher this value, the higher the gate's scheduling priority in the current cycle is.

The scheduler relies on the dependence graph representation of the circuit.
At the start only the SOURCE gate is available.
Then one by one, according to a criterion, a gate is selected from the list of available ones
and added to the schedule. Having scheduled the gate, it is taken out of the available list;
after having scheduled a gate,
some new gates may become available because they don't depend on non-scheduled gates anymore;
those gates are found and put in the available list of gates.
This continues, filling cycle by cycle from low to high,
until the available list gets empty (which happens after scheduling the last gate, the SINK gate).

Above it was mentioned that a gate can only be scheduled in a particular cycle
when the resources are available for it.
In this, the scheduler relies on the resource manager of the platform.
The latter was created and initialized from the platform configuration file before scheduling started.
Please refer to :ref:`cclplatform` for a description of the specification of resources of the CC-Light platform.
And furthermore note that only the resources that are specified in the platform configuration file
determine the resource constraints that apply to the scheduler; recall that for each resource type,
several resources can be specified, each of which typically has some kind of exclusive use.
The simplest one is the ``qubits`` resource type of which there are as many resources as there are qubits.
The resource manager maintains a so-called ``machine state`` that describes the occupation status of each resource.
This resource state typically consists of two elements: the operation type that is using this resource;
and the occupation period, which is described by a pair of cycle values,
representing the first cycle that it is occupied, and the first cycle that it is free again, respectively.

If a gate is to be scheduled at cycle ``t``,
then all the resources for executing the gate are checked to be available
from cycle ``t`` till (and not including) ``t`` plus the gate's ``duration`` in cycles;
and when actually committing to scheduling the gate at cycle ``t``,
all its resources are set to occupied for the duration of its execution.
The resource manager offers methods for this check (``bool rm.available()``) and commit (``rm.reserve()``).
Doing this check and committing for a particular gate, some additional gate attributes may be required by the resource manager.
For the CC-Light resource manager, these additional gate attributes are:

- ``operation_name`` initialized from the configuration file ``cc_light_instr`` gate attribute representing the operation of the gate; it is used by the ``qwgs`` resource type only; two gates having the same ``operation_name`` are assumed to use the same wave form

- ``operation_type`` initialized from the configuration file ``type`` gate attribute representing the kind of operation of the gate: ``mw`` for rotation gates, ``readout`` for measurement gates, and ``flux`` for one and two-qubit flux gates; it is used by each resource type

This concludes the description of the involvement of the resource manager in the scheduling of a gate.

The list scheduler algorithm uses a so-called availability list to represent gates that can be scheduled; see above.
When the available list becomes empty, all cycle values were assigned and scheduling is almost done.
The gates in the circuit are then first sorted on their cycle value.

Then latency compensation is done:
for each gate for which in the platform configuration file a ``latency`` attribute value is specified,
the gate's cycle value is incremented by this latency value converted to cycles; the latter is usually negative.
This mechanism allows to start execution of a gate earlier to compensate
for a relative delay in the control electronics that is involved in executing the gate.
So in theory, in the quantum hardware, gates which before latency compensation had the same cycle value,
also execute in the same cycle.
After this, the gates in the circuit are again sorted on their cycle value.

After the ``bundler`` has been called to produce a bundled IR, any buffer delays are inserted.
Buffer delays can be specified in the platform configuration file in the ``hardware_settings`` section.
Insertion makes use of the ``type`` attribute of the gate in the platform configuration file,
the one which can have the values ``mw``, ``readout`` and ``flux``.
For each bundle, it checks for each gate in the bundle,
whether there is a non-zero buffer delay specified with a gate in the previous bundle,
and if any, takes the maximum of those buffer delays, and adds it (converted to cycles)
to the bundle's ``start_cycle`` attribute. Moreover, when the previous bundle got shifted in time
because of earlier bundle delays, the same shift is applied first to the current bundle.
In this way, the schedule gets stretched for all qubits at the same time.
This is a valid thing to do and doesn't invalidate dependences nor resource constraints.

:Note: Buffer insertion only has effect on the ``start_cycle`` attributes of the bundles and not on the ``cycle`` attributes of the gates. It would be better to do buffer insertion on the circuit and to do bundling afterwards, so that circuit and bundles are consistent.

In the backward case, the scheduler traverses the dependence graph bottom-up, scheduling the SINK gate first.
Gates become available for scheduling at a particular cycle
when at that cycle plus its duration all its dependent gates have started execution.
And scheduling finishes when the available list is empty, after having scheduled the SOURCE gate.
In this, cycles are decremented after having scheduled SINK at some very high cycle value,
and later, after having scheduled SOURCE,
the cycle values of the gates are consistently shifted down so that SOURCE starts at cycle 0.
The resource manager's state and methods also are parameterized on the scheduling direction.

Scheduling for software platforms
^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^

- Scheduling for qx
- Scheduling for quantumsim


Scheduling for hardware platforms
^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^

- Scheduling for CC-Light platform
- Scheduling for CC platform
- Scheduling for CBox platform


.. include:: scheduling_ccl.rst
.. include:: scheduling_cc.rst
