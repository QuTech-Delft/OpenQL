/**
 * @file   visualizer_types.h
 * @date   11/2020
 * @author Tim van der Meer
 * @brief  declaration of the common types used throughout the visualizer
 */
 
#pragma once

#ifdef WITH_VISUALIZER

#include "visualizer.h"
#include "gate.h"
#include "gate_visual.h"
#include "utils/num.h"
#include "utils/str.h"
#include "utils/vec.h"
#include "utils/map.h"

namespace ql {

// enum class VisualizationType {

//     /**
//      * Visualize the quantum program as a circuit composed of abstract gates, or
//      * waveforms acting on qubits and classical registers
//      */
//     CIRCUIT,

//     /**
//      * Visualize the quantum program as a qubit interaction graph, with the
//      * labels of edges between qubits indicating the number of interactions.
//      */
//     INTERACTION_GRAPH
// };

typedef std::array<utils::Byte, 3> Color;

const Color white = {{ 255, 255, 255 }};
const Color black = {{ 0, 0, 0 }};
const Color gray = {{ 128, 128, 128 }};
const Color lightblue = {{ 70, 210, 230 }};
const Color purple = {{ 225, 118, 225 }};
const Color green = {{ 112, 222, 90 }};
const Color yellow = {{ 200, 200, 20 }};
const Color red = {{ 255, 105, 97 }};

enum BitType {CLASSICAL, QUANTUM};

struct Position4 {
    utils::Int x0;
    utils::Int y0;
    utils::Int x1;
    utils::Int y1;

    Position4() = delete;
};

struct Position2 {
    utils::Int x;
    utils::Int y;

    Position2() = delete;
};

struct EndPoints {
    utils::Int start;
    utils::Int end;

    EndPoints() = delete;
};

struct Dimensions {
    utils::Int width;
    utils::Int height;

    Dimensions() = delete;
};

struct GateOperand {
    BitType bitType;
    utils::Int index;

    friend utils::Bool operator<(const GateOperand &lhs, const GateOperand &rhs) {
        if (lhs.bitType == QUANTUM && rhs.bitType == CLASSICAL) return true;
        if (lhs.bitType == CLASSICAL && rhs.bitType == QUANTUM) return false;
        return lhs.index < rhs.index;
    }

    friend utils::Bool operator>(const GateOperand &lhs, const GateOperand &rhs) {return operator<(rhs, lhs);}
    friend utils::Bool operator<=(const GateOperand &lhs, const GateOperand &rhs) {return !operator>(lhs, rhs);}
    friend utils::Bool operator>=(const GateOperand &lhs, const GateOperand &rhs) {return !operator<(lhs, rhs);}

    GateOperand() = delete;
};

struct GateProperties {
    utils::Str name;
    utils::Vec<utils::Int> operands;
    utils::Vec<utils::Int> creg_operands;
    utils::Int duration;
    utils::Int cycle;
    gate_type_t type;
    utils::Vec<utils::Int> codewords; // std::vector<size_t> codewords; // index 0 is right and index 1 is left, in case of multi-qubit gate
    utils::Str visual_type;
    utils::Int virtual_qubit_index; // only used for remap gates

    GateProperties() = delete;
};

// ------------- Layout declaration -------------- //

class InteractionGraphLayout {
private:
    utils::Bool outputDotFile = false;
    utils::Int borderWidth = 32;
    utils::Int minInteractionCircleRadius = 100;
    utils::Real interactionCircleRadiusModifier = 3.0;
    utils::Int qubitRadius = 17;
    utils::Int labelFontHeight = 13;
    Color circleOutlineColor = black;
    Color circleFillColor = white;
    Color labelColor = black;
    Color edgeColor = black;

public:
    utils::Bool saveImage = false;

    utils::Bool isDotFileOutputEnabled() const { return outputDotFile; }
    utils::Int getBorderWidth() const { return borderWidth; }
    utils::Int getMinInteractionCircleRadius() const { return minInteractionCircleRadius; }
    utils::Real getInteractionCircleRadiusModifier() const { return interactionCircleRadiusModifier; }
    utils::Int getQubitRadius() const { return qubitRadius; }
    utils::Int getLabelFontHeight() const { return labelFontHeight; }
    Color getCircleOutlineColor() const { return circleOutlineColor; }
    Color getCircleFillColor() const { return circleFillColor; }
    Color getLabelColor() const { return labelColor; }
    Color getEdgeColor() const { return edgeColor; }
    
    void enableDotFileOutput(const utils::Bool argument) { outputDotFile = argument; }
    void setBorderWidth(const utils::Int argument) { assertPositive(argument, "borderWidth"); borderWidth = argument; }
    void setMinInteractionCircleRadius(const utils::Int argument) { assertPositive(argument, "minInteractionCircleRadius"); minInteractionCircleRadius = argument; }
    void setInteractionCircleRadiusModifier(const utils::Real argument) { assertPositive(argument, "interactionCircleRadiusModifier"); interactionCircleRadiusModifier = argument; }
    void setQubitRadius(const utils::Int argument) { assertPositive(argument, "qubitRadius"); qubitRadius = argument; }
    void setLabelFontHeight(const utils::Int argument) { assertPositive(argument, "labelFontHeight"); labelFontHeight = argument; }
    void setCircleOutlineColor(const Color argument) { circleOutlineColor = argument; }
    void setCircleFillColor(const Color argument) { circleFillColor = argument; }
    void setLabelColor(const Color argument) { labelColor = argument; }
    void setEdgeColor(const Color argument) { edgeColor = argument; }
};

// ----------------------------------------------- //
// -                    CYCLES                   - //
// ----------------------------------------------- //

class CycleLabels {
private:
    utils::Bool enabled = true;
    utils::Bool inNanoSeconds = false;
    utils::Int rowHeight = 24;
    utils::Int fontHeight = 13;
    Color fontColor = black;

public:
    utils::Bool areEnabled() const { return enabled; }
    utils::Bool areInNanoSeconds() const { return inNanoSeconds; }
    utils::Int getRowHeight() const { return rowHeight; }
    utils::Int getFontHeight() const { return fontHeight; }
    Color getFontColor() const { return fontColor; }
    
    void setEnabled(const utils::Bool argument) { enabled = argument; }
    void setInNanoSeconds(const utils::Bool argument) { inNanoSeconds = argument; }
    void setRowHeight(const utils::Int argument) { assertPositive(argument, "cycles.labels.rowHeight"); rowHeight = argument; }
    void setFontHeight(const utils::Int argument) { assertPositive(argument, "cycles.labels.fontHeight"); fontHeight = argument; }
    void setFontColor(const Color argument) { fontColor = argument; }
};

class CycleEdges {
private:
    utils::Bool enabled = true;
    Color color = black;
    utils::Real alpha = 0.2;

public:
    utils::Bool areEnabled() const { return enabled; }
    Color getColor() const { return color; }
    utils::Real getAlpha() const { return alpha; }
    
    void setEnabled(const utils::Bool argument) { enabled = argument; }
    void setColor(const Color argument) { color = argument; }
    void setAlpha(const utils::Real argument) { assertPositive(argument, "cycles.edges.alpha"); alpha = argument; }
};

class CycleCutting {
private:
    utils::Bool enabled = true;
    utils::Int emptyCycleThreshold = 2;
    utils::Int cutCycleWidth = 16;
    utils::Real cutCycleWidthModifier = 0.5;

public:
    utils::Bool isEnabled() const { return enabled; }
    utils::Int getEmptyCycleThreshold() const { return emptyCycleThreshold; }
    utils::Int getCutCycleWidth() const { return cutCycleWidth; }
    utils::Real getCutCycleWidthModifier() const { return cutCycleWidthModifier; }

    void setEnabled(const utils::Bool argument) { enabled = argument; }
    void setEmptyCycleThreshold(const utils::Int argument) { assertPositive(argument, "cycles.cutting.emptyCycleThreshold"); emptyCycleThreshold = argument; }
    void setCutCycleWidth(const utils::Int argument) { assertPositive(argument, "cycles.cutting.cutCycleWidth"); cutCycleWidth = argument; }
    void setCutCycleWidthModifier(const utils::Real argument) { assertPositive(argument, "cycles.cutting.cutCycleWidthModifier"); cutCycleWidthModifier = argument; }
};

class Cycles {
private:
    utils::Bool compress = false;
    utils::Bool partitionCyclesWithOverlap = true;

public:
    CycleLabels labels;
    CycleEdges edges;
    CycleCutting cutting;

    utils::Bool areCompressed() const { return compress; }
    utils::Bool arePartitioned() const { return partitionCyclesWithOverlap; }

    void setCompressed(const utils::Bool argument) { compress = argument; }
    void setPartitioned(const utils::Bool argument) { partitionCyclesWithOverlap = argument; }
};

// ----------------------------------------------- //
// -                  BIT LINES                  - //
// ----------------------------------------------- //

class BitLineLabels {
private:
    utils::Bool enabled = true;
    utils::Int columnWidth = 32;
    utils::Int fontHeight = 13;
    Color qbitColor = black;
    Color cbitColor = gray;

public:
    utils::Bool areEnabled() const { return enabled; }
    utils::Int getColumnWidth() const { return columnWidth; }
    utils::Int getFontHeight() const { return fontHeight; }
    Color getQbitColor() const { return qbitColor; }
    Color getCbitColor() const { return cbitColor; }
    
    void setEnabled(const utils::Bool argument) { enabled = argument; }
    void setColumnWidth(const utils::Int argument) { assertPositive(argument, "bitLines.labels.columnWidth"); columnWidth = argument; }
    void setFontHeight(const utils::Int argument) { assertPositive(argument, "bitLines.labels.fontHeight"); fontHeight = argument; }
    void setQbitColor(const Color argument) { qbitColor = argument; }
    void setCbitColor(const Color argument) { cbitColor = argument; }
};

class QuantumLines {
private:
    Color color = black;

public:
    Color getColor() const { return color; }

    void setColor(const Color argument) { color = argument; }
};

class ClassicalLines {
private:
    utils::Bool enabled = true;
    utils::Bool group = true;
    utils::Int groupedLineGap = 2;
    Color color = gray;

public:
    utils::Bool isEnabled() const { return enabled; }
    utils::Bool isGrouped() const { return group; }
    utils::Int getGroupedLineGap() const { return groupedLineGap; }
    Color getColor() const { return color; }
    
    void setEnabled(const utils::Bool argument) { enabled = argument; }
    void setGrouped(const utils::Bool argument) { group = argument; }
    void setGroupedLineGap(const utils::Int argument) { assertPositive(argument, "bitLines.classical.groupedLineGap"); groupedLineGap = argument; }
    void setColor(const Color argument) { color = argument; }
};

class BitLineEdges {
private:
    utils::Bool enabled = true;
    utils::Int thickness = 3;
    Color color = black;
    utils::Real alpha = 0.4;

public:
    utils::Bool areEnabled() const { return enabled; }
    utils::Int getThickness() const { return thickness; }
    Color getColor() const { return color; }
    utils::Real getAlpha() const { return alpha; }
    
    void setEnabled(const utils::Bool argument) { enabled = argument; }
    void setThickness(const utils::Int argument) { assertPositive(argument, "bitLines.edges.thickness"); thickness = argument; }
    void setColor(const Color argument) { color = argument; }
    void setAlpha(const utils::Real argument) { assertPositive(argument, "bitLines.edges.alpha"); alpha = argument; }

};

class BitLines {
public:
    BitLineLabels labels;
    QuantumLines quantum;
    ClassicalLines classical;
    BitLineEdges edges;
};

// ----------------------------------------------- //
// -               GENERAL PARAMETERS            - //
// ----------------------------------------------- //

struct Grid {
private:
    utils::Int cellSize = 32;
    utils::Int borderSize = 32;

public:
    utils::Int getCellSize() const { return cellSize; }
    utils::Int getBorderSize() const { return borderSize; }

    void setCellSize(const utils::Int argument) { assertPositive(argument, "grid.cellSize"); cellSize = argument; }
    void setBorderSize(const utils::Int argument) { assertPositive(argument, "grid.borderSize"); borderSize = argument; }
};

struct GateDurationOutlines {
private:
    utils::Bool enabled = true;
    utils::Int gap = 2;
    utils::Real fillAlpha = 0.1;
    utils::Real outlineAlpha = 0.3;
    Color outlineColor = black;

public:
    utils::Bool areEnabled() const { return enabled; }
    utils::Int getGap() const { return gap; }
    utils::Real getFillAlpha() const { return fillAlpha; }
    utils::Real getOutlineAlpha() const { return outlineAlpha; }
    Color getOutlineColor() const { return outlineColor; }

    void setEnabled(const utils::Bool argument) { enabled = argument; }
    void setGap(const utils::Int argument) { assertPositive(argument, "gateDurationOutlines.gap"); gap = argument; }
    void setFillAlpha(const utils::Real argument) { assertPositive(argument, "gateDurationOutlines.fillAlpha"); fillAlpha = argument; }
    void setOutlineAlpha(const utils::Real argument) { assertPositive(argument, "gateDurationOutlines.outlineAlpha"); outlineAlpha = argument; }
    void setOutlineColor(const Color argument) { outlineColor = argument; }
};

struct Measurements {
private:
    utils::Bool enableConnection = true;
    utils::Int lineSpacing = 2;
    utils::Int arrowSize = 10;

public:
    utils::Bool isConnectionEnabled() const { return enableConnection; }
    utils::Int getLineSpacing() const { return lineSpacing; }
    utils::Int getArrowSize() const { return arrowSize; }

    void enableDrawConnection(const utils::Bool argument) { enableConnection = argument; }
    void setLineSpacing(const utils::Int argument) { assertPositive(argument, "measurements.lineSpacing"); lineSpacing = argument; }
    void setArrowSize(const utils::Int argument) { assertPositive(argument, "measurements.arrowSize"); arrowSize = argument; }
};

// ----------------------------------------------- //
// -                    PULSES                   - //
// ----------------------------------------------- //

struct Pulses {
private:
    utils::Bool enabled = false;
    utils::Int pulseRowHeightMicrowave = 32;
    utils::Int pulseRowHeightFlux = 32;
    utils::Int pulseRowHeightReadout = 32;
    Color pulseColorMicrowave = {{ 0, 0, 255 }};
    Color pulseColorFlux = {{ 255, 0, 0 }};
    Color pulseColorReadout = {{ 0, 255, 0 }};

public:
    utils::Bool areEnabled() const { return enabled; }
    utils::Int getPulseRowHeightMicrowave() const { return pulseRowHeightMicrowave; }
    utils::Int getPulseRowHeightFlux() const { return pulseRowHeightFlux; }
    utils::Int getPulseRowHeightReadout() const { return pulseRowHeightReadout; }
    Color getPulseColorMicrowave() const { return pulseColorMicrowave; }
    Color getPulseColorFlux() const { return pulseColorFlux; }
    Color getPulseColorReadout() const { return pulseColorReadout; }

    void setEnabled(const utils::Bool argument) { enabled = argument; }
    void setPulseRowHeightMicrowave(const utils::Int argument) { assertPositive(argument, "pulses.pulseRowHeightMicrowave"); pulseRowHeightMicrowave = argument; }
    void setPulseRowHeightFlux(const utils::Int argument) { assertPositive(argument, "pulses.pulseRowHeightFlux"); pulseRowHeightFlux = argument; }
    void setPulseRowHeightReadout(const utils::Int argument) { assertPositive(argument, "pulses.pulseRowHeightReadout"); pulseRowHeightReadout = argument; }
    void setPulseColorMicrowave(const Color argument) { pulseColorMicrowave = argument; }
    void setPulseColorFlux(const Color argument) { pulseColorFlux = argument; }
    void setPulseColorReadout(const Color argument) { pulseColorReadout = argument; }
};

// ----------------------------------------------- //
// -                CIRCUIT LAYOUT               - //
// ----------------------------------------------- //

struct CircuitLayout {
    utils::Bool saveImage = false;

    Cycles cycles;
    BitLines bitLines;
    Grid grid;
    GateDurationOutlines gateDurationOutlines;
    Measurements measurements;
    Pulses pulses;

    utils::Map<utils::Str, GateVisual> customGateVisuals;

    const utils::Map<gate_type_t, GateVisual> defaultGateVisuals {
        // TODO: use the proper symbol for dagger gates
        // TODO: use the proper symbol for measurement gates

        {ql::__identity_gate__, { black, {
            { GATE, 13, "I", 13, white, lightblue, lightblue }}}},

        {ql::__hadamard_gate__,	{ black, {
            { GATE, 13, "H", 13, white, lightblue, lightblue }}}},

        {ql::__pauli_x_gate__, { black, {
            { GATE, 13, "X", 13, white, green, green }}}},

        {ql::__pauli_y_gate__, { black, {
            { GATE, 13, "Y", 13, white, green, green }}}},

        {ql::__pauli_z_gate__, { black, {
            { GATE, 13, "Z", 13, white, green, green }}}},

        {ql::__phase_gate__, { black, {
            { GATE, 13, "S", 13, white, yellow, yellow }}}},

        {ql::__phasedag_gate__, { black, {
            { GATE, 13, "S\u2020", 13, white, yellow, yellow }}}},

        {ql::__t_gate__, { black, {
            { GATE, 13, "T", 13, white, red, red }}}},

        {ql::__tdag_gate__, { black, {
            { GATE, 13, "T\u2020", 13, white, red, red }}}},

        {ql::__rx90_gate__, { black, {
            {}}}},
        {ql::__mrx90_gate__, { black, {
            {}}}},
        {ql::__rx180_gate__, { black, {
            {}}}},
        {ql::__ry90_gate__, { black, {
            {}}}},
        {ql::__mry90_gate__, { black, {
            {}}}},
        {ql::__ry180_gate__, { black, {
            {}}}},
        {ql::__rx_gate__, { black, {
            {}}}},
        {ql::__ry_gate__, { black, {
            {}}}},
        {ql::__rz_gate__, { black, {
            {}}}},
        {ql::__prepz_gate__, { black, {
            {}}}},

        {ql::__cnot_gate__,	{ black, {
            { CONTROL, 3, "", 0, black, black, black },
            { NOT, 8, "", 0, black, black, black }}}},

        {ql::__cphase_gate__, { lightblue, {
            { CONTROL, 3, "", 0, black, lightblue, lightblue },
            { CONTROL, 3, "", 0, black, lightblue, lightblue }}}},

        {ql::__toffoli_gate__, { black, {
            {}}}},
        {ql::__custom_gate__, { black, {
            {}}}},
        {ql::__composite_gate__, { black, {
            {}}}},

        {ql::__measure_gate__, { gray, {
            { GATE, 13, "M", 13, white, purple, purple },
            { NONE, 3, "", 0, black, black, black }}}},

        {ql::__display__, { black, {
            {}}}},
        {ql::__display_binary__, { black, {
            {}}}},
        {ql::__nop_gate__, { black, {
            {}}}},
        {ql::__dummy_gate__, { black, {
            {}}}},

        {ql::__swap_gate__, { black, {
            { CROSS, 6, "", 0, black, black, black },
            { CROSS, 6, "", 0, black, black, black }}}},

        {ql::__wait_gate__, { black, {
            {}}}},
        {ql::__classical_gate__, { black, {
            {}}}}
    };
};

} // namespace ql

#endif //WITH_VISUALIZER