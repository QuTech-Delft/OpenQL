.. _visualizer_configuration:

========================
Visualizer configuration
========================

The visualizer is configured by way of the visualizer configuration file. Each attribute has a default setting, so many can be omitted if no change
is wanted.

--------------------------------
General visualization parameters
--------------------------------

The visualizer configuration file has several top-level sections:

* ``circuit``: contains options for the circuit visualization, including pulse visualization
* ``interactionGraph``: contains options for the interaction graph
* ``mappingGraph``: contains options for the mapping graph

In addition there are also two top-level attributes:

* ``saveImage``: whether the generated image should be saved to disk
* ``backgroundColor``: the background color of the generated image

Each of the three top-level sections will be discussed below.

---------------------
Circuit visualization
---------------------

The ``circuit`` section has several child sections:

* ``cycles``: contains parameters that govern cycle labels, edges, cycle compression and cutting
* ``bitLines``: defines the labels and lines, including grouping lines for both quantum and classical bitLines
* ``grid``: defines several parameters of the image grid
* ``gateDurationOutlines``: controls parameters for gate duration outlines
* ``measurements``: several parameters controlling measurement visualization
* ``pulses``: parameters for pulse visualization
* ``instructions``: a map of instruction types (the keys) with that type's gate visualization as value, used for custom instructions

Example configuration (self-explanatory attributes have no description):

.. code:: javascript

    "cycles":
    {
        // parameters for the labels above each cycle
        "labels":
        {
            "show": true,
            // whether the cycle labels should be shown in nanoseconds or cycle numbers
            "inNanoSeconds": false,
            // the height of the cycle label row
            "rowHeight": 24,
            "fontHeight": 13,
            "fontColor": [0, 0, 0]
        },
        // parameters for the vertical edges between cycles
        "edges":
        {
            "show": true,
            "color": [0, 0, 0],
            "alpha": 0.2
        },
        // parameters for the cutting of cycles (cycles are cut when no new gates are started)
        "cutting":
        {
            "cut": true,
            // how many cycles should be without a gate starting before the cycle is cut
            "emptyCycleThreshold": 2,
            "cutCycleWidth": 16,
            // a multiplier on the width of the cut cycles
            "cutCycleWidthModifier": 0.5
        },
        // cycles are compressed by reducing each gate's duration to one cycle
        "compressCycles": false,
        // partioning a cycle means that each gate in that cycle gets its own column within the cycle
        // this can be done to remove visual overlap
        "partitionCyclesWithOverlap": true
    },
    "bitLines":
    {
        // parameters for the labels on each quantum or classical bit line
        "labels":
        {
            "show": true,
            // the width of the label column
            "columnWidth": 32,
            "fontHeight": 13,
            // the colors of quantum and classical bit labels
            "qbitColor": [0, 0, 0],
            "cbitColor": [128, 128, 128]
        },
        // parameters specifically for quantum bit lines
        "quantum":
        {
            "color": [0, 0, 0]
        },
        // parameters specifically for classical bit lines
        "classical":
        {
            "show": true,
            // grouping classical bit lines collapses them into a double line to reduce visual clutter
            "group": false,
            // controls the gap between the double line indicating the collapsed classical lines
            "groupedLineGap": 2,
            "color": [128, 128, 128]
        },
        // parameters for the horizontal edges between bit lines
        "edges":
        {
            "show": false,
            "thickness": 5,
            "color": [0, 0, 0],
            "alpha": 0.4
        }
    },
    "grid":
    {
        // the size of each cell formed by a the crossing of a single bit line and cycle
        "cellSize": 32,
        // the border at the edges of the generated image
        "borderSize": 32
    },
    "gateDurationOutlines":
    {
        "show": true,
        "gap": 2,
        "fillAlpha": 0.2,
        "outlineAlpha": 0.3,
        "outlineColor": [0, 0, 0]
    },