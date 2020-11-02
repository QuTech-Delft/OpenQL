/**
 * @file   visualizer.h
 * @date   08/2020
 * @author Tim van der Meer
 * @brief  declaration of the public visualizer interface
 */
 
#ifndef QL_VISUALIZER_H
#define QL_VISUALIZER_H

#include "program.h"
#include "gate_visual.h"

#include <cstdint>

const std::array<unsigned char, 3> white = {{ 255, 255, 255 }};
const std::array<unsigned char, 3> black = {{ 0, 0, 0 }};
const std::array<unsigned char, 3> gray = {{ 128, 128, 128 }};
const std::array<unsigned char, 3> lightblue = {{ 70, 210, 230 }};
const std::array<unsigned char, 3> purple = {{ 225, 118, 225 }};
const std::array<unsigned char, 3> green = {{ 112, 222, 90 }};
const std::array<unsigned char, 3> yellow = {{ 200, 200, 20 }};
const std::array<unsigned char, 3> red = {{ 255, 105, 97 }};

namespace ql
{

void visualize(const ql::quantum_program* program, const std::string& configPath, const std::string& waveformMappingPath);

// ----------------------------------------------- //
// -                    CYCLES                   - //
// ----------------------------------------------- //

struct CycleLabels
{
    bool show = true;
    bool inNanoSeconds = false;
    int rowHeight = 24;
    int fontHeight = 13;
    std::array<unsigned char, 3> fontColor = black;
};

struct CycleEdges
{
    bool show = true;
    std::array<unsigned char, 3> color = {{ 0, 0, 0 }};
    double alpha = 0.2;
};

struct CycleCutting
{
    bool cut = true;
    int emptyCycleThreshold = 2;
    int cutCycleWidth = 16;
    double cutCycleWidthModifier = 0.5;
};

struct Cycles
{
    CycleLabels labels;
    CycleEdges edges;
    CycleCutting cutting;
    bool compress = false;
    bool partitionCyclesWithOverlap = true;
};

// ----------------------------------------------- //
// -                  BIT LINES                  - //
// ----------------------------------------------- //

struct BitLineLabels
{
    bool show = true;
    int columnWidth = 32;
    int fontHeight = 13;
    std::array<unsigned char, 3> qbitColor = {{ 0, 0, 0 }};
    std::array<unsigned char, 3> cbitColor = {{ 128, 128, 128 }};
};

struct QuantumLines
{
    std::array<unsigned char, 3> color = {{ 0, 0, 0 }};
};

struct ClassicalLines
{
    bool show = true;
    bool group = true;
    int groupedLineGap = 2;
    std::array<unsigned char, 3> color = {{ 128, 128, 128 }};
};

struct BitLineEdges
{
    bool show = true;
    int thickness = 3;
    std::array<unsigned char, 3> color = {{ 0, 0, 0 }};
    double alpha = 0.4;
};

struct BitLines
{
    BitLineLabels labels;
    QuantumLines quantum;
    ClassicalLines classical;
    BitLineEdges edges;
};

// ----------------------------------------------- //
// -               GENERAL PARAMETERS            - //
// ----------------------------------------------- //

struct Grid
{
    int cellSize = 32;
    int borderSize = 32;
};

struct GateDurationOutlines
{
    bool show = true;
    int gap = 2;
    double fillAlpha = 0.1;
    double outlineAlpha = 0.3;
    std::array<unsigned char, 3> outlineColor = black;
};

struct Measurements
{
    bool drawConnection = true;
    int lineSpacing = 2;
    int arrowSize = 10;
};

// ----------------------------------------------- //
// -                    PULSES                   - //
// ----------------------------------------------- //

struct Pulses
{
    bool displayGatesAsPulses = false;
    int pulseRowHeightMicrowave = 32;
    int pulseRowHeightFlux = 32;
    int pulseRowHeightReadout = 32;
    std::array<unsigned char, 3> pulseColorMicrowave = {{ 0, 0, 255 }};
    std::array<unsigned char, 3> pulseColorFlux = {{ 255, 0, 0 }};
    std::array<unsigned char, 3> pulseColorReadout = {{ 0, 255, 0 }};
};

// ----------------------------------------------- //
// -                    LAYOUT                   - //
// ----------------------------------------------- //

struct Layout
{
    Cycles cycles;
    BitLines bitLines;
    Grid grid;
    GateDurationOutlines gateDurationOutlines;
    Measurements measurements;
    Pulses pulses;

    std::map<std::string, GateVisual> customGateVisuals;

	const std::map<ql::gate_type_t, GateVisual> defaultGateVisuals
	{
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

} // ql

#endif //QL_VISUALIZER_H