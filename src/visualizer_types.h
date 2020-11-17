/**
 * @file   visualizer_types.h
 * @date   11/2020
 * @author Tim van der Meer
 * @brief  declaration of the common types used throughout the visualizer
 */
 
#pragma once

#ifdef WITH_VISUALIZER

#include "gate_visual.h"

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

typedef std::array<unsigned char, 3> Color;

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
    long x0;
    long y0;
    long x1;
    long y1;

    Position4() = delete;
};

struct Position2 {
    long x;
    long y;

    Position2() = delete;
};

struct EndPoints {
    const int start;
    const int end;
};

struct Dimensions {
    const int width;
    const int height;
};

struct GateOperand {
    BitType bitType;
    int index;

    friend bool operator<(const GateOperand &lhs, const GateOperand &rhs) {
        if (lhs.bitType == QUANTUM && rhs.bitType == CLASSICAL) return true;
        if (lhs.bitType == CLASSICAL && rhs.bitType == QUANTUM) return false;
        return lhs.index < rhs.index;
    }

    friend bool operator>(const GateOperand &lhs, const GateOperand &rhs) {return operator<(rhs, lhs);}
    friend bool operator<=(const GateOperand &lhs, const GateOperand &rhs) {return !operator>(lhs, rhs);}
    friend bool operator>=(const GateOperand &lhs, const GateOperand &rhs) {return !operator<(lhs, rhs);}

    GateOperand() = delete;
};

struct GateProperties {
    std::string name;
    std::vector<int> operands;
    std::vector<int> creg_operands;
    int duration;
    int cycle;
    gate_type_t type;
    std::vector<int> codewords;
    std::string visual_type;

    GateProperties() = delete;
};

// ------------- Layout declaration -------------- //

class InteractionGraphLayout {
    private:
    bool outputDotFile = false;
    int borderWidth = 32;
    int minInteractionCircleRadius = 100;
    double interactionCircleRadiusModifier = 3.0;
    int qubitRadius = 17;
    int labelFontHeight = 13;
    Color circleOutlineColor = black;
    Color circleFillColor = white;
    Color labelColor = black;
    Color edgeColor = black;

    public:
    bool saveImage = false;

    bool isDotFileOutputEnabled() const { return outputDotFile; }
    int getBorderWidth() const { return borderWidth; }
    int getMinInteractionCircleRadius() const { return minInteractionCircleRadius; }
    double getInteractionCircleRadiusModifier() const { return interactionCircleRadiusModifier; }
    int getQubitRadius() const { return qubitRadius; }
    int getLabelFontHeight() const { return labelFontHeight; }
    Color getCircleOutlineColor() const { return circleOutlineColor; }
    Color getCircleFillColor() const { return circleFillColor; }
    Color getLabelColor() const { return labelColor; }
    Color getEdgeColor() const { return edgeColor; }
    
    void enableDotFileOutput(const bool argument) { outputDotFile = argument; }
    void setBorderWidth(const int argument) { assertPositive(argument, "borderWidth"); borderWidth = argument; }
    void setMinInteractionCircleRadius(const int argument) { assertPositive(argument, "minInteractionCircleRadius"); minInteractionCircleRadius = argument; }
    void setInteractionCircleRadiusModifier(const double argument) { assertPositive(argument, "interactionCircleRadiusModifier"); interactionCircleRadiusModifier = argument; }
    void setQubitRadius(const int argument) { assertPositive(argument, "qubitRadius"); qubitRadius = argument; }
    void setLabelFontHeight(const int argument) { assertPositive(argument, "labelFontHeight"); labelFontHeight = argument; }
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
    bool enabled = true;
    bool inNanoSeconds = false;
    int rowHeight = 24;
    int fontHeight = 13;
    Color fontColor = black;

    public:
    bool areEnabled() const { return enabled; }
    bool areInNanoSeconds() const { return inNanoSeconds; }
    int getRowHeight() const { return rowHeight; }
    int getFontHeight() const { return fontHeight; }
    Color getFontColor() const { return fontColor; }
    
    void setEnabled(const bool argument) { enabled = argument; }
    void setInNanoSeconds(const bool argument) { inNanoSeconds = argument; }
    void setRowHeight(const int argument) { assertPositive(argument, "cycles.labels.rowHeight"); rowHeight = argument; }
    void setFontHeight(const int argument) { assertPositive(argument, "cycles.labels.fontHeight"); fontHeight = argument; }
    void setFontColor(const Color argument) { fontColor = argument; }
};

class CycleEdges {
    private:
    bool enabled = true;
    std::array<unsigned char, 3> color = {{ 0, 0, 0 }};
    double alpha = 0.2;

    public:
    bool areEnabled() const { return enabled; }
    Color getColor() const { return color; }
    double getAlpha() const { return alpha; }
    
    void setEnabled(const bool argument) { enabled = argument; }
    void setColor(const Color argument) { color = argument; }
    void setAlpha(const double argument) { assertPositive(argument, "cycles.edges.alpha"); alpha = argument; }
};

class CycleCutting {
    private:
    bool enabled = true;
    int emptyCycleThreshold = 2;
    int cutCycleWidth = 16;
    double cutCycleWidthModifier = 0.5;

    public:
    bool isEnabled() const { return enabled; }
    int getEmptyCycleThreshold() const { return emptyCycleThreshold; }
    int getCutCycleWidth() const { return cutCycleWidth; }
    double getCutCycleWidthModifier() const { return cutCycleWidthModifier; }

    void setEnabled(const bool argument) { enabled = argument; }
    void setEmptyCycleThreshold(const int argument) { assertPositive(argument, "cycles.cutting.emptyCycleThreshold"); emptyCycleThreshold = argument; }
    void setCutCycleWidth(const int argument) { assertPositive(argument, "cycles.cutting.cutCycleWidth"); cutCycleWidth = argument; }
    void setCutCycleWidthModifier(const double argument) { assertPositive(argument, "cycles.cutting.cutCycleWidthModifier"); cutCycleWidthModifier = argument; }
};

class Cycles {
    private:
    bool compress = false;
    bool partitionCyclesWithOverlap = true;

    public:
    CycleLabels labels;
    CycleEdges edges;
    CycleCutting cutting;

    bool areCompressed() const { return compress; }
    bool arePartitioned() const { return partitionCyclesWithOverlap; }

    void setCompressed(const bool argument) { compress = argument; }
    void setPartitioned(const bool argument) { partitionCyclesWithOverlap = argument; }
};

// ----------------------------------------------- //
// -                  BIT LINES                  - //
// ----------------------------------------------- //

class BitLineLabels {
    private:
    bool enabled = true;
    int columnWidth = 32;
    int fontHeight = 13;
    Color qbitColor = {{ 0, 0, 0 }};
    Color cbitColor = {{ 128, 128, 128 }};

    public:
    bool areEnabled() const { return enabled; }
    int getColumnWidth() const { return columnWidth; }
    int getFontHeight() const { return fontHeight; }
    Color getQbitColor() const { return qbitColor; }
    Color getCbitColor() const { return cbitColor; }
    
    void setEnabled(const bool argument) { enabled = argument; }
    void setColumnWidth(const int argument) { assertPositive(argument, "bitLines.labels.columnWidth"); columnWidth = argument; }
    void setFontHeight(const int argument) { assertPositive(argument, "bitLines.labels.fontHeight"); fontHeight = argument; }
    void setQbitColor(const Color argument) { qbitColor = argument; }
    void setCbitColor(const Color argument) { cbitColor = argument; }
};

class QuantumLines {
    private:
    Color color = {{ 0, 0, 0 }};

    public:
    Color getColor() const { return color; }

    void setColor(const Color argument) { color = argument; }
};

class ClassicalLines {
    private:
    bool enabled = true;
    bool group = true;
    int groupedLineGap = 2;
    Color color = {{ 128, 128, 128 }};

    public:
    bool isEnabled() const { return enabled; }
    bool isGrouped() const { return group; }
    int getGroupedLineGap() const { return groupedLineGap; }
    Color getColor() const { return color; }
    
    void setEnabled(const bool argument) { enabled = argument; }
    void setGrouped(const bool argument) { group = argument; }
    void setGroupedLineGap(const int argument) { assertPositive(argument, "bitLines.classical.groupedLineGap"); groupedLineGap = argument; }
    void setColor(const Color argument) { color = argument; }
};

class BitLineEdges {
    private:
    bool enabled = true;
    int thickness = 3;
    Color color = {{ 0, 0, 0 }};
    double alpha = 0.4;

    public:
    bool areEnabled() const { return enabled; }
    int getThickness() const { return thickness; }
    Color getColor() const { return color; }
    double getAlpha() const { return alpha; }
    
    void setEnabled(const bool argument) { enabled = argument; }
    void setThickness(const int argument) { assertPositive(argument, "bitLines.edges.thickness"); thickness = argument; }
    void setColor(const Color argument) { color = argument; }
    void setAlpha(const double argument) { assertPositive(argument, "bitLines.edges.alpha"); alpha = argument; }

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
    int cellSize = 32;
    int borderSize = 32;

    public:
    int getCellSize() const { return cellSize; }
    int getBorderSize() const { return borderSize; }

    void setCellSize(const int argument) { assertPositive(argument, "grid.cellSize"); cellSize = argument; }
    void setBorderSize(const int argument) { assertPositive(argument, "grid.borderSize"); borderSize = argument; }
};

struct GateDurationOutlines {
    private:
    bool enabled = true;
    int gap = 2;
    double fillAlpha = 0.1;
    double outlineAlpha = 0.3;
    Color outlineColor = black;

    public:
    bool areEnabled() const { return enabled; }
    int getGap() const { return gap; }
    double getFillAlpha() const { return fillAlpha; }
    double getOutlineAlpha() const { return outlineAlpha; }
    Color getOutlineColor() const { return outlineColor; }

    void setEnabled(const bool argument) { enabled = argument; }
    void setGap(const int argument) { assertPositive(argument, "gateDurationOutlines.gap"); gap = argument; }
    void setFillAlpha(const double argument) { assertPositive(argument, "gateDurationOutlines.fillAlpha"); fillAlpha = argument; }
    void setOutlineAlpha(const double argument) { assertPositive(argument, "gateDurationOutlines.outlineAlpha"); outlineAlpha = argument; }
    void setOutlineColor(const Color argument) { outlineColor = argument; }
};

struct Measurements {
    private:
    bool enableConnection = true;
    int lineSpacing = 2;
    int arrowSize = 10;

    public:
    bool isConnectionEnabled() const { return enableConnection; }
    int getLineSpacing() const { return lineSpacing; }
    int getArrowSize() const { return arrowSize; }

    void enableDrawConnection(const bool argument) { enableConnection = argument; }
    void setLineSpacing(const int argument) { assertPositive(argument, "measurements.lineSpacing"); lineSpacing = argument; }
    void setArrowSize(const int argument) { assertPositive(argument, "measurements.arrowSize"); arrowSize = argument; }
};

// ----------------------------------------------- //
// -                    PULSES                   - //
// ----------------------------------------------- //

struct Pulses {
    private:
    bool enabled = false;
    int pulseRowHeightMicrowave = 32;
    int pulseRowHeightFlux = 32;
    int pulseRowHeightReadout = 32;
    Color pulseColorMicrowave = {{ 0, 0, 255 }};
    Color pulseColorFlux = {{ 255, 0, 0 }};
    Color pulseColorReadout = {{ 0, 255, 0 }};

    public:
    bool areEnabled() const { return enabled; }
    int getPulseRowHeightMicrowave() const { return pulseRowHeightMicrowave; }
    int getPulseRowHeightFlux() const { return pulseRowHeightFlux; }
    int getPulseRowHeightReadout() const { return pulseRowHeightReadout; }
    Color getPulseColorMicrowave() const { return pulseColorMicrowave; }
    Color getPulseColorFlux() const { return pulseColorFlux; }
    Color getPulseColorReadout() const { return pulseColorReadout; }

    void setEnabled(const bool argument) { enabled = argument; }
    void setPulseRowHeightMicrowave(const int argument) { assertPositive(argument, "pulses.pulseRowHeightMicrowave"); pulseRowHeightMicrowave = argument; }
    void setPulseRowHeightFlux(const int argument) { assertPositive(argument, "pulses.pulseRowHeightFlux"); pulseRowHeightFlux = argument; }
    void setPulseRowHeightReadout(const int argument) { assertPositive(argument, "pulses.pulseRowHeightReadout"); pulseRowHeightReadout = argument; }
    void setPulseColorMicrowave(const Color argument) { pulseColorMicrowave = argument; }
    void setPulseColorFlux(const Color argument) { pulseColorFlux = argument; }
    void setPulseColorReadout(const Color argument) { pulseColorReadout = argument; }
};

// ----------------------------------------------- //
// -                CIRCUIT LAYOUT               - //
// ----------------------------------------------- //

struct CircuitLayout {
    bool saveImage = false;

    Cycles cycles;
    BitLines bitLines;
    Grid grid;
    GateDurationOutlines gateDurationOutlines;
    Measurements measurements;
    Pulses pulses;

    std::map<std::string, GateVisual> customGateVisuals;

    const std::map<ql::gate_type_t, GateVisual> defaultGateVisuals {
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