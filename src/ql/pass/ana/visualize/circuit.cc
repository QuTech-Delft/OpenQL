/** \file
 * Defines the circuit visualizer pass.
 */

#include "ql/pass/ana/visualize/circuit.h"

#include "detail/circuit.h"
#include "ql/pmgr/factory.h"

namespace ql {
namespace pass {
namespace ana {
namespace visualize {
namespace circuit {

bool VisualizeCircuitPass::is_pass_registered = pmgr::Factory::register_pass<VisualizeCircuitPass>("ana.visualize.Circuit");

/**
 * Dumps docs for the circuit visualizer.
 */
void VisualizeCircuitPass::dump_docs(
    std::ostream &os,
    const utils::Str &line_prefix
) const {
    utils::dump_str(os, line_prefix,
#ifndef WITH_VISUALIZER
    R"(
    NOTE*: the visualizer was not compiled into the build of OpenQL that
    generated this documentation. If this was not intended, and OpenQL is
    running on Linux or Mac, the X11 library development headers might be
    missing and the visualizer has disabled itself.

    )"
#endif
    R"(
    The circuit visualizer produces an image of the circuit containing the
    operations on each qubit per cycle. If so configured, it can also render
    instrument waveforms alongside it.

    * Configuration file structure *

      The visualizer is configured by way of the visualizer configuration file.
      Each attribute has a default setting, so many can be omitted if no change
      is wanted.

      The circuit visualizer supports the following top-level sections:

       - `"circuit"`: contains options for the circuit visualization, including
         pulse visualization.
       - `"saveImage"`: a boolean indicating whether the generated image should
         be saved to disk. When this is true, the file will be saved regardless
         of/in addition to the interactive window as controlled by the
         `interactive` option.
       - `"backgroundColor"`: the background color of the generated image.

      NOTE: a single visualizer configuration file may be used for all three
      visualization pass types. The configuration file format is designed to be
      cross-compatible.

      NOTE: when the to-be-visualized circuit is very large, the interactive
      window may have trouble rendering the circuit even when zoomed in.
      Therefore, it is recommended to use non-interactive mode and view the
      generated bitmap with a more capable external viewer.

      The `"circuit"` section has several child sections.

       - `"cycles"`: contains parameters that govern cycle labels, edges, cycle
         compression and cutting.
       - `"bitLines"`: defines the labels and lines, including grouping lines
         for both quantum and classical bitLines.
       - `"grid"`: defines several parameters of the image grid.
       - `"gateDurationOutlines"`: controls parameters for gate duration
         outlines.
       - `"measurements"`: several parameters controlling measurement
         visualization.
       - `"pulses"`: parameters for pulse visualization.
       - `"instructions"`: a map of instruction types (the keys) with that
         type's gate visualization as value, used for custom instructions.
)" R"(
      Example configuration (self-explanatory attributes have no description):

      ```javascript
      "cycles": {
          // parameters for the labels above each cycle
          "labels": {
              "show": true,
              // whether the cycle labels should be shown in nanoseconds or
              // cycle numbers
              "inNanoSeconds": false,
              // the height of the cycle label row
              "rowHeight": 24,
              "fontHeight": 13,
              "fontColor": [0, 0, 0]
          },
          // parameters for the vertical edges between cycles
          "edges": {
              "show": true,
              "color": [0, 0, 0],
              "alpha": 0.2
          },
          // parameters for the cutting of cycles (cycles are cut when no new
          // gates are started)
          "cutting": {
              "cut": true,
              // how many cycles should be without a gate starting before the
              // cycle is cut
              "emptyCycleThreshold": 2,
              "cutCycleWidth": 16,
              // a multiplier on the width of the cut cycles
              "cutCycleWidthModifier": 0.5
          },
          // cycles are compressed by reducing each gate's duration to one cycle
          "compressCycles": false,
          // partitioning a cycle means that each gate in that cycle gets its
          // own column within the cycle; this can be done to remove visual
          // overlap
          "partitionCyclesWithOverlap": true
      },)" R"(
      "bitLines": {
          // parameters for the labels on each quantum or classical bit line
          "labels": {
              "show": true,
              // the width of the label column
              "columnWidth": 32,
              "fontHeight": 13,
              // the colors of quantum and classical bit labels
              "qbitColor": [0, 0, 0],
              "cbitColor": [128, 128, 128]
          },
          // parameters specifically for quantum bit lines
          "quantum": {
              "color": [0, 0, 0]
          },
          // parameters specifically for classical bit lines
          "classical": {
              "show": true,
              // grouping classical bit lines collapses them into a double line
              // to reduce visual clutter
              "group": false,
              // controls the gap between the double line indicating the
              // collapsed classical lines
              "groupedLineGap": 2,
              "color": [128, 128, 128]
          },
          // parameters for the horizontal edges between bit lines
          "edges": {
              "show": false,
              "thickness": 5,
              "color": [0, 0, 0],
              "alpha": 0.4
          }
      },)" R"(
      "grid": {
          // the size of each cell formed by a the crossing of a single bit line
          // and cycle
          "cellSize": 32,
          // the border at the edges of the generated image
          "borderSize": 32
      },
      "gateDurationOutlines": {
          "show": true,
          // the gap between the edge of the cell and the gate duration outline
          "gap": 2,
          // the filled background alpha
          "fillAlpha": 0.2,
          // the outline alpha
          "outlineAlpha": 0.3,
          "outlineColor": [0, 0, 0]
      },
      "measurements": {
          // whether to draw a connection from the measurement gate to the
          // classical line it stores the result in
          "drawConnection": true,
          // the gap between the double line representing the connection
          "lineSpacing": 2,
          "arrowSize": 10
      },
      "pulses": {
          // set this to true to use the pulse visualization
          "displayGatesAsPulses": false,
          // these heights control the line row heights
          "pulseRowHeightMicrowave": 32,
          "pulseRowHeightFlux": 32,
          "pulseRowHeightReadout": 32,
          // these colors control the line colors
          "pulseColorMicrowave": [0, 0, 255],
          "pulseColorFlux": [255, 0, 0],
          "pulseColorReadout": [0, 255, 0]
      },
      "instructions" {
          // defined below
      }
      ```
)" R"(
    * Gate visualization *

      A visualization needs to be defined by the user for each gate type used in the circuit.
      In the instructions section of the visualizer configuration file, each instruction
      "type" has its own corresponding description of gate visualization parameters.
      These instruction types are mapped to actual custom instructions from the
      hardware configuration file by adding a `"visual_type"` key to the instructions.
      For example:

      ```
      {
          ...,
          "instructions" {
              ...,
              "h q1": {
                  "duration": 40,
                  "qubits": ["q1"],
                  "visual_type": "h"
              },
              ...
          },
          ...
      }
      ```
)" R"(
      This custom Hadamard gate defined on qubit 1 has one additional attribute
      `"visual_type"` describing its visualization type. The value of this
      attribute links to a key in the visualizer configuration file, which has
      the description of the gate visualization parameters that will be used
      to visualize this custom instruction. Note that this allows multiple
      custom instructions to share the same visualization parameters, without
      having to duplicate the parameters.

      The `instructions` section of the visualizer configuration file then
      defines how each gate type is rendered. Here's an excerpt from an example
      configuration file:

      ```
      {
          ...,
          "instructions": {
              ...,
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
              },
              ...
          },
          ...
      }
      ```
)" R"(
      Each gate has a `"connectionColor"` which defines the color of the
      connection line for multi-operand gates, and an array of `"nodes"`. A node
      is the visualization of the gate acting on a specific qubit or classical
      bit. If a Hadamard gate is acting on qubit 3, that is represented by one
      node. If a CNOT gate is acting on qubits 1 and 2, it will have two nodes,
      one describing the visualization of the CNOT gate at qubit 1 and one
      describing the visualization on qubit 2. A measurement gate measuring
      qubit 5 and storing the result in classical bit 0 will again have two
      nodes.

      Each node has several attributes describing its visualization.

       - `"type"`: the visualization type of the node, see below for a list of
         the available types.
       - `"radius"`: the radius of the node in pixels.
       - `"displayName"`: text that will be displayed on the node (for example
         `"H"` will be displayed on the Hadamard gate in the example above).
       - `"fontHeight"`: the height of the font in pixels used by the
         `"displayName"`.
       - `"fontColor"`: the color of the font used by the `"displayName"`.
       - `"backgroundColor"`: the background color of the node.
       - `"outlineColor"`: the color of the edge-line of the node.

      The colors are defined as RGB arrays: `[R, G, B]`.

      The type of the nodes can be one of the following.

       - `"NONE"`: the node will not be visible.
       - `"GATE"`: a square representing a gate.
       - `"CONTROL"`: a small filled circle.
       - `"NOT"`: a circle outline with cross inside (a CNOT cross).
       - `"CROSS"`: a diagonal cross.

      When a gate has multiple operands, each operand should have a node
      associated with it. Simply create as many nodes in the node array as there
      are operands and define a type and visual parameters for it. Don't forget
      the comma to separate each node in the array. Note that nodes are coupled
      to each operand sequentially, i.e. the first node in the node array will
      be used for the first qubit in the operand vector.
)" R"(
    * Pulse visualization *

      Along with an abstract representation of the gates used in the quantum
      circuit, the gates can also be represented by the RF pulses used in the
      real hardware. This will be done when the `"displayGatesAsPulses"` flag
      in the `"pulses"` section is set to true. In this case, the
      `waveform_mapping` option must be used to specify a waveform configuration
      file.

      Each qubit consists of three lines, the microwave, flux and readout lines,
      controlling single-qubit gates, two-qubit gates and readouts respectively.
      The waveforms used by the hardware should be stored in the waveform
      mapping configuration file. Then, in the hardware configuration file the
      `"visual_codeword"` and `"qubits"` attributes of each instruction are
      used as key into the table contained in the waveform mapping file to find
      the corresponding waveform for the specific instruction and qubit
      (waveforms for the same instruction can be different for different
      qubits). Note that a two-qubit gate has two codeword attributes, one for
      each qubit: `"visual_right_codeword"` and `"visual_left_codeword"`.

      In the waveform mapping configuration file, the waveforms are grouped by
      codeword first and then by addressed qubit. The waveforms themselves are
      stored as an array of real numbers. The scale of these numbers does not
      matter, the visualizer will automatically scale the pulses to fit inside
      the graph. The time between samples is determined by the sample rate (the
      sample rate can be different for each of the three lines).

      TODO: the structure of the waveform mapping configuration file should
      still be documented. For now, use the examples in `tests/visualizer` as
      a baseline.

    )");
}

/**
 * Returns a user-friendly type name for this pass.
 */
utils::Str VisualizeCircuitPass::get_friendly_type() const {
    return "Circuit visualizer";
}

/**
 * Constructs a circuit visualizer pass.
 */
VisualizeCircuitPass::VisualizeCircuitPass(
    const utils::Ptr<const pmgr::Factory> &pass_factory,
    const utils::Str &instance_name,
    const utils::Str &type_name
) : Analysis(pass_factory, instance_name, type_name) {
    options.add_str(
        "config",
        "Path to the visualizer configuration file.",
        "visualizer_config.json"
    );
    options.add_str(
        "waveform_mapping",
        "Path to the visualizer waveform mapping file.",
        "waveform_mapping.json"
    );
    options.add_bool(
        "interactive",
        "When `yes`, the visualizer will open a window when the pass is run. "
        "When `no`, an image will be saved as `<output_prefix>.bmp` instead."
    );
}

/**
 * Runs the circuit visualizer.
 */
utils::Int VisualizeCircuitPass::run(
    const ir::Ref &,
    const pmgr::pass_types::Context &
) const {

#ifdef WITH_VISUALIZER
    detail::visualizeCircuit(
        ir, {
            "CIRCUIT",
            options["config"].as_str(),
            options["waveform_mapping"].as_str(),
            options["interactive"].as_bool(),
            context.output_prefix,
            context.full_pass_name
        }
    );
    return 0;
#else
    QL_EOUT(
        "The visualizer was disabled during compilation of OpenQL. If this was "
        "not intended, and OpenQL is running on Linux or Mac, the X11 library "
        "development headers might be missing and the visualizer has disabled "
        "itself."
    );
    return -1;
#endif
}

} // namespace circuit
} // namespace visualize
} // namespace ana
} // namespace pass
} // namespace ql
