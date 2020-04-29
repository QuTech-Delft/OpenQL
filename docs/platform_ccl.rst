.. _cclplatform:

CC-Light Platform
-----------------

The file `hardware_configuration_cc_light.json
<https://github.com/QE-Lab/OpenQL/blob/develop/tests/hardware_config_cc_light.json>`_
available inside the ``tests`` directory is an example configuration file for
the CC-Light platform with 7 qubits.

This file consists of several sections (in arbitrary order) which are described below.

``eqasm_compiler`` specifies the backend compiler to be used for this CC-Light platform,
which in this case has the name ``cc_light_compiler``.
The backend compiler is called after the platform independent passes, and calls several private passes by itself.
This backend compiler and its passes are described in detail in :ref:`compiler_passes`.
One of these is the code generation pass.

.. code::

    "eqasm_compiler" : "cc_light_compiler",

``hardware_settings`` is used to configure various
hardware settings of the platform as shown below. These settings affect the
scheduling of instructions. Please refer to :ref:`platform` for a full description and an example.

``topology`` specifies the mapping of qubit indices to qubit positions in the platform, as well as the mapping of connection indices to connections in the platform.
A connection is a directed connection in the platform between a pair of qubits that supports qubit interaction.
It is directed to distinguish the control and target qubits of two-qubit gates.
In a platform topology's connection graph, qubits are the nodes, and connection are the edges.

Figure :numref:`fig_qubit_numbering_ccl` shows these numberings in the 7 qubit CC-Light platform.

.. _fig_qubit_numbering_ccl:

.. figure:: ./qubit_number.png
    :width: 800px
    :align: center
    :alt: Connection graph with qubit and connection (edge)  numbering in the 7 qubits CC-Light Platform
    :figclass: align-center

    Connection graph with qubit and connection (edge) numbering in the 7 qubits CC-Light Platform


The ``topology``  section starts with
the specification of the two dimensions of a rectangular qubit grid by specifying ``x_size`` and ``y_size``.
The positions of the real qubits of the platform are defined relative to this (artificial) grid.
The coordinates in the X direction are 0 to x_size-1.
In the Y direction they are 0 to y_size-1.
Next, for each available qubit in the platform, its position in the grid is specified:
the ``id`` specifies the particular qubit's index, and ``x`` and ``y`` specify its position in the grid,
as coordinates in the X and Y direction, respectively.
Please note that not every position in the x_size by y_size grid needs to correspond to a qubit.

Qubits are connected in directed pairs, called edges.
Edge indices form a contigous range starting from 0.
Each edge in the topology is given an ``id`` which denotes its index, and a source (control) and destination (target) qubit index by ``src`` and ``dst``, respectively. This means that although Edge 0 and Edge 8 are
between qubit 0 and qubit 2, they are different as these edges are in opposite directions.
The qubit indices specified here must correspond to available qubits in the platform.

.. code-block:: html
	:linenos:

	"topology" : {
		"x_size": 5,
		"y_size": 3,
		"qubits":
		[ 
			{ "id": 0,  "x": 1, "y": 2 },
			{ "id": 1,  "x": 3, "y": 2 },
			{ "id": 2,  "x": 0, "y": 1 },
			{ "id": 3,  "x": 2, "y": 1 },
			{ "id": 4,  "x": 4, "y": 1 },
			{ "id": 5,  "x": 1, "y": 0 },
			{ "id": 6,  "x": 3, "y": 0 }
		],
		"edges":
		[
			{ "id": 0,  "src": 2, "dst": 0 },
			{ "id": 1,  "src": 0, "dst": 3 },
			{ "id": 2,  "src": 3, "dst": 1 },
			{ "id": 3,  "src": 1, "dst": 4 },
			{ "id": 4,  "src": 2, "dst": 5 },
			{ "id": 5,  "src": 5, "dst": 3 },
			{ "id": 6,  "src": 3, "dst": 6 },
			{ "id": 7,  "src": 6, "dst": 4 },
			{ "id": 8,  "src": 0, "dst": 2 },
			{ "id": 9,  "src": 3, "dst": 0 },
			{ "id": 10,  "src": 1, "dst": 3 },
			{ "id": 11,  "src": 4, "dst": 1 },
			{ "id": 12,  "src": 5, "dst": 2 },
			{ "id": 13,  "src": 3, "dst": 5 },
			{ "id": 14,  "src": 6, "dst": 3 },
			{ "id": 15,  "src": 4, "dst": 6 }
		]
	},


These mappings are used in:

* the QISA, the instruction set of the platform, notably in the instructions that set the masks stored in the mask registers that are used in the instructions of two-qubit gates to address the operands.
* the mapper pass that maps virtual qubit indices to real qubit indices. It is described in detail in :ref:`mapping`.
* the postdecomposition pass that maps two-qubit flux instructions to sets of one-qubit flux instructions.


``resources`` is the section that is used to specify/configure various resource types available
in the platform as discussed below. Specification of these resource types affects
scheduling and mapping of gates. The configuration of the various resource types
in `hardware_configuration_cc_light.json
<https://github.com/QE-Lab/OpenQL/blob/develop/tests/hardware_config_cc_light.json>`_
assumes that the CC-Light architecture has the following relations between devices, connections, qubits and operations:

.. _table_ccl_connections:

.. table::
	:align: center

	=====================    =============   =============      =================== 
	   Device Name           DIO connector   Target qubits        Operation Type    
	=====================    =============   =============      =================== 
	 UHFQC-0                   DIO1          0, 2, 3, 5, 6          measurement       
	 UHFQC-1                   DIO2             1, 4                measurement       
	 AWG-8 0, channel 0~6      DIO3             0~6                    flux              
	 AWG-8 1, channel 0        DIO4             0,1                  microwave         
	 AWG-8 1, channel 1        DIO4             5,6                  microwave         
	 AWG-8 2, channel 0        DIO5            2,3,4                 microwave         
	 VSM                        --              0~6              microwave masking 
	=====================    =============   =============      =================== 

The ``resources`` section specifies zero or more resource types.
Each of these must be predefined by the platform's resource manager.
For CC-Light, these resource types are ``qubits``, ``qwgs``, ``meas_units``, ``edges`` and ``detuned_qubits``.
The presence of one in the configuration file
indicates that the resource-constrained scheduler should take it into account
when trying to schedule operations in parallel, i.e. with overlapping executions;
absence of one in the configuration file thus indicates that this resource is ignored by the scheduler.
Although their names suggest otherwise, they are just vehicles to configure the scheduler
and need not correspond to real resources present in the hardware.

``qubits``: That one qubit can only be involved in one operation at each particular cycle,
is specified by the ``qubits`` resource type, as shown
below. ``count`` needs to be at least the number of available qubits.

.. code-block:: html
    :linenos:

	"qubits":
	{
	    "count": 7
	},

So, when this resource type is included in the configuration in this way,
it will guarantee that the resource-constrained scheduler will never schedule two operations in parallel
when these share a qubit index in the range of 0 to count-1 as operand.

``qwgs``: This resource type specifies, when configured, several sets of qubit indices.
For each set it specifies that when one of the qubits in the set is in use in a particular cycle
by an instruction of 'mw' type (single-qubit rotation gates usually),
that when one of the other qubits in the set is in use by an instruction of 'mw' type,
that instruction must be doing the same operation.
In CC-light, this models QWG wave generators that only can generate one type of wave at the same time,
and in which each wave generator is connected through a switch to a subset of the qubits.

.. code-block:: html
    :linenos:

	"qwgs" :
	{
	  "count": 3,
	  "connection_map":
	  {
	    "0" : [0, 1],
	    "1" : [2, 3, 4],
	    "2" : [5, 6]
	  }
	},

The number of sets (waveform generators) is specified by the ``count`` field. In
the ``connection_map`` it is specified which waveform generator is connected to which qubits.
Each qubit that can be used by an instruction of 'mw' type,
should be specified at most once in the combination of sets of connected qubits.
For instance, the line with ``"0"`` specifies that ``qwg 0`` is connected to
qubits 0 and 1. This is based on the ``AWG-8 1, channel 0`` entry in
Table :numref:`table_ccl_connections`. This information is utilized by the
scheduler to perform resource-constraint aware scheduling of gates.

``meas_units``: This resource type is similar to ``qwgs``; the difference is
that it is not constraining on the operations to be equal
but on the start cycle of measurement to be equal.
It specifies, when configured, several sets of qubit indices.
For each set it specifies that when one of the qubits in the set is in use in a particular cycle
by an instruction of 'readout' type (measurement gates usually)
that when one of the other qubits in the set is in use by an instruction of 'readout' type
the latter must also have started in that cycle.
In CC-light, this models measurement units that each can only measure multiple qubits at the same time
when the measurements of those qubits start in the same cycle.

There are ``count`` number of sets (measurement units). For each
measurement unit it is described which set of qubits it controls.
Each qubit that can be used by an instruction of 'readout' type,
should be specified at most once in the combination of sets of connected qubits.

.. code-block:: html
	:linenos:

	"meas_units" :
	{
	  "count": 2,
	  "connection_map":
	  {
	    "0" : [0, 2, 3, 5, 6],
	    "1" : [1, 4]
	  }
	},


``edges``: This resource type specifies, when present, for each directed qubit pair corresponding
to a directed connection in the platform (``edge``), which set of other edges
cannot execute a two-qubit gate in parallel.

Two-qubit flux gates (instructions of ``flux`` type) are controlled by
qubit-selective frequency detuning.  Frequency-detuning may cause neighbor
qubits (qubits connected by an edge) to inadvertently engage in a two-qubit flux
gate as well. This happens when two connected qubits are both executing a
two-qubit flux gate. Therefore, for each edge executing a two-qubit gate,
certain other edges should not execute a two-qubit gate.

Edges and the constraints imposed by these edges are specified in the ``edges`` section.
``count`` specifies at least the number of edges between qubits in the platform.
``connection_map`` specifies connections.
For example, the entry with "0" specifies for Edge 0 a constraint on Edge 2 and Edge 10.
This means, if Edge 0 is in use by a two-qubit flux gate,
a two-qubit flux gate on Edge 2 and Edge 10 will not be scheduled, until the one on Edge 0 completes.

When ``edges`` is present as a resource type, each edge of the platform must appear in the ``connection_map``.
Providing an empty list for an edge in the ``connection_map`` will result
in not applying any edge constraint during scheduling.

.. code-block:: html
    :linenos:

	"edges":
	{
	  "count": 16,
	  "connection_map":
	  {
	    "0": [2, 10], 
	    "1": [3, 11],
	    "2": [0, 8],
	    "3": [1, 9],
	    "4": [6, 14],
	    "5": [7, 15],
	    "6": [4, 12],
	    "7": [5, 13],
	    "8": [2, 10],
	    "9": [3, 11],
	    "10": [0, 8],
	    "11": [1, 9],
	    "12": [6, 14],
	    "13": [7, 15],
	    "14": [4, 12],
	    "15": [5, 13]
	  }
	},


``detuned_qubits``: Constraints on executing two-qubit gates in parallel to other gates,
are specified in this ``detuned_qubits`` section, when present.
For each edge, the set of qubits is specified that cannot execute a gate
when on the particular edge a two-qubit gate is executed;
at the same time, this resource type specifies implicitly for each qubit
when it would be executing a gate, on which edges a two-qubit gate cannot execute in parallel.

There are at least ``count`` number of qubits involved.
When ``detuned_qubits`` is present as a resource type,
each edge of the platform must appear in the ``connection_map``.
Providing an empty set of qubits for an edge in the ``connection_map`` will result
in not applying the ``detuned_qubits`` constraint related to this edge during scheduling.
Not all qubits need to be involved in this type of constraint with some edge.
In the example below, Qubit 0 and Qubit 1 are examples of qubits executing a gate on which
can be in parallel to executing a two-qubit gate on any pair of qubits.

A two-qubit flux gate lowers the frequency of its source qubit to get near the frequency of
its target qubit.  Any two qubits which have near frequencies execute a
two-qubit flux gate. To prevent any neighbor qubit of the source qubit that has
the same frequency as the target qubit to interact as well, those neighbors must
have their frequency detuned (lowered out of the way).  A detuned qubit cannot
execute a single-qubit rotation (an instruction of 'mw' type).

.. code-block:: html
    :linenos:

	"detuned_qubits":
	{
	    "count": 7,
	    "connection_map":
	    {
	    "0": [3],
	    "1": [2],
	    "2": [4],
	    "3": [3],
	    "4": [],
	    "5": [6],
	    "6": [5],
	    "7": [],
	    "8": [3],
	    "9": [2],
	    "10": [4],
	    "11": [3],
	    "12": [],
	    "13": [6],
	    "14": [5],
	    "15": []
	    }
	}


``instructions``: Instructions can be specified/configured in the ``instructions`` section.
Examples of a 1-qubit and a 2-qubit instruction are shown below:

.. code-block:: html
    :linenos:

	"instructions": {
		"x q0": {
			"duration": 40,
			"latency": 0,
			"qubits": ["q0"],
			"matrix": [ [0.0,0.0], [1.0,0.0], 
					    [1.0,0.0], [0.0,0.0]
					  ],
			"disable_optimization": false,
			"type": "mw",
			"cc_light_instr_type": "single_qubit_gate",
			"cc_light_instr": "x",
			"cc_light_codeword": 60,
			"cc_light_opcode": 6
		},
		"cnot q2,q0": {
			"duration": 80,
			"latency": 0,
			"qubits": ["q2","q0"],
			"matrix": [ [0.1,0.0], [0.0,0.0], [0.0,0.0], [0.0,0.0],
						[0.0,0.0], [1.0,0.0], [0.0,0.0], [0.0,0.0], 
						[0.0,0.0], [0.0,0.0], [0.0,0.0], [1.0,0.0], 
						[0.0,0.0], [0.0,0.0], [1.0,0.0], [0.0,0.0], 
					  ],
			"disable_optimization": true,
			"type": "flux",
			"cc_light_instr_type": "two_qubit_gate",
			"cc_light_instr": "cnot",
			"cc_light_right_codeword": 127,
			"cc_light_left_codeword": 135,
			"cc_light_opcode": 128
	   },
	   ...
	}

Please refer to :ref:`platform` for a description of the CC-Light independent attributes.
The CC-Light dependent attributes are:

``cc_light_instr_type`` is used to
specify the type of instruction based on the number of expected qubits.
Please refer to :ref:`scheduling` for its use by the rcscheduler.

``cc_light_instr`` specifies the name of this instruction used in CC-Light architecture. This name
is used in the generation of the output code and in the implementation of the checking of the ``qwg`` resource.
Please refer to :ref:`scheduling` for its use by the rcscheduler.

``cc_light_codeword``, ``cc_light_right_codeword``, ``cc_light_left_codeword``
and ``cc_light_opcode`` are used in the generation of the control store file for
CC-Light platform. For single qubit instructions, ``cc_light_codeword`` refers
to the codeword to be used for this instruction. Recall that the quantum pipeline
contains a VLIW front end with two VLIW lanes, each lane processing one
quantum operation. ``cc_light_right_codeword`` and ``cc_light_left_codeword``
are used to specify the codewords used for the left and right operation in
two-qubit instruction. ``cc_light_opcode`` specifies the opcode used for this
instruction.

.. warning::
	At the moment, generation of the control-store file is disabled in
	the compiler as this was not being used in experiments.


``gate_decomposition`` Gate decompositions can also be specified in the configuration file in the
``gate_decomposition`` section. Please refer to :ref:`platform` for a description and full example of this section.



