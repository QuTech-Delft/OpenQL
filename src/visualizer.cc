/**
 * @file   visualizer.cc
 * @date   08/2020
 * @author Tim van der Meer
 * @brief  definition of the visualizer
 */
 
#include "visualizer.h"
#include "visualizer_internal.h"`
#include "json.h"
#include "instruction_map.h"

#include <iostream>

using json = nlohmann::json;

namespace ql
{
// --- DONE ---
// visualization of custom gates
// option to enable or disable classical bit lines
// different types of cycle/duration(ns) labels
// gate duration outlines in gate color
// measurement without explicitly specified classical operand assumes default classical operand (same number as qubit number)
// read cycle duration from hardware config file, instead of having hardcoded value
// handle case where user does not or incorrectly specifies visualization nodes for custom gate

// -- IN PROGRESS ---
// implement a generic grid structure object to contain the visual structure of the circuit, to ease positioning of components in all the drawing functions
// 'cutting' circuits where nothing/not much is happening both in terms of idle cycles and idle qubits

// --- FUTURE WORK ---
// TODO: display wait/barrier gate (need wait gate fix first)
// TODO: fix overlapping connections for multiqubit gates/measurements
// TODO: representing the gates as waveforms (see andreas paper for examples)
// TODO: add classical bit number to measurement connection when classical lines are grouped
// TODO: implement measurement symbol (to replace the M on measurement gates)
// TODO: generate default gate visuals from the configuration file
// TODO: change IOUT to DOUT (IOUT is used to avoid debug information from other source files while developing the visualizer!)
// TODO: allow the user to set the layout object from Python
// TODO: add option to save the image and/or open the window

#ifndef WITH_VISUALIZER

//void visualize(const ql::quantum_program* program, const Layout layout)
void visualize(const ql::quantum_program* program, const std::string& configPath)
{
	WOUT("Visualizer is disabled. If this was not intended, the X11 library might be missing and the visualizer has disabled itself.");
}

#else

using namespace cimg_library;

unsigned int cycleDuration = 40;

//void visualize(const ql::quantum_program* program, const Layout layout)
void visualize(const ql::quantum_program* program, const std::string& configPath)
{
    IOUT("Starting visualization...");

	IOUT("Parsing visualizer configuration file.");
	const Layout layout = parseConfiguration(configPath);
	
    IOUT("Validating layout...");
	validateLayout(layout);

    // Get the gate list from the program.
    IOUT("Getting gate list...");
    std::vector<ql::gate*> gates;
    std::vector<ql::quantum_kernel> kernels = program->kernels;
    for (ql::quantum_kernel kernel : kernels)
    {
        circuit c = kernel.get_circuit();
        gates.insert( gates.end(), c.begin(), c.end() );
    }
    
	// Load cycle time and calculate amount of cycles.
	cycleDuration = program->platform.cycle_time;
	IOUT("Cycle duration is: " + std::to_string(cycleDuration) + " ns.");
    IOUT("Calculating amount of cycles...");
    unsigned int amountOfCycles = calculateAmountOfCycles(gates);

	// Compress the circuit in terms of cycles and gate duration if the option has been set.
	if (layout.cycles.compressCycles)
	{
        IOUT("Compressing circuit...");
		std::vector<bool> filledCycles(amountOfCycles);
		for (unsigned int i = 0; i < gates.size(); i++)
		{
			filledCycles.at(gates.at(i)->cycle) = true;
		}

        //replace with DOUT
		//std::cout << "amount of cycles before compression: " << amountOfCycles << std::endl;
		unsigned int amountOfCompressions = 0;
		for (unsigned int i = 0; i < filledCycles.size(); i++)
		{
			//std::cout << i;
			if (filledCycles.at(i) == false)
			{
				//std::cout << " not filled" << std::endl;
				//std::cout << "\tcompressing... min cycle to compress: " << i - amountOfCompressions << std::endl;
				for (unsigned int j = 0; j < gates.size(); j++)
				{
					const unsigned int gateCycle = (unsigned int)gates.at(j)->cycle;
					//std::cout << "\tgate cycle: " << gateCycle;
					if (gateCycle >= i - amountOfCompressions)
					{
						gates.at(j)->cycle = gates.at(j)->cycle - 1;
						//std::cout << " -> compressing cycle" << std::endl;
					}
					else
					{
						//std::cout << " -> no compression" << std::endl;
					}
				}
				amountOfCycles--;
				amountOfCompressions++;
			}
			//else
				//std::cout << " filled" << std::endl;
		}
		//std::cout << "amount of cycles after compression: " << amountOfCycles << std::endl;
	}

	// Calculate amount of qubits and classical bits.
    IOUT("Calculating amount of qubits and classical bits...");
	fixMeasurementOperands(gates);
	const unsigned int amountOfQubits = calculateAmountOfBits(gates, &gate::operands);
	const unsigned int amountOfCbits = calculateAmountOfBits(gates, &gate::creg_operands);
	CircuitData circuitData = { amountOfQubits, amountOfCbits, amountOfCycles };
    
	// Calculate image width and height based on the amount of cycles and amount of operands. The height depends on whether classical bit lines are grouped or not.
    IOUT("Calculating image width and height...");
	const unsigned int width = (layout.bitLines.drawLabels ? layout.bitLines.labelColumnWidth : 0) + amountOfCycles * layout.grid.cellSize + 2 * layout.grid.borderSize;
	const unsigned int amountOfRows = amountOfQubits + (layout.bitLines.groupClassicalLines ? (amountOfCbits > 0 ? 1 : 0) : amountOfCbits);
	const unsigned int height = (layout.cycles.showCycleNumbers ? layout.cycles.rowHeight : 0) + amountOfRows * layout.grid.cellSize + 2 * layout.grid.borderSize;
    
	// Initialize image.
    IOUT("Initializing image...");
	const unsigned int numberOfChannels = 3;
	CImg<unsigned char> image(width, height, 1, numberOfChannels);
	image.fill(255);

	// Draw the cycle numbers if the option has been set.
	if (layout.cycles.showCycleNumbers)
	{
        IOUT("Drawing cycle numbers...");
		drawCycleNumbers(image, layout, circuitData);
	}

	// Draw the quantum and classical bit lines.
    IOUT("Drawing qubit lines...");
	for (unsigned int i = 0; i < amountOfQubits; i++)
	{
		drawBitLine(image, layout, QUANTUM, i, circuitData);
	}
	
	// Draw the classical lines if enabled.
	if (layout.bitLines.showClassicalLines)
	{
		// Draw the grouped classical bit lines if the option is set.
		if (amountOfCbits > 0 && layout.bitLines.groupClassicalLines)
		{
			IOUT("Drawing grouped classical bit lines...");
			drawGroupedClassicalBitLine(image, layout, circuitData);
		}
		// Otherwise draw each classical bit line seperate.
		else
		{
			IOUT("Drawing ungrouped classical bit lines...");
			for (unsigned int i = amountOfQubits; i < amountOfQubits + amountOfCbits; i++)
			{
				drawBitLine(image, layout, CLASSICAL, i, circuitData);
			}
		}
	}

	// Draw the gates.
    IOUT("Drawing gates...");
	for (gate* gate : gates)
	{
        //const GateVisual gateVisual = layout.gateVisuals.at(gate->type());
        IOUT("Drawing gate: [name: " + gate->name + "]");
		drawGate(image, layout, circuitData, gate);
	}
	
	// Display the image.
    IOUT("Displaying image...");
	image.display("Quantum Circuit");

    IOUT("Visualization complete...");
}

Layout parseConfiguration(const std::string& configPath)
{
	json config;
	try
	{
		config = load_json(configPath);
	}
	catch (json::exception &e)
	{
		FATAL("Failed to load the visualization config file: malformed json file: \n\t" << std::string(e.what()));
	}

	Layout layout;

	// Fill the layout object with the values from the config file, or if those values are missing, with the default hardcoded values.
	//TODO: replace these hardcoded assignments by automatic json to layout object mapping (is possible with nlohmann json!)
	if (config.count("cycles") == 1)
	{
		layout.cycles.showCycleNumbers = config["cycles"].value("showCycleNumbers", layout.cycles.showCycleNumbers);
		layout.cycles.showCyclesInNanoSeconds = config["cycles"].value("showCyclesInNanoSeconds", layout.cycles.showCyclesInNanoSeconds);
		layout.cycles.rowHeight = config["cycles"].value("rowHeight", layout.cycles.rowHeight);
		layout.cycles.fontHeight = config["cycles"].value("fontHeight", layout.cycles.fontHeight);
		layout.cycles.fontColor = config["cycles"].value("fontColor", layout.cycles.fontColor);

		layout.cycles.compressCycles = config["cycles"].value("compressCycles", layout.cycles.compressCycles);
		layout.cycles.showGateDurationOutline = config["cycles"].value("showGateDurationOutline", layout.cycles.showGateDurationOutline);
		layout.cycles.gateDurationGap = config["cycles"].value("gateDurationGap", layout.cycles.gateDurationGap);
		layout.cycles.gateDurationAlpha = config["cycles"].value("gateDurationAlpha", layout.cycles.gateDurationAlpha);
		layout.cycles.gateDurationOutLineAlpha = config["cycles"].value("gateDurationOutLineAlpha", layout.cycles.gateDurationOutLineAlpha);
		layout.cycles.gateDurationOutlineColor = config["cycles"].value("gateDurationOutlineColor", layout.cycles.gateDurationOutlineColor);
	}

	if (config.count("bitLines") == 1)
	{
		layout.bitLines.drawLabels = config["bitLines"].value("drawLabels", layout.bitLines.drawLabels);
		layout.bitLines.labelColumnWidth = config["bitLines"].value("labelColumnWidth", layout.bitLines.labelColumnWidth);
		layout.bitLines.fontHeight = config["bitLines"].value("fontHeight", layout.bitLines.fontHeight);
		layout.bitLines.qBitLabelColor = config["bitLines"].value("qBitLabelColor", layout.bitLines.qBitLabelColor);
		layout.bitLines.cBitLabelColor = config["bitLines"].value("cBitLabelColor", layout.bitLines.cBitLabelColor);

		layout.bitLines.showClassicalLines = config["bitLines"].value("showClassicalLines", layout.bitLines.showClassicalLines);
		layout.bitLines.groupClassicalLines = config["bitLines"].value("groupClassicalLines", layout.bitLines.groupClassicalLines);
		layout.bitLines.groupedClassicalLineGap = config["bitLines"].value("groupedClassicalLineGap", layout.bitLines.groupedClassicalLineGap);
		layout.bitLines.qBitLineColor = config["bitLines"].value("qBitLineColor", layout.bitLines.qBitLineColor);
		layout.bitLines.cBitLineColor = config["bitLines"].value("cBitLineColor", layout.bitLines.cBitLineColor);
	}

	if (config.count("grid") == 1)
	{
		layout.grid.cellSize = config["grid"].value("cellSize", layout.grid.cellSize);
		layout.grid.borderSize = config["grid"].value("borderSize", layout.grid.borderSize);
	}
	
	if (config.count("measurements") == 1)
	{
		layout.measurements.drawConnection = config["measurements"].value("drawConnection", layout.measurements.drawConnection);
		layout.measurements.lineSpacing = config["measurements"].value("lineSpacing", layout.measurements.lineSpacing);
		layout.measurements.arrowSize = config["measurements"].value("arrowSize", layout.measurements.arrowSize);
	}

	return layout;
}

void validateLayout(const Layout layout)
{
	//TODO
}

unsigned int calculateAmountOfBits(const std::vector<ql::gate*> gates, const std::vector<size_t> ql::gate::* operandType)
{
	//TODO: handle circuits not starting at the c- or q-bit with index 0

	unsigned int minAmount = -1; // unsigned, so -1 is equal to the maximum value of the type
	unsigned int maxAmount = 0;

	for (const gate* gate : gates)
	{
		std::vector<size_t>::const_iterator begin = (gate->*operandType).begin();
		const std::vector<size_t>::const_iterator end = (gate->*operandType).end();
		
		for (; begin != end; ++begin)
		{
			const size_t number = *begin;
			if (number < minAmount)
				minAmount = (unsigned int) number;
			if (number > maxAmount)
				maxAmount = (unsigned int) number;
		}
	}

	// If both minAmount and maxAmount are at their original values, the list of operands for all the gates was empty.
	// This means there are no operands of the given type for these gates and we return 0.
	if (minAmount == (unsigned int) -1 && maxAmount == 0)
		return 0;
	else
		return 1 + maxAmount - minAmount; // +1 because: max - min = #qubits - 1
}

unsigned int calculateAmountOfCycles(const std::vector<ql::gate*> gates)
{
    unsigned int amountOfCycles = 0;
	for (const gate* gate : gates)
	{
		const unsigned int gateCycle = (unsigned int)gate->cycle;
		if (gateCycle > amountOfCycles)
			amountOfCycles = gateCycle;
	}
	amountOfCycles++; // because the cycles start at zero, we add one to get the true amount of cycles
	const gate* lastGate = gates.at(gates.size() - 1);
	const unsigned int lastGateDuration = (unsigned int)lastGate->duration;
	const unsigned int lastGateDurationInCycles = lastGateDuration / cycleDuration;
	if (lastGateDurationInCycles > 1)
	{
		amountOfCycles += lastGateDurationInCycles - 1;
	}

    return amountOfCycles;
}

unsigned int calculateAmountOfGateOperands(const ql::gate* gate)
{
	return (unsigned int)gate->operands.size() + (unsigned int)gate->creg_operands.size();
}

void fixMeasurementOperands(const std::vector<ql::gate*> gates)
{
	for (gate* gate : gates)
	{
		// Check for a measurement gate without explicitly specified classical operand.
		if (isMeasurement(gate))
		{
			if (calculateAmountOfGateOperands(gate) == 1)
			{
				// Set classical measurement operand to the bit corresponding to the measuremens qubit number.
				IOUT("Found measurement gate with no classical operand. Assuming default classical operand.");
				const unsigned int cbit = gate->operands[0];
				gate->creg_operands.push_back(cbit);
			}
		}
	}
}

bool isMeasurement(const ql::gate* gate)
{
	//TODO: this method of checking for measurement gates is not very robust!
	return (gate->name.find("measure") != std::string::npos);
}

void drawCycleNumbers(cimg_library::CImg<unsigned char>& image, const Layout layout, const CircuitData circuitData)
{
	for (unsigned int i = 0; i < circuitData.amountOfCycles; i++)
	{
		std::string cycleLabel;
		if (layout.cycles.showCyclesInNanoSeconds)
		{
			cycleLabel = std::to_string(i * cycleDuration);
		}
		else
		{
			cycleLabel = std::to_string(i);
		}
		
		const char* text = cycleLabel.c_str();
		CImg<unsigned char> imageTextDimensions;
		const unsigned char color = 1;
		imageTextDimensions.draw_text(0, 0, text, &color, 0, 1, layout.bitLines.fontHeight);
		const unsigned int textWidth = imageTextDimensions.width();
		const unsigned int textHeight = imageTextDimensions.height();

		const unsigned int labelColumnWidth = layout.bitLines.drawLabels ? layout.bitLines.labelColumnWidth : 0;
		const unsigned int xGap = (layout.grid.cellSize - textWidth) / 2;
		const unsigned int yGap = (layout.grid.cellSize - textHeight) / 2;
		const unsigned int xCycle = layout.grid.borderSize + labelColumnWidth + i * layout.grid.cellSize + xGap;
		const unsigned int yCycle = layout.grid.borderSize + yGap;

		image.draw_text(xCycle, yCycle, text, layout.cycles.fontColor.data(), 0, 1, layout.cycles.fontHeight);
	}
}

void drawBitLine(cimg_library::CImg<unsigned char> &image, const Layout layout, const BitType bitType, const unsigned int row, const CircuitData circuitData)
{
	const unsigned int cycleNumbersRowHeight = layout.cycles.showCycleNumbers ? layout.cycles.rowHeight : 0;
	const unsigned int labelColumnWidth = layout.bitLines.drawLabels ? layout.bitLines.labelColumnWidth : 0;
	const unsigned int x0 = labelColumnWidth + layout.grid.borderSize;
	const unsigned int x1 = labelColumnWidth + layout.grid.borderSize + circuitData.amountOfCycles * layout.grid.cellSize;
	const unsigned int y = cycleNumbersRowHeight + layout.grid.borderSize + row * layout.grid.cellSize + layout.grid.cellSize / 2;

	std::array<unsigned char, 3> bitLineColor;
	std::array<unsigned char, 3> bitLabelColor;
	switch (bitType)
	{
		case CLASSICAL:
			bitLineColor = layout.bitLines.cBitLineColor;
			bitLabelColor = layout.bitLines.cBitLabelColor;
			break;
		case QUANTUM:
			bitLineColor = layout.bitLines.qBitLineColor;
			bitLabelColor = layout.bitLines.qBitLabelColor;
			break;
	}

	image.draw_line(x0, y, x1, y, bitLineColor.data());

	// Draw the bit line label if enabled.
	if (layout.bitLines.drawLabels)
	{
		const unsigned int bitIndex = (bitType == CLASSICAL) ? (row - circuitData.amountOfQubits) : row;
		const std::string bitTypeText = (bitType == CLASSICAL) ? "c" : "q";
		std::string label = bitTypeText + std::to_string(bitIndex);
		const char* text = label.c_str();
		CImg<unsigned char> imageTextDimensions;
		const unsigned char color = 1;
		imageTextDimensions.draw_text(0, 0, text, &color, 0, 1, layout.bitLines.fontHeight);
		const unsigned int textWidth = imageTextDimensions.width();
		const unsigned int textHeight = imageTextDimensions.height();
		const unsigned int xGap = (layout.grid.cellSize - textWidth) / 2;
		const unsigned int yGap = (layout.grid.cellSize - textHeight) / 2;
		const unsigned int xLabel = layout.grid.borderSize + xGap;
		const unsigned int yLabel = layout.grid.borderSize + cycleNumbersRowHeight + row * layout.grid.cellSize + yGap;
		image.draw_text(xLabel, yLabel, text, bitLabelColor.data(), 0, 1, layout.bitLines.fontHeight);
	}
}

void drawGroupedClassicalBitLine(cimg_library::CImg<unsigned char>& image, const Layout layout, const CircuitData circuitData)
{
	const unsigned int cycleNumbersRowHeight = layout.cycles.showCycleNumbers ? layout.cycles.rowHeight : 0;
	const unsigned int labelColumnWidth = layout.bitLines.drawLabels ? layout.bitLines.labelColumnWidth : 0;
	const unsigned int x0 = labelColumnWidth + layout.grid.borderSize;
	const unsigned int x1 = labelColumnWidth + layout.grid.borderSize + circuitData.amountOfCycles * layout.grid.cellSize;
	const unsigned int y = cycleNumbersRowHeight + layout.grid.borderSize + circuitData.amountOfQubits * layout.grid.cellSize + layout.grid.cellSize / 2;

	image.draw_line(x0, y - layout.bitLines.groupedClassicalLineGap, x1, y - layout.bitLines.groupedClassicalLineGap, layout.bitLines.cBitLineColor.data());
	image.draw_line(x0, y + layout.bitLines.groupedClassicalLineGap, x1, y + layout.bitLines.groupedClassicalLineGap, layout.bitLines.cBitLineColor.data());
	//TODO: store the dashed line parameters in the layout object
	image.draw_line(x0 + 8, y + layout.bitLines.groupedClassicalLineGap + 2, x0 + 12, y - layout.bitLines.groupedClassicalLineGap - 3, layout.bitLines.cBitLineColor.data());
	//TODO: draw a number indicating the amount of classical lines that are grouped
	const std::string label = std::to_string(circuitData.amountOfClassicalBits);
	const char* text = label.c_str();
	CImg<unsigned char> imageTextDimensions;
	const unsigned char color = 1;
	imageTextDimensions.draw_text(0, 0, text, &color, 0, 1, layout.bitLines.fontHeight);
	//const unsigned int textWidth = imageTextDimensions.width();
	//const unsigned int textHeight = imageTextDimensions.height();
	const unsigned int xLabel = x0 + 8;
	const unsigned int yLabel = y - layout.bitLines.groupedClassicalLineGap - 3 - 13;
	image.draw_text(xLabel, yLabel, text, layout.bitLines.cBitLabelColor.data(), 0, 1, layout.bitLines.fontHeight);

	// Draw the bit line label if enabled.
	if (layout.bitLines.drawLabels)
	{
		const std::string label = "C";
		const char* text = label.c_str();
		CImg<unsigned char> imageTextDimensions;
		const unsigned char color = 1;
		imageTextDimensions.draw_text(0, 0, text, &color, 0, 1, layout.bitLines.fontHeight);
		const unsigned int textWidth = imageTextDimensions.width();
		const unsigned int textHeight = imageTextDimensions.height();
		const unsigned int xGap = (layout.grid.cellSize - textWidth) / 2;
		const unsigned int yGap = (layout.grid.cellSize - textHeight) / 2;
		const unsigned int xLabel = layout.grid.borderSize + xGap;
		const unsigned int yLabel = layout.grid.borderSize + cycleNumbersRowHeight + circuitData.amountOfQubits * layout.grid.cellSize + yGap;
		image.draw_text(xLabel, yLabel, text, layout.bitLines.cBitLabelColor.data(), 0, 1, layout.bitLines.fontHeight);
	}
}

void drawGate(cimg_library::CImg<unsigned char> &image, const Layout layout, const CircuitData circuitData, gate* const gate)
{
	const unsigned int amountOfOperands = calculateAmountOfGateOperands(gate);
	//const unsigned int amountOfOperands = (unsigned int)gate->operands.size() + (unsigned int)gate->creg_operands.size();
	const unsigned int cycleNumbersRowHeight = layout.cycles.showCycleNumbers ? layout.cycles.rowHeight : 0;
	const unsigned int labelColumnWidth = layout.bitLines.drawLabels ? layout.bitLines.labelColumnWidth : 0;
	
	IOUT("drawing gate with name: '" << gate->name << "'");
	
	GateVisual gateVisual;
	if (gate->type() == __custom_gate__)
	{
		IOUT("Custom gate found. Using user specified visualization.");
		gateVisual = gate->gateVisual;
	}
	else
	{
		IOUT("Default gate found. Using default visualization!");
		gateVisual = layout.defaultGateVisuals.at(gate->type());
	}

	// Check for correct amount of nodes.
	if (amountOfOperands != gateVisual.nodes.size())
	{
		WOUT("Amount of gate operands and visualization nodes are not equal. Skipping gate...");
		return;
	}

	if (amountOfOperands > 1)
	{
        IOUT("setting up multi-operand gate...");
		// Draw the lines between each node. If this is done before drawing the nodes, there is no need to calculate line segments, we can just draw one
		// big line between the nodes and the nodes will be drawn on top of those.
		// Note: does not work with transparent nodes! If those are ever implemented, the connection line drawing will need to be changed!

		unsigned int minRow = -1; // maximum value of an unsigned int is equal to -1
		unsigned int maxRow = 0;
		for (unsigned int i = 0; i < gate->operands.size(); i++)
		{
			const unsigned int operand = gate->operands.at(i);
			if (operand < minRow)
				minRow = operand;
			if (operand > maxRow)
				maxRow = operand;
		}
		if (layout.bitLines.groupClassicalLines)
		{
			if (gate->creg_operands.size() > 0)
			{
				maxRow = circuitData.amountOfQubits;
			}
		}
		else
		{
			for (unsigned int i = 0; i < gate->creg_operands.size(); i++)
			{
				const unsigned int operand = gate->creg_operands.at(i) + circuitData.amountOfQubits;
				if (operand < minRow)
					minRow = operand;
				if (operand > maxRow)
					maxRow = operand;
			}
		}
		const unsigned int column = (unsigned int)gate->cycle;

		Position4 connectionPosition =
		{
			layout.grid.borderSize + labelColumnWidth + column * layout.grid.cellSize + layout.grid.cellSize / 2,
			layout.grid.borderSize + cycleNumbersRowHeight + minRow * layout.grid.cellSize + layout.grid.cellSize / 2,
			layout.grid.borderSize + labelColumnWidth + column * layout.grid.cellSize + layout.grid.cellSize / 2,
			layout.grid.borderSize + cycleNumbersRowHeight + maxRow * layout.grid.cellSize + layout.grid.cellSize / 2
		};

		//TODO: probably have connection line type as part of a gate's visual definition
		//if (gate->type() == __measure_gate__)
		if (isMeasurement(gate))
		{
			if (layout.measurements.drawConnection && layout.bitLines.showClassicalLines)
			{
				const unsigned groupedClassicalLineOffset = layout.bitLines.groupClassicalLines ? layout.bitLines.groupedClassicalLineGap : 0;

				image.draw_line(connectionPosition.x0 - layout.measurements.lineSpacing, connectionPosition.y0,
					connectionPosition.x1 - layout.measurements.lineSpacing, connectionPosition.y1 - layout.measurements.arrowSize - groupedClassicalLineOffset,
					gateVisual.connectionColor.data());

				image.draw_line(connectionPosition.x0 + layout.measurements.lineSpacing, connectionPosition.y0,
					connectionPosition.x1 + layout.measurements.lineSpacing, connectionPosition.y1 - layout.measurements.arrowSize - groupedClassicalLineOffset,
					gateVisual.connectionColor.data());

				const unsigned int x0 = connectionPosition.x1 - layout.measurements.arrowSize / 2;
				const unsigned int y0 = connectionPosition.y1 - layout.measurements.arrowSize - groupedClassicalLineOffset;
				const unsigned int x1 = connectionPosition.x1 + layout.measurements.arrowSize / 2;
				const unsigned int y1 = connectionPosition.y1 - layout.measurements.arrowSize - groupedClassicalLineOffset;
				const unsigned int x2 = connectionPosition.x1;
				const unsigned int y2 = connectionPosition.y1 - groupedClassicalLineOffset;
				image.draw_triangle(x0, y0, x1, y1, x2, y2, gateVisual.connectionColor.data(), 1);
			}
		}
		else
		{
			image.draw_line(connectionPosition.x0, connectionPosition.y0, connectionPosition.x1, connectionPosition.y1, gateVisual.connectionColor.data());
		}
        IOUT("finished setting up multi-operand gate");
	}

	// Draw the gate duration outline if the option has been set.
	if (!layout.cycles.compressCycles && layout.cycles.showGateDurationOutline)
	{
        IOUT("drawing gate duration outline...");
		const unsigned int gateDurationInCycles = ((unsigned int)gate->duration) / cycleDuration;
		// Only draw the gate outline if the gate takes more than one cycle.
		if (gateDurationInCycles > 1)
		{
			for (unsigned int i = 0; i < amountOfOperands; i++)
			{
				const unsigned int columnStart = (unsigned int)gate->cycle;
				const unsigned int columnEnd = columnStart + gateDurationInCycles - 1;
				const unsigned int row = gate->operands[i];

				const unsigned int x0 = layout.grid.borderSize + labelColumnWidth + columnStart * layout.grid.cellSize + layout.cycles.gateDurationGap;
				const unsigned int y0 = layout.grid.borderSize + cycleNumbersRowHeight + row * layout.grid.cellSize + layout.cycles.gateDurationGap;
				const unsigned int x1 = layout.grid.borderSize + labelColumnWidth + (columnEnd + 1) * layout.grid.cellSize - +layout.cycles.gateDurationGap;
				const unsigned int y1 = layout.grid.borderSize + cycleNumbersRowHeight + (row + 1) * layout.grid.cellSize - +layout.cycles.gateDurationGap;
				
				// Draw the outline in the colors of the node.
				const Node node = gateVisual.nodes.at(i);
				image.draw_rectangle(x0, y0, x1, y1, node.backgroundColor.data(), layout.cycles.gateDurationAlpha);
				image.draw_rectangle(x0, y0, x1, y1, node.outlineColor.data(), layout.cycles.gateDurationOutLineAlpha, 0xF0F0F0F0);
				
				//image.draw_rectangle(x0, y0, x1, y1, layout.cycles.gateDurationOutlineColor.data(), layout.cycles.gateDurationAlpha);
				//image.draw_rectangle(x0, y0, x1, y1, layout.cycles.gateDurationOutlineColor.data(), layout.cycles.gateDurationOutLineAlpha, 0xF0F0F0F0);
			}
		}
	}

	// Draw the nodes.
    IOUT("drawing gate nodes...");
	for (unsigned int i = 0; i < amountOfOperands; i++)
	{
        IOUT("drawing gate node with index: " + std::to_string(i) + "...");
        //TODO: change the try-catch later on! the gate config will be read from somewhere else than the default layout
        try
        {
		    const Node node = gateVisual.nodes.at(i);
            const BitType operandType = (i >= gate->operands.size()) ? CLASSICAL : QUANTUM;
            const unsigned int index = (operandType == QUANTUM) ? i : (i - (unsigned int)gate->operands.size());
            const NodePositionData positionData =
            {
	            (layout.grid.cellSize - node.radius * 2) / 2,
	            labelColumnWidth,
	            cycleNumbersRowHeight,
	            (unsigned int)gate->cycle,
	            operandType == CLASSICAL ? (unsigned int)gate->creg_operands.at(index) + circuitData.amountOfQubits : (unsigned int)gate->operands.at(index)
            };

            switch (node.type)
            {
	            case NONE:		DOUT("node.type = NONE"); break; // Do nothing.
	            case GATE:		DOUT("node.type = GATE"); drawGateNode(image, layout, circuitData, node, positionData); break;
	            case CONTROL:	DOUT("node.type = CONTROL"); drawControlNode(image, layout, circuitData, node, positionData); break;
	            case NOT:		DOUT("node.type = NOT"); drawNotNode(image, layout, circuitData, node, positionData); break;
	            case CROSS:		DOUT("node.type = CROSS"); drawCrossNode(image, layout, circuitData, node, positionData); break;
                default:        EOUT("Unknown gate display node type!"); break;
            }
        }
        catch (const std::out_of_range& e)
        {
            return;
        }
		
        IOUT("finished drawing gate node with index: " + std::to_string(i) + "...");
	}

	// Draw the measurement symbol.
		//const unsigned int xGap = 2;
		//const unsigned int yGap = 13;

		//const unsigned int x0 = position.x0 + xGap;
		//const unsigned int y0 = position.y0 + yGap;
		//const unsigned int x1 = position.x1 + xGap;
		//const unsigned int y1 = y0;

		//const unsigned int xa = x0 + (x1 - x0) / 3;
		//const unsigned int ya = y0 + yGap / 2;
		//const unsigned int xb = x1 - (x1 - x0) / 3;
		//const unsigned int yb = ya;

		//const unsigned int u0 = xa - x0;
		//const unsigned int v0 = ya - y0;
		//const unsigned int u1 = x1 - xb;
		//const unsigned int v1 = y1 - yb;

		//image.draw_spline(x0, y0, u0, v0, x1, y1, u1, v1, layout.operation.gateNameColor.data());
}

void drawGateNode(cimg_library::CImg<unsigned char>& image, const Layout layout, const CircuitData circuitData, const Node node, const NodePositionData positionData)
{
	const Position4 position =
	{
		layout.grid.borderSize + positionData.labelColumnWidth + positionData.column * layout.grid.cellSize + positionData.gap,
		layout.grid.borderSize + positionData.cycleNumbersRowHeight + positionData.row * layout.grid.cellSize + positionData.gap,
		layout.grid.borderSize + positionData.labelColumnWidth + (positionData.column + 1) * layout.grid.cellSize - positionData.gap,
		layout.grid.borderSize + positionData.cycleNumbersRowHeight + (positionData.row + 1) * layout.grid.cellSize - positionData.gap
	};

	// Draw the gate background.
	image.draw_rectangle(position.x0, position.y0, position.x1, position.y1, node.backgroundColor.data());
	image.draw_rectangle(position.x0, position.y0, position.x1, position.y1, node.outlineColor.data(), 1, 0xFFFFFFFF);

	// Draw the gate symbol. The width and height of the symbol are calculated first to correctly position the symbol within the gate.
	const char* text = node.displayName.c_str();
	CImg<unsigned char> imageTextDimensions;
	const unsigned char color = 1;
	imageTextDimensions.draw_text(0, 0, text, &color, 0, 1, node.fontHeight);
	const unsigned int textWidth = imageTextDimensions.width();
	const unsigned int textHeight = imageTextDimensions.height();
	image.draw_text(position.x0 + (node.radius * 2 - textWidth) / 2, position.y0 + (node.radius * 2 - textHeight) / 2, text, node.fontColor.data(), 0, 1, node.fontHeight);
}

void drawControlNode(cimg_library::CImg<unsigned char>& image, const Layout layout, const CircuitData circuitData, const Node node, const NodePositionData positionData)
{
	const Position2 connectionPosition =
	{
		layout.grid.borderSize + positionData.labelColumnWidth + positionData.column * layout.grid.cellSize + layout.grid.cellSize / 2,
		layout.grid.borderSize + positionData.cycleNumbersRowHeight + positionData.row * layout.grid.cellSize + layout.grid.cellSize / 2
	};

	image.draw_circle(connectionPosition.x, connectionPosition.y, node.radius, node.backgroundColor.data());
}

void drawNotNode(cimg_library::CImg<unsigned char>& image, const Layout layout, const CircuitData circuitData, const Node node, const NodePositionData positionData)
{
	// TODO: allow for filled not node instead of only an outline not node

	const Position2 notPosition =
	{
		layout.grid.borderSize + positionData.labelColumnWidth + positionData.column * layout.grid.cellSize + layout.grid.cellSize / 2,
		layout.grid.borderSize + positionData.cycleNumbersRowHeight + positionData.row * layout.grid.cellSize + layout.grid.cellSize / 2
	};

	// Draw the outlined circle.
	image.draw_circle(notPosition.x, notPosition.y, node.radius, node.backgroundColor.data(), 1, 0xFFFFFFFF);

	// Draw two lines to represent the plus sign.
	const unsigned int xHor0 = notPosition.x - node.radius;
	const unsigned int xHor1 = notPosition.x + node.radius;
	const unsigned int yHor = notPosition.y;

	const unsigned int xVer = notPosition.x;
	const unsigned int yVer0 = notPosition.y - node.radius;
	const unsigned int yVer1 = notPosition.y + node.radius;

	image.draw_line(xHor0, yHor, xHor1, yHor, node.backgroundColor.data());
	image.draw_line(xVer, yVer0, xVer, yVer1, node.backgroundColor.data());
}

void drawCrossNode(cimg_library::CImg<unsigned char>& image, const Layout layout, const CircuitData circuitData, const Node node, const NodePositionData positionData)
{
	const Position2 crossPosition =
	{
		layout.grid.borderSize + positionData.labelColumnWidth + positionData.column * layout.grid.cellSize + layout.grid.cellSize / 2,
		layout.grid.borderSize + positionData.cycleNumbersRowHeight + positionData.row * layout.grid.cellSize + layout.grid.cellSize / 2
	};

	// Draw two diagonal lines to represent the cross.
	const unsigned int x0 = crossPosition.x - node.radius;
	const unsigned int y0 = crossPosition.y - node.radius;
	const unsigned int x1 = crossPosition.x + node.radius;
	const unsigned int y1 = crossPosition.y + node.radius;

	image.draw_line(x0, y0, x1, y1, node.backgroundColor.data());
	image.draw_line(x0, y1, x1, y0, node.backgroundColor.data());
}

#endif //WITH_VISUALIZER

} // ql
