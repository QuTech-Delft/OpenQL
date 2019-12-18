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


Entry points
^^^^^^^^^^^^

The following entry points are supported:

- ``p.schedule()``
  In the context of ``program`` object ``p``, this method schedules the circuits of the kernels of the program,
  according to a strategy specified by the scheduling options,
  but without taking resource constraints and other constraints of the target platform into account.

- ``bundles = cc_light_schedule_rc(circuit, platform, num_qubits, num_creg)``
  In the context of the ``cc_light_eqasm_compiler``, a derived class of the ``eqasm_compiler`` class,
  in its ``compile(prog_name, kernels, platform)`` method, inside a loop over the specified kernels,
  the resource-constrained scheduler is called to schedule the specified circuit,
  according to a strategy specified by the scheduling options,
  and taking resource constraints and other constraints of the target platform into account.
  It creates a bundled version of the IR and returns it.

These entry points each create a ``sched``  object of class ``Scheduler`` and call a selection of its methods:

- ``sched.init(circuit, platform, num_qubits, num_creg)``
  A dependence graph representation of the specified circuit is constructed.
  This graph is a Directed Acyclic Graph (DAG).
  In this graph, the nodes represent the gates and the directed edges the dependences.
  The top of the graph is a newly created SOURCE gate, the bottom is a newly created SINK gate.
  With respect to dependences, the SOURCE and SINK gates behave as if they update all qubits and classical registers.
  Gates are added in the order of presence in the circuit
  and linked in dependence chains according to their operation and operands.

  The nodes have as attributes (apart from the gate's attributes):

  - ``name`` with the qasm string representation of the gate (such as ``cnot q[1],q[2]``)

  The edges have as attributes:

  - ``weight`` representing the number of cycles needed from the start of execution of the gate at the source of the edge, to the start of execution of the gate at the target of the edge; this value is initialized from the ``duration`` attribute of the gate

  - ``cause`` representing the qubit or classical register causing the dependence

  - ``depType`` representing the type of the dependence

  The latter two attributes are currently only used internally in the dependence graph construction.

- ``bundles = sched.bundler(circuit)``
  Construct the bundled representation of the circuit. The cycle attribute of each gate of the circuit must be valid,
  and the gates in the circuit must have been sorted by their cycle value.

- ``bundles = sched.schedule_asap(sched_dot)``
  The cycle attributes of the gates are initialized consistent with an ASAP (i.e. downward) walk over the dependence graph.
  Subsequently, the gates in the circuit are sorted by their cycle value;
  and the ``bundler`` called to produce a bundled version of the IR to return.

- ``bundles = sched.schedule_alap(sched_dot)``
  The cycle attributes of the gates are initialized consistent with an ALAP (i.e. upward) walk over the dependence graph.
  Subsequently, the gates in the circuit are sorted by their cycle value;
  and the ``bundler`` called to produce a bundled version of the IR to return.

- ``bundles = sched.schedule_alap_uniform()``
  The cycle attributes of the gates are initialized consistent with a uniform ALAP schedule:
  this modified ALAP schedule aims to have an equal number of gates starting in each non-empty bundle.
  Subsequently, the gates in the circuit are sorted by their cycle value;
  and the ``bundler`` called to produce a bundled version of the IR to return.

- ``bundles = sched.schedule_asap(resource_manager, platform, sched_dot)``
  TBD: call the real scheduler, rm, latency, buffers

- ``bundles = sched.schedule_alap(resource_manager, platform, sched_dot)``
  TBD: call the real scheduler, rm, latency, buffers

Only in older versions of the scheduler (when the option ``scheduler_post179`` is set to ``no``)
and in the scheduler of the mapper branch, in the ``sched_dot`` parameter of the methods above
a ``dot`` representation of the dependence graph of the kernel's circuit is constructed,
in which the gates are ordered along a timeline according to their cycle attribute.


Input and output intermediate representation
^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^

The Intermediate Representation (IR) they expect as input and what they update in the IR; in general, they should accept any IR, so all types of gates, quantum as well as classical.



Options
^^^^^^^

The following options are supported:

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



The particular options they listen to; there usually is an option to disable it; also there are ways to dump the IR before and/or after it although this is not generally possible yet.

Function
^^^^^^^^

The function they perform, in terms of the IR and the options.

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
