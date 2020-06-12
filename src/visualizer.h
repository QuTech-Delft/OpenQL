#include <cstdint>
#include <program.h>
#include <CImg.h>

namespace ql
{

enum BitType {CLASSICAL, QUANTUM};
enum NodeType {NONE, GATE, CONTROL, NOT, CROSS};

const std::array<unsigned char, 3> white = { 255, 255, 255 };
const std::array<unsigned char, 3> black = { 0, 0, 0 };
const std::array<unsigned char, 3> gray = { 128, 128, 128 };
const std::array<unsigned char, 3> lightblue = { 70, 210, 230 };
const std::array<unsigned char, 3> purple = { 225, 118, 225 };
const std::array<unsigned char, 3> green = { 112, 222, 90 };
const std::array<unsigned char, 3> yellow = { 200, 200, 20 };
const std::array<unsigned char, 3> red = { 255, 105, 97 };

struct Position4
{
	long x0;
	long y0;
	long x1;
	long y1;
};

struct Position2
{
	long x;
	long y;
};

struct NodePositionData
{
	const unsigned int gap;
	const unsigned int labelColumnWidth;
	const unsigned int cycleNumbersRowHeight;

	const unsigned int column;
	const unsigned int row;
};

struct Cycles
{
	// Cycle number row.
	bool showCycleNumbers = true;
	unsigned int rowHeight = 24;
	unsigned int fontHeight = 13;
	std::array<unsigned char, 3> fontColor = black;

	// Whether the cycles should be compressed or gate duration outlines should be shown on the qubits.
	bool compressCycles = false;
	bool showGateDurationOutline = true;
	unsigned int gateDurationGap = 2;
	float gateDurationAlpha = 0.1f;
	float gateDurationOutLineAlpha = 0.3f;
	std::array<unsigned char, 3> gateDurationOutlineColor = black;
};

struct BitLines
{
	// Labels.
	bool drawLabels = true;
	//TODO: set this to 0 automatically if drawLabels is false?
	unsigned int labelColumnWidth = 32;
	unsigned int fontHeight = 13;
	std::array<unsigned char, 3> qBitLabelColor = { 0, 0, 0 };
	std::array<unsigned char, 3> cBitLabelColor = { 128, 128, 128 };

	// Lines.
	bool showClassicalLines = true;
	bool groupClassicalLines = false;
	unsigned int groupedClassicalLineGap = 2;
	std::array<unsigned char, 3> qBitLineColor = { 0, 0, 0 };
	std::array<unsigned char, 3> cBitLineColor = { 128, 128, 128 };
};

struct Grid
{
	unsigned int cellSize = 32;
	unsigned int borderSize = 32;
};

struct Measurements
{
	bool drawConnection = false;
	unsigned int lineSpacing = 2;
	unsigned int arrowSize = 10;
};

struct Node
{
	NodeType type;

	unsigned int radius;

	std::string displayName;
	unsigned int fontHeight;
	std::array<unsigned char, 3> fontColor;

	std::array<unsigned char, 3> backgroundColor;
	std::array<unsigned char, 3> outlineColor;
};

struct GateConfig
{
	std::array<unsigned char, 3> connectionColor;
	std::vector<Node> nodes;
};

struct Layout
{
	Cycles cycles;
	BitLines bitLine;
	Grid grid;
	Measurements measurements;

	std::map<ql::gate_type_t, GateConfig> gateConfigs
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

struct CircuitData
{
	const unsigned int amountOfQubits;
	const unsigned int amountOfClassicalBits;
	const unsigned int amountOfCycles;
};

void visualize(const ql::quantum_program* program, const Layout layout);

void validateLayout(const Layout layout);

unsigned int calculateAmountOfBits(const std::vector<ql::gate*> gates, const std::vector<size_t> ql::gate::* operandType);

void drawCycleNumbers(cimg_library::CImg<unsigned char>& image, const Layout layout, const CircuitData circuitData);
void drawBitLine(cimg_library::CImg<unsigned char>& image, const Layout layout, const BitType bitType, const unsigned int row, const CircuitData circuitData);
void drawGroupedClassicalBitLine(cimg_library::CImg<unsigned char>& image, const Layout layout, const CircuitData circuitData);
void drawGate(cimg_library::CImg<unsigned char>& image, const Layout layout, const CircuitData circuitData, ql::gate* const gate);

void drawGateNode(cimg_library::CImg<unsigned char>& image, const Layout layout, const CircuitData circuitData, const Node node, const NodePositionData positionData);
void drawControlNode(cimg_library::CImg<unsigned char>& image, const Layout layout, const CircuitData circuitData, const Node node, const NodePositionData positionData);
void drawNotNode(cimg_library::CImg<unsigned char>& image, const Layout layout, const CircuitData circuitData, const Node node, const NodePositionData positionData);
void drawCrossNode(cimg_library::CImg<unsigned char>& image, const Layout layout, const CircuitData circuitData, const Node node, const NodePositionData positionData);

/*//TODO: remove these functions for release version
std::vector<ql::gate*> getTeleportationCircuit()
{
	std::vector<ql::gate*> gates;

	ql::gate* gate1 = new ql::hadamard(1);
	gate1->cycle = 0;
	gates.push_back(gate1);

	ql::gate* gate2 = new ql::cnot(1, 2);
	gate2->cycle = 1;
	gates.push_back(gate2);

	ql::gate* gate3 = new ql::cnot(0, 1);
	gate3->cycle = 3;
	gates.push_back(gate3);

	ql::gate* gate4 = new ql::hadamard(0);
	gate4->cycle = 5;
	gates.push_back(gate4);

	ql::gate* gate5 = new ql::measure(1, 0);
	gate5->cycle = 5;
	gates.push_back(gate5);

	ql::gate* gate6 = new ql::measure(0, 1);
	gate6->cycle = 6;
	gates.push_back(gate6);

	ql::gate* gate7 = new ql::cnot(1, 2);
	gate7->cycle = 7;
	gates.push_back(gate7);

	ql::gate* gate8 = new ql::cphase(0, 2);
	gate8->cycle = 9;
	gates.push_back(gate8);

	return gates;
}

std::vector<ql::gate*> getDemoCircuit1()
{
	std::vector<ql::gate*> gates;


	unsigned int cycle = 0;
	ql::gate* gate = new ql::cnot(2, 1);
	gate->cycle = cycle;
	gates.push_back(gate);


	cycle = 2;
	gate = new ql::pauli_x(1);
	gate->cycle = cycle;
	gates.push_back(gate);

	gate = new ql::hadamard(2);
	gate->cycle = cycle;
	gates.push_back(gate);

	gate = new ql::phase(3);
	gate->cycle = cycle;
	gates.push_back(gate);

	gate = new ql::pauli_y(4);
	gate->cycle = cycle;
	gates.push_back(gate);


	cycle = 3;
	gate = new ql::t(2);
	gate->cycle = cycle;
	gates.push_back(gate);

	gate = new ql::pauli_z(3);
	gate->cycle = cycle;
	gates.push_back(gate);


	cycle = 4;
	gate = new ql::tdag(2);
	gate->cycle = cycle;
	gates.push_back(gate);

	gate = new ql::pauli_z(3);
	gate->cycle = cycle;
	gates.push_back(gate);


	cycle = 5;
	gate = new ql::pauli_x(1);
	gate->cycle = cycle;
	gates.push_back(gate);

	gate = new ql::hadamard(2);
	gate->cycle = cycle;
	gates.push_back(gate);

	gate = new ql::phasedag(3);
	gate->cycle = cycle;
	gates.push_back(gate);

	gate = new ql::pauli_y(4);
	gate->cycle = cycle;
	gates.push_back(gate);


	cycle = 6;
	gate = new ql::cnot(2, 1);
	gate->cycle = cycle;
	gates.push_back(gate);


	cycle = 8;
	gate = new ql::measure(0, 0);
	gate->cycle = cycle;
	gates.push_back(gate);


	cycle = 9;
	gate = new ql::measure(1, 1);
	gate->cycle = cycle;
	gates.push_back(gate);


	cycle = 10;
	gate = new ql::measure(2, 2);
	gate->cycle = cycle;
	gates.push_back(gate);


	cycle = 11;
	gate = new ql::measure(3, 3);
	gate->cycle = cycle;
	gates.push_back(gate);


	cycle = 12;
	gate = new ql::measure(4, 4);
	gate->cycle = cycle;
	gates.push_back(gate);

	return gates;
}

std::vector<ql::gate*> getDemoCircuit2()
{
	std::vector<ql::gate*> gates;
	
	
	unsigned int cycle = 0;
	ql::gate* gate = new ql::hadamard(0);
	gate->cycle = cycle;
	gates.push_back(gate);

	gate = new ql::hadamard(1);
	gate->cycle = cycle;
	gates.push_back(gate);

	gate = new ql::hadamard(2);
	gate->cycle = cycle;
	gates.push_back(gate);

	gate = new ql::hadamard(3);
	gate->cycle = cycle;
	gates.push_back(gate);


	cycle = 1;
	gate = new ql::cphase(1, 2);
	gate->cycle = cycle;
	gates.push_back(gate);


	cycle = 3;
	gate = new ql::t(1);
	gate->cycle = cycle;
	gates.push_back(gate);

	gate = new ql::t(2);
	gate->cycle = cycle;
	gates.push_back(gate);


	cycle = 4;
	gate = new ql::cphase(0, 1);
	gate->cycle = cycle;
	gates.push_back(gate);


	cycle = 6;
	gate = new ql::t(0);
	gate->cycle = cycle;
	gates.push_back(gate);

	gate = new ql::pauli_y(1);
	gate->cycle = cycle;
	gates.push_back(gate);

	gate = new ql::cphase(2, 3);
	gate->cycle = cycle;
	gates.push_back(gate);


	cycle = 8;
	gate = new ql::pauli_x(2);
	gate->cycle = cycle;
	gates.push_back(gate);

	gate = new ql::t(3);
	gate->cycle = cycle;
	gates.push_back(gate);


	cycle = 9;
	gate = new ql::cphase(1, 2);
	gate->cycle = cycle;
	gates.push_back(gate);


	cycle = 11;
	gate = new ql::pauli_x(1);
	gate->cycle = cycle;
	gates.push_back(gate);

	gate = new ql::t(2);
	gate->cycle = cycle;
	gates.push_back(gate);


	cycle = 12;
	gate = new ql::cphase(0, 1);
	gate->cycle = cycle;
	gates.push_back(gate);

	return gates;
}

std::vector<ql::gate*> getDummyGates()
{
	std::vector<ql::gate*> gates;

	ql::gate* gate1 = new ql::hadamard(0);
	gate1->cycle = 1;
	gates.push_back(gate1);

	ql::gate* gate2 = new ql::pauli_z(1);
	gate2->cycle = 2;
	gates.push_back(gate2);

	ql::gate* gate3 = new ql::pauli_x(4);
	gate3->cycle = 3;
	gates.push_back(gate3);

	ql::gate* gate4 = new ql::hadamard(1);
	gate4->cycle = 4;
	gates.push_back(gate4);

	ql::gate* gate5 = new ql::swap(1, 3);
	gate5->cycle = 5;
	gates.push_back(gate5);

	ql::gate* gate6 = new ql::measure(0, 0);
	gate6->cycle = 6;
	gates.push_back(gate6);

	return gates;
}*/

} // ql
