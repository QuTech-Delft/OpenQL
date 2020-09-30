.. _visualizer:

Visualizer
==========

The visualizer is a special compiler pass that will display a visualization of the program being compiled by the compiler.
The specific look of that visualization can be controlled by the general visualization parameters, which include options for different
kinds of labels for example, and the gate specific visualization parameters, like the color of the gate, or what kind of shape to show.

Visualizer usage
----------------
The visualizer can be ran by adding the visualizer pass to the compiler and compiling the quantum program you want to run. Python example:

.. code:: python

    c = ql.Compiler("example_compiler")
    c.add_pass("Visualizer");
    c.set_pass_option("Visualizer", "visualizer_config_path", "full_path_to_file"));

    # add program and gates

    c.compile(program)

The visualization parameters are read from a configuration file. The path to this file should be added through a pass option
for the visualizer as seen above. If a parameter is missing from that file, the default hardcoded value contained in `src\visualizer.h` will be used instead.


General visualization parameters
--------------------------------

The json config file containing the visualization parameters has several sections:

* ``cycles``: contains parameters that govern cycle labels, and general gate visualization
* ``bitLines``: defines the labels and lines, including grouping lines for both quantum and classical bitLines
* ``grid``: defines the overal structure of the visualization
* ``measurements``: several parameters controlling measurement visualization
* ``instructions``: a map of instruction types (the keys) with that type's gate visualization as value, used for custom instructions

Example configuration:

.. code:: javascript

    "cycles":
    {
        // whether the cycle labels should be shown
        "showCycleLabels": true, 

        // whether the cycle labels should be shown in nanoseconds or cycle numbers
        "showCyclesInNanoSeconds": true, 
        
        // the height of the cycle label row
        "rowHeight": 24, 
        
        // the font height of the cycle labels
        "fontHeight": 13, 
        
        // the color of the cycle labels
        "fontColor": [0, 0, 0], 

      
        // if true, the visualized circuit will be compressed, with each gate only taking one cycle
        "compressCycles": false, 

        // Whether empty cycle ranges above the threshold should be cut
        "cutEmptyCycles": true, 

        // The threshold amount for empty cycles to be cut
        "emptyCycleThreshold": 2,
       
        // shows a transparent outline for the duration of a multi-cycle gate
        "showGateDurationOutline": true, 
        
        // the gap in pixels between the gate's node and its duration outline
        "gateDurationGap": 2, 

        // the transparency alpha value of the gate duration outline area
        "gateDurationAlpha": 0.1, 
      
        // the alpha value of the gate duration outline itself
        "gateDurationOutLineAlpha": 0.3, 
      
        // the color of the outline
        "gateDurationOutlineColor": [0, 0, 0] 
    },
    "bitLines":
    {
        // whether the labels should be drawn for the bit lines
        "drawLabels": true, 

        // the width of the column reserved for the bit line labels
        "labelColumnWidth": 32, 

        // the font height of the labels
        "fontHeight": 13, 

        // the color of the qubit labels
        "qBitLabelColor": [0, 0, 0], 

        // the color of the classical bit labels
        "cBitLabelColor": [128, 128, 128], 



        // whether the classical bit lines should be shown
        "showClassicalLines": true, 

        // whether all the classical bit lines should be grouped into one 'multi'-line for additional visualization clarity
        "groupClassicalLines": true, 

        // controls the space between the double line for the grouped classical bit line (if enabled)
        "groupedClassicalLineGap": 2, 

        // the color of the qubit lines
        "qBitLineColor": [0, 0, 0], 

        // the color of the classical bit lines
        "cBitLineColor": [128, 128, 128] 
    },
    "grid":
    {
        // the width and height of each cell in the visualization grid
        "cellSize": 32, 

        // the border size at the edges of the image (white space)
        "borderSize": 32 
    },
    "measurements":
    {
        // whether the connection between a measurement and its classical operand should be shown
        "drawConnection": true, 
        
        // controls the space between the double line connecting a measurement to its classical operand
        "lineSpacing": 2, 

        // the size of the arrow at the end of the connection line that point to its classical operand
        "arrowSize": 10 
    },
    "instructions":
        // discussed in the next section


Custom gates
------------

When using custom gates the default gate visualizations are not used and the visualization needs to be defined by the user. In the ``instructions``
section of the visualizater configuration file, each instruction 'type' has its own corresponding description of gate visualization parameters.
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

The colors are defined as RGB arrays: `[R, G, B]`.

The type of the nodes can be one of the following:

* ``NONE``: the node will not be visible
* ``GATE``: a square representing a gate
* ``CONTROL``: a small filled circle
* ``NOT``: a circle outline with cross inside (a CNOT cross)
* ``CROSS``: a diagonal cross

When a gate has multiple operands, each operand should have a node associated with it. Simply create as many nodes in the `nodes` array as
there are operands and define a type and visual parameters for it. Don't forget the comma to seperate each node in the array.


Future work
-----------

Features and issues on the todo-list are:

* display wait/barrier gates (not possible right now because the program passed to the visualizer does not contain these gates)
* gate connections overlap when in the same cycle
* add the classical bit number to the measurement connection when classical bit lines are grouped
* add a proper measurement symbol
* add an option to save the image and/or only generate that image without opening a window
* add option to represent each gate as a pulse instead of an abstract symbol