.. _platform:

Platform
========

OpenQL supports various target platforms.
These platforms can be software simulators or architectures targetting hardware quantum computers.
The following platforms are supported by OpenQL:

- software simulator platforms
	- :ref:`qxplatform`
	- :ref:`qsimplatform`
- hardware platforms
	- :ref:`cclplatform`
	- :ref:`ccplatform`
	- :ref:`cboxplatform`

:Note: Quantumsim and QX are not really platforms. They are means to simulate a particular (hardware) platform. Qasm files for use by QX and python scripts to interface to quantumsim are generated for any hardware platform under the control of options. See the descriptions of the QX and Quantumsim platforms referred to above.

:Note: We are planning to use DQCsim, a platform to connect to simulators. In that context, software simulator platforms are connected to by DQCsim, and OpenQL just provides compilation support to a particular hardware platform.

A platform can be created in OpenQL by using the ``Platform()`` API as shown below:

.. code:: python

    platform = ql.Platform('<platform_name>', <path_to_json_config_file>)

For example,
a platform with the name ``CCL_platform`` and using ``hardware_config_cc_light.json`` as platform configuration file
can be created as:

.. code:: python

    platform = ql.Platform('CCL_platform', 'hardware_config_cc_light.json')


Platform Configuration File
---------------------------

The platform configuration file describes the target platform in JSON format.
The information in this file is used by all platform independent compiler passes.
The parameterization by this information makes these platform independent compiler passes
in source code independent of the platform but in effective function dependent on the platform.

A platform configuration file  consists of several sections (in arbitrary order) which are described below.
Most of these are mandatory; the specification of the ``topology`` and the ``resources`` sections are optional.

For example (the ``...`` contains the specification of the respective section):

.. code-block:: html
    :linenos:

    {
        "eqasm_compiler" : "cc_light_compiler",

        "hardware_settings":
        {
            "qubit_number": 7,
            "cycle_time" : 20,
            ...
        },

        "topology":
        {
            ...
        },

        "resources":
        {
            ...
        },

        "instructions":
        {
            ...
        },

        "gate_decomposition":
        {
            ...
        }
    }


The platform comfiguration file for its structure is platform independent.
It can be extended at will with more sections and more attributes for platform dependent purposes.

The sections below describe sections and attributes that are used by platform independent compiler passes.

Please refer to the sections of the specific platforms for full examples and for the description of any additional attributes.


Attribute eqasm_compiler
^^^^^^^^^^^^^^^^^^^^^^^^

The ``eqasm_compiler`` attribute specifies the backend compiler to be used for this platform.
After the passes of the platform independent compiler have been called,
the platform independent compiler switches out to the backend compiler to run the platform dependent passes.
The specification of this attribute is mandatory.
The ``eqasm_compiler`` attribute can take the following values; these correspond to the platforms that are supported:

* ``none``: no backend compiler is called
* ``qx``: no backend compiler is called; see above how to generate a qasm file for QX
* ``cc_light_compiler``: backend compiler for CC_Light
* ``eqasm_backend_cc``: backend compiler for CC
* ``qumis_compiler``: backend compiler to CBOX


Section hardware_settings
^^^^^^^^^^^^^^^^^^^^^^^^^

The ``hardware_settings`` section specifies various parameters describing the platform.
These include the ``qubit_number'' and ``cycle_time`` which are generally used,
and the buffer delays, only used by the rcscheduler,
which are related to control electronics in the experiments (for hardware backends).
The specification of this section is mandatory.

For example:

.. code-block:: html
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

In this:

* ``qubit_number`` indicates the number of (real) qubits available in the platform.
  Gates and instructions that addresss qubits do this by providing a qubit index in the range of 0 to qubit_number-1.
  Using an index outside this range will raise an error.

* ``cycle_time`` is the clock cycle time.
  As all other timing specifications in the configuration file it is specified in nanoseconds.
  Only at multiples of this cycle time, instructions can start executing.
  The schedulers assign a cycle value to each gate, which means that that gate can start executing
  a number of nanoseconds after program execution start
  that equals that cycle value multiplied by the clock cycle_time value.

* The other entries of the ``hardware_settings`` section specify various buffer times to be
  inserted between various operations due to control electronics setup. For example,
  ``mw_mw_buffer`` can be used to specify time to be inserted between a microwave
  operation followed by another microwave operation. See :ref:`scheduling` for details.


Section topology
^^^^^^^^^^^^^^^^

Specifies the qubit topology as the connection graph of the qubits of the platform.
This is primarily used by the mapping pass; this section is optional.
It specifies the mapping of qubit indices to qubit positions in the platform, as well as the mapping of connection indices to connections in the platform.
A connection is a directed connection in the platform between a pair of qubits that supports qubit interaction.
It is directed to distinguish the control and target qubits of two-qubit gates.
In a platform topology's connection graph, qubits are the nodes, and connection are the edges.

It looks like (the ``...`` contains further specifications):

.. code-block:: html
    :linenos:

    "topology" :
    {
        "x_size": 5,
        "y_size": 3,
        "qubits":
        [
           { "id": 0,  "x": 1, "y": 2 },
           ...
        ],
        "edges":
        [
           { "id": 0,  "src": 2, "dst": 0 },
           ...
        ]
    },


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
Each edge in the topology is given an ``id`` which denotes its index, and a source (control) and destination (target) qubit index by ``src`` and ``dst``, respectively. This means that there can be edges between the same pair of qubits but in opposite directions.
The qubit indices specified here must correspond to available qubits in the platform.

For a full example of this section, please refer to :ref:`cclplatform`.


Section resources
^^^^^^^^^^^^^^^^^

Specify the classical control constraints of the platform.
This section is optional.
These constraints are used by the resource manager, that on its turn is used by the scheduling and mapping passes.
These classical control constraints are described as restrictions on concurrent access to resources of predefined resource types.
Specification of these resources affects
scheduling and mapping of gates.

The ``resources`` section specifies zero or more resource types
that are predefined by the mandatory platform dependent resource manager.
For CC-Light, these resource types are ``qubits``, ``qwgs``, ``meas_units``, ``edges``, and ``detuned_qubits``.
The presence of one in the configuration file
indicates that the resource-constrained scheduler should take it into account
when trying to schedule operations in parallel, i.e. with overlapping executions.
Although their names suggest otherwise, they are just vehicles to configure the scheduler
and need not correspond to real resources present in the hardware.
This also implies that they can be easily reused for other platforms.

For a full example of this section, including an extensive description of the various resource types,
please refer to :ref:`cclplatform`.
For a description of their use by the scheduler, please refer to :ref:`scheduling`.

Section instructions
^^^^^^^^^^^^^^^^^^^^

Specifies the list of primitive gates supported by the platform.
Creation of a primitive custom gate takes its parameters from this specification to initialize the gate's attributes.

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
			"type": "mw"
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
			"type": "flux"
	   },
	   ...
	}

``x q0`` is the name of the instruction which will be used to refer to this
instruction inside the OpenQL program. ``x`` would also be allowed as name. The former defines a ``specialized gate``, the latter defines a ``generalized gate``; please refer to :ref:`quantum_gates` for the definitions of these terms and to :ref:`input_external_representation` for the use of these two forms of instruction definitions.

* ``duration`` specifies the time duration required to complete this instruction.

* ``latency``; due to control electronics, it
  is sometimes required to add a positive or negative latency to an instruction.
  This can be specified by the ``latency`` field. This field is divided by cycle
  time and rounded up to obtain an integer number of cycles. After scheduling is
  performed, an instruction is shifted back or forth in time depending upon
  the calculated cycles corresponding to the latency field.

* ``qubits`` refer to the list of qubit operands.

  :Note: This field has to match the operands in the name of the instruction, if specified there. This is checked. Otherwise there is no use of this field. So there is redundancy here.

* ``matrix`` specifies the process matrix representing this instruction.
  If optimization is enabled, this matrix will be used by the optimizer to fuse
  operations together, as discussed in :ref:`optimization`. This can be left
  un-specified if optimization is disabled.

* ``disable_optimization`` is used to enable/disable optimization of this
  instruction. Setting ``disable_optimization`` to ``true`` will mean that this
  instruction cannot be compiled away during optimization.

  :Note: This is not implemented. Propose to do so. Then have to define what is exactly means: compiling away is interpreted as the gate with this flag ``true`` will never be deleted from a circuit once created, nor that the circuit that contains it will be deleted.

* ``type`` indicates whether the instruction is a microwave (``mw``), flux (``flux``) or readout (``readout``).
  This is used in CC-Light by the resource manager to select the resources of a gate for scheduling.



Section gate_decomposition
^^^^^^^^^^^^^^^^^^^^^^^^^^

Specifies a list of gates defined by decomposition into primitive gates.

Examples of two decompositions are shown below.
``%0`` and ``%1`` refer to the first argument and the second argument. This means
according to the decomposition on Line 2, ``rx180 %0`` will allow us to
decompose ``rx180 q0`` to ``x q0``. Similarly, the decomposition on Line 3 will
allow us to decompose ``cnot q2, q0`` to three instructions, namely:
``ry90 q0``, ``cz q2, q0`` and ``ry90 q0``.

.. code-block:: html
    :linenos:

	"gate_decomposition": {
		"rx180 %0" : ["x %0"],
		"cnot %0,%1" : ["ry90 %1","cz %0,%1","ry90 %1"]
	}

These decompositions are simple macros (in-place substitutions) which allow
programmer to manually specify a decomposition. These take place at the time
of creation of a gate in a kernel. This means the scheduler will schedule decomposed
instructions. OpenQL can also perform Control and Unitary decompositions which
are discussed in :ref:`decomposition`.


.. include:: platform_qx.rst
.. include:: platform_quantumsim.rst
.. include:: platform_ccl.rst
.. include:: platform_cc.rst
.. include:: platform_cbox.rst
