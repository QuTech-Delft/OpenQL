.. _cclplatform:

CC-Light Platform Configuration File
------------------------------------

The file `hardware_configuration_cc_light.json
<https://github.com/QE-Lab/OpenQL/blob/develop/tests/hardware_config_cc_light.json>`_
available inside the ``tests`` directory is an example configuration file for
the CC-Light platform with 7 qubits.
This file consists of several sections (in arbitrary order) which are described below.

``eqasm_compiler`` is just a field with a single value.
It specifies the backend compiler to be used for this CC-Light platform, which in this case has the name ``cc_light_compiler``.
The backend compiler is called after the platform independent passes, and calls several private passes by itself.
This backend compiler and its passes are described in detail in :ref:'Compiler_Passes'.
One of these is the code generation pass.

.. code::

    "eqasm_compiler" : "cc_light_compiler",

``hardware_settings`` is used to configure various
hardware settings of the platform as shown below. These settings affect the
scheduling of instructions.

.. code-block::
   :linenos:

	"hardware_settings":
	{
		"qubit_number": 7,
		"cycle_time" : 20,

		"mw_mw_buffer": 0,
		"mw_flux_buffer": 0,
		"mw_readout_buffer": 0,
		"flux_mw_buffer": 0,
		"flux_flux_buffer": 0,
		"flux_readout_buffer": 0,
		"readout_mw_buffer": 0,
		"readout_flux_buffer": 0,
		"readout_readout_buffer": 0
	}

``qubit_number`` indicates the number of (real) qubits available in the platform.
Instructions that addresss qubits do this by providing a qubit index in the range of 0 to qubit_number-1.
Using an index outside this range will raise an error.

``cycle_time`` is the clock cycle time.
As all other timing specifications in the configuration file it is specified in nanoseconds.
Only at multiples of this cycle time, instructions can start executing.
The schedulers assign a cycle value to each instruction, which means that that instruction can start executing
a number of nanoseconds after program execution start
that equals that cycle value multiplied by the clock cycle time value.

The other entries of the ``hardware_settings`` section specify various buffer times to be
inserted between various operations due to control electronics setup. For example,
``mw_mw_buffer`` can be used to specify time to be inserted between a microwave
operation followed by another microwave operation. See :ref:'Scheduling' for details.

``topology`` specifies the mapping of qubit indices to qubit positions in the platform, as well as the mapping of edge indices to edges in the platform.
An edge is a connection between a pair of qubits.
It is directed to distinguish the control and target qubits of two-qubit gates.

Figure :numref:`fig_qubit_numbering_ccl` shows these numberings in the 7 qubit CC-Light platform.

.. _fig_qubit_numbering_ccl:

.. figure:: ./qubit_number.png
    :width: 800px
    :align: center
    :alt: Qubit and edge numbering in the 7 qubits CC-Light Platform
    :figclass: align-center

    Qubit and edge numbering in the 7 qubits CC-Light Platform


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


.. code-block::
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
- the QISA, the instruction set of the platform, notably in the instructions that set the masks stored in the mask registers that are used in the instructions of two-qubit gates to address the operands.
- the mapper pass that maps virtual qubit indices to real qubit indices. It is described in detail in :ref:`mapping`.
- the postdecomposition pass that ...


``resources`` is the section that is used to specify/configure various resources available
in the platform as discussed below. Specification of these resources effect
scheduling and mapping of instructions. CC-Light architecture  assumes the
following connections in `hardware_configuration_cc_light.json
<https://github.com/QE-Lab/OpenQL/blob/develop/tests/hardware_config_cc_light.json>`_.

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

Qubits available in the platform are specified in the ``qubits`` section, as shown
below. For CC-Light only ``count`` needs to be specified which indicates the
number of available qubits. Each qubit  can be used by only one gate at a time.

.. code-block::
   :linenos:

	"qubits":
	{
	    "count": 7
	},

Single-qubit rotation gates (instructions of 'mw' type) are controlled by qwgs.
Each qwg controls a private set of qubits.  A qwg can control multiple qubits at
the same time, but only when they perform the same gate and started at the same
time. Waveform generators and their constraints are specified in ``qwgs``
section, as shown below.

.. code-block::
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

The number of these waveform generators is specified by the ``count`` field. In
the ``connection_map`` it is specified which wafeform generator is connected to
which qubits. For instance, Line 6 specifies that ``qwg 0`` is connected to
qubits 0 and 1. This is based on ``AWG-8 1, channel 0`` entry in 4th row in
Table :numref:`table_ccl_connections` This information is utilized by the
scheduler to perform resource-constraint aware scheduling of instructions.

.. note::
	By providing an empty list for a qwg will result in not applying any qwg
	constraint during scheduling.

Single-qubit measurements (instructions of 'readout' type) are controlled by
measurement units.  Each one controls a private set of qubits.  A measurement
unit can control multiple qubits at the same time, but only when they started
at the same time. There are 'count' number of measurement units. For each
measurement unit it is described which set of qubits it controls. Available
measurement/readout units are specified in ``meas_units`` section, as shown
below.

.. code-block::
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


Two-qubit flux gates (instructions of ``flux`` type) are controlled by
qubit-selective frequency detuning.  Frequency-detuning may cause neighbor
qubits (qubits connected by an edge) to inadvertently engage in a two-qubit flux
gate as well. This happens when two connected qubits are both executing a
two-qubit flux gate. Therefore, for each edge executing a two-qubit gate,
certain other edges should not execute a two-qubit gate.

Edges and the constraints imposed by these edges are specified in ``edges``
section. ``count`` at Line 3 specifies the number of edges between qubits in the
platform. ``connection_map`` specifies connections. For example, on Line 6, Edge
0 implies a constraint on Edge 2 and Edge 10. This means, if Edge 0 is reserved
for an operation, an operation on Edge 2 and Edge 10 will not be scheduled,
until operation on Edge 0 is complete.

.. code-block::
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

.. note::
	By providing an empty list for an edge in the ``connection_map`` will result
	in not applying any edge constraint during scheduling.


Detuning constraints are specified in ``detuned_qubits`` section. A two-qubit
flux gate lowers the frequency of its source qubit to get near the frequency of
its target qubit.  Any two qubits which have near frequencies execute a
two-qubit flux gate. To prevent any neighbor qubit of the source qubit that has
the same frequency as the target qubit to interact as well, those neighbors must
have their frequency detuned (lowered out of the way).  A detuned qubit cannot
execute a single-qubit rotation (an instruction of 'mw' type).  An edge is a
pair of qubits which can execute a two-qubit flux gate.  There are ``count``
number of edges. For each edge it is described, when executing a two-qubit gate for it,
which set of qubits it detunes.

.. code-block::
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


Instructions can be specified/configured in ``instructions section``. An example
of a 1-qubit 2-qubit instruction is shown below:

.. code-block::
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

``x q0`` is the name of the instruction which will be used to refer to this
instruction inside OpenQL program. The ``duration`` specifies the time
duration required to complete this instruction. Due to control electronics, it
is sometimes required to add a positive or negtive latency to an instruction.
This can be specified by ``latency`` field. This field is divided by cycle
time and rounded up to obtain an integer number of cycles. After scheduling is
performed, an instruction is shifted back or forth in time depending upon
calculated cycles corresponding to the latency field.


``qubits`` refer to the list of qubit operands.

``matrix`` field specifies the process matrix representing this instruction.
If optimization is enabled, this matrix will be used by optimizer to fuse
operations together, as discussed in :ref:'optimization'. This can be left
un-specified if optimization is disabled.

``disable_optimization`` field is used to enable/disable optimization of this
instruction. Setting ``disable_optimization`` to ``true`` will mean that this
instruction cannot be compiled away during optimization.

An instruction can be of microwave, flux or readout type which is
specified by the ``type`` field. ``cc_light_instr_type`` field is used to
specify the type of instruction based on number of qubits. ``cc_light_instr``
specifies the name of this instruction used in CC-Light architecture. This name
will be used in the generated output code.

``cc_light_codeword``, ``cc_light_right_codeword``, ``cc_light_left_codeword``
and ``cc_light_opcode`` are used in the generation of control store file for
CC-Light platform. For single qubit instructions, ``cc_light_codeword`` refers
to the codeword to be used for this instruction. Recall that quantum pipeline
contains a VLIW front end with two VLIW lanes, each lane processing one
quantum operation. ``cc_light_right_codeword`` and ``cc_light_left_codeword``
is used to specify the codewords used for the left and right operation in
two-qubit instruction. ``cc_light_opcode`` specifies the opcode used for this
instruction.

.. warning::
	At the moment, generation of control-store file is disabled in
	the compiler as this was not being used in experiments.


Gate decompositions can also be specified in the configuration file in the
``gate_decomposition`` section. Examples of two decompositions are shown below.
``%0`` and ``%1`` refer to first argument and second argument. This means
according to the decomposition on Line 2, ``rx180 %0`` will allow us to
decompose ``rx180 q0`` to ``x q0``. Simmilarly, the decomposition on Line 3 will
allow us to decompose ``cnot q2, q0`` to three instructions, namely; ``ry90
q2``, ``cz q2, q0`` and ``ry90 q0``.

.. code-block::
   :linenos:

	"gate_decomposition": {
		"rx180 %0" : ["x %0"],
		"cnot %0,%1" : ["ry90 %0","cz %0,%1","ry90 %1"]
	}

These decompositions are simple macros (in-place substitutions) which allow
programmer to mannually specify a decomposition. These take place at the time
of creation of kernel. This means scheduler will schedule decomposed
instructions. OpenQL can also perform Control and Unitary decompositions which
are discussed in :ref:'decompositions'.


