/** \file
 * Declaration of the public visualizer interface.
 */
 
#pragma once

#include "utils/num.h"
#include "utils/str.h"
#include "utils/vec.h"
#include "utils/map.h"
#include "program.h"
#include "gate_visual.h"

#include <cstdint>

namespace ql {

const Color white = {{ 255, 255, 255 }};
const Color black = {{ 0, 0, 0 }};
const Color gray = {{ 128, 128, 128 }};
const Color lightblue = {{ 70, 210, 230 }};
const Color purple = {{ 225, 118, 225 }};
const Color green = {{ 112, 222, 90 }};
const Color yellow = {{ 200, 200, 20 }};
const Color red = {{ 255, 105, 97 }};

static const utils::Int MAX_ALLOWED_VISUALIZER_CYCLE = 2000;

struct VisualizerConfigurationPaths {
    const utils::Str &config;
    const utils::Str &waveformMapping;
};

void visualize(const quantum_program *program, const utils::Str &visualizationType, const VisualizerConfigurationPaths &configurationPaths);

void assertPositive(utils::Int argument, const utils::Str &parameter);
void assertPositive(utils::Real argument, const utils::Str &parameter);

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
    Color color = {{ 0, 0, 0 }};
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
    Color qbitColor = {{ 0, 0, 0 }};
    Color cbitColor = {{ 128, 128, 128 }};

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
    Color color = {{ 0, 0, 0 }};

public:
    Color getColor() const { return color; }

    void setColor(const Color argument) { color = argument; }
};

class ClassicalLines {
private:
    utils::Bool enabled = true;
    utils::Bool group = true;
    utils::Int groupedLineGap = 2;
    Color color = {{ 128, 128, 128 }};

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
    Color color = {{ 0, 0, 0 }};
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
// -                    LAYOUT                   - //
// ----------------------------------------------- //

struct Layout {
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

        {__identity_gate__, { black, {
            { GATE, 13, "I", 13, white, lightblue, lightblue }}}},

        {__hadamard_gate__,	{ black, {
            { GATE, 13, "H", 13, white, lightblue, lightblue }}}},

        {__pauli_x_gate__, { black, {
            { GATE, 13, "X", 13, white, green, green }}}},

        {__pauli_y_gate__, { black, {
            { GATE, 13, "Y", 13, white, green, green }}}},

        {__pauli_z_gate__, { black, {
            { GATE, 13, "Z", 13, white, green, green }}}},

        {__phase_gate__, { black, {
            { GATE, 13, "S", 13, white, yellow, yellow }}}},

        {__phasedag_gate__, { black, {
            { GATE, 13, "S\u2020", 13, white, yellow, yellow }}}},

        {__t_gate__, { black, {
            { GATE, 13, "T", 13, white, red, red }}}},

        {__tdag_gate__, { black, {
            { GATE, 13, "T\u2020", 13, white, red, red }}}},

        {__rx90_gate__, { black, {
            {}}}},
        {__mrx90_gate__, { black, {
            {}}}},
        {__rx180_gate__, { black, {
            {}}}},
        {__ry90_gate__, { black, {
            {}}}},
        {__mry90_gate__, { black, {
            {}}}},
        {__ry180_gate__, { black, {
            {}}}},
        {__rx_gate__, { black, {
            {}}}},
        {__ry_gate__, { black, {
            {}}}},
        {__rz_gate__, { black, {
            {}}}},
        {__prepz_gate__, { black, {
            {}}}},

        {__cnot_gate__,	{ black, {
            { CONTROL, 3, "", 0, black, black, black },
            { NOT, 8, "", 0, black, black, black }}}},

        {__cphase_gate__, { lightblue, {
            { CONTROL, 3, "", 0, black, lightblue, lightblue },
            { CONTROL, 3, "", 0, black, lightblue, lightblue }}}},

        {__toffoli_gate__, { black, {
            {}}}},
        {__custom_gate__, { black, {
            {}}}},
        {__composite_gate__, { black, {
            {}}}},

        {__measure_gate__, { gray, {
            { GATE, 13, "M", 13, white, purple, purple },
            { NONE, 3, "", 0, black, black, black }}}},

        {__display__, { black, {
            {}}}},
        {__display_binary__, { black, {
            {}}}},
        {__nop_gate__, { black, {
            {}}}},
        {__dummy_gate__, { black, {
            {}}}},

        {__swap_gate__, { black, {
            { CROSS, 6, "", 0, black, black, black },
            { CROSS, 6, "", 0, black, black, black }}}},

        {__wait_gate__, { black, {
            {}}}},
        {__classical_gate__, { black, {
            {}}}}
    };
};

} // namespace ql