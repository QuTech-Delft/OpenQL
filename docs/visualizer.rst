.. _visualizer:

==========
Visualizer
==========

The visualizer is a special compiler pass that will visualize the quantum circuit being compiled. Three different types of visualization can be generated:

* **basic circuit visualization**: displays the circuit as an abstract set of gates acting on qubits
* **pulse visualization**: specifically designed for the quantum hardware designed by the DiCarlo lab, this visualization displays the RF pulses that control the quantum hardware
* **a mapping graph**: shows the evolution of the mapping between logical and real qubits as the circuit runs each cycle
* **the qubit interaction graph**: displays the interactions between qubits in the circuit

All of these visualization types can be customised to turn features on or off and to change the looks of the visualization. This is done by way of the
visualizer configuration file, which will be elaborated upon in another section.

------------------------
General visualizer usage
------------------------
The visualizer can be ran by adding the visualizer pass to the compiler, providing the pass with the paths to the configuration files, and compiling the
quantum program you want to run. Python example:

.. code:: python

    c = ql.Compiler("example_compiler")

    c.add_pass("Visualizer");
    c.set_pass_option("Visualizer", "visualizer_type", "<insert type of visualization here>")
    c.set_pass_option("Visualizer", "visualizer_config_path", "<insert full path to file here>"));

    # only necessary when using the pulse visualization
    c.set_pass_option("Visualizer", "visualizer_waveform_mapping_path", "<insert full path to file here>"));

    # add program and gates

    c.compile(program)

There are three different pass options for the visualizer, each with a default value if no user-specified value is provided:

+----------------------------------+------------------------+
| option                           | default value          |
+==================================+========================+
| visualizer_type                  | CIRCUIT                |
+----------------------------------+------------------------+
| visualizer_config_path           | visualizer_config.json |
+----------------------------------+------------------------+
| visualizer_waveform_mapping_path | waveform_mapping.json  |
+----------------------------------+------------------------+

The first option, ``visualizer_type`` determines the type of visualization to use. ``CIRCUIT`` for the basic circuit visualization, ``MAPPING_GRAPH`` for
the mapping graph visualization and ``INTERACTION_GRAPH`` for the qubit interaction graph. The visualization parameters are read from the configuration file 
specified by the ``visualizer_config_path`` pass option. When using the pulse visualization, the waveform mapping configuration file, stored in the
``visualizer_waveform_mapping_path`` pass option is used to determine the mapping between gates and pulses.

---------------------
Circuit visualization
---------------------

The circuit visualizater produces an image of the circuit containing the operations on each qubit per cycle. An OpenQL program can use default gates or
custom gates. Note that default gates will be deprecated at some point in the future.

Custom gates
------------

When using custom gates the default gate visualizations are not used and the visualization needs to be defined by the user. In the ``instructions``
section of the visualizer configuration file, each instruction 'type' has its own corresponding description of gate visualization parameters.
These instruction types are mapped to actual custom instructions from the hardware configuration file by including an additional attribute to each
custom instruction, describing its visualization type:

.. code: json

    {
        "h q1": {
            "duration": 40,
            "latency": 0,
            "qubits": ["q1"],
            "matrix": [ [0.0,1.0], [1.0,0.0], [1.0,0.0], [0.0,0.0] ],
            "disable_optimization": false,
            "type": "mw",
            "cc_light_instr_type": "single_qubit_gate",
            "cc_light_instr": "h",
            "cc_light_codeword": 91,
            "cc_light_opcode": 9,
            "visual_type": "h"
        }
    }

This custom Hadamard gate defined on qubit 1 has one additional attribute ``visual_type`` describing its visualization type. The value of this 
attribute links to a key in the visualizer configuration file, which has the description of the gate visualization parameters that will be used
to visualize this custom instruction. Note that this allows multiple custom instructions to share the same visualization parameters, without having
to duplicate the parameters.

In the ``instructions`` section of the visualizer configuration file the gate visualization parameters are described like so:

.. code:: json

    {
        "h": {
            "connectionColor": [0, 0, 0],
            "nodes": [
                {
                    "type": "GATE",
                    "radius": 13,
                    "displayName": "H",
                    "fontHeight": 13,
                    "fontColor": [255, 255, 255],
                    "backgroundColor": [70, 210, 230],
                    "outlineColor": [70, 210, 230]
                }
            ]
        }
    }

Each gate has a `connectionColor` which defines the color of the connection line for multi-operand gates, and an array of 'nodes'.
A node is the visualization of the gate acting on a specific qubit or classical bit. If a Hadamard gate is acting on qubit 3, that is
represented by one node. If a CNOT gate is acting on qubits 1 and 2, it will have two nodes, one describing the visualization of the
CNOT gate at qubit 1 and one describing the visualization on qubit 2. A measurement gate measuring qubit 5 and storing the result in
classical bit 0 will again have two nodes.

Each node has several attributes describing its visualization:

* ``type``: the visualization type of the node, see below for a list of the available types
* ``radius``: the radius of the node in pixels
* ``displayName``: text that will be displayed on the node (for example 'H' will be displayed on the Hadamard gate in the example above)
* ``fontHeight``: the height of the font in pixels used by the `displayName`
* ``fontColor``: the color of the font used by the `displayName`
* ``backgroundColor``: the background color of the node
* ``outlineColor``: the color of the edge-line of the node

The colors are defined as RGB arrays: ``[R, G, B]``.

The type of the nodes can be one of the following:

* ``NONE``: the node will not be visible
* ``GATE``: a square representing a gate
* ``CONTROL``: a small filled circle
* ``NOT``: a circle outline with cross inside (a CNOT cross)
* ``CROSS``: a diagonal cross

When a gate has multiple operands, each operand should have a node associated with it. Simply create as many nodes in the node array as
there are operands and define a type and visual parameters for it. Don't forget the comma to seperate each node in the array.
Nodes are coupled to each operand sequentially, i.e. the first node in the node array will be used for the first qubit in the operand vector.

-----------------------
Qubit Interaction Graph
-----------------------

