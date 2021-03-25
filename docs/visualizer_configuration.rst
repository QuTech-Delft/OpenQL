.. _visualizer_configuration:

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