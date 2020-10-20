/**
 * @file   visualizer.cc
 * @date   08/2020
 * @author Tim van der Meer
 * @brief  definition of the visualizer
 */
 
#include "visualizer.h"
#include "visualizer_internal.h"
#include "json.h"
#include "instruction_map.h"

#include <iostream>
#include <limits>

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
// allow the user to set the layout parameters from a configuration file
// implement a generic grid structure object to contain the visual structure of the circuit, to ease positioning of components in all the drawing functions
// change IOUT to DOUT (IOUT is used to avoid debug information from other source files while developing the visualizer!)
// visual_type attribute instead of full visual attribute in hw config file, links to seperate visualization config file where details of that visual type are detailed
// 'cutting' circuits where nothing/not much is happening both in terms of idle cycles and idle qubits
// add bit line zigzag indicating a cut cycle range
// add cutEmptyCycles and emptyCycleThreshold to the documentation
// make a copy of the gate vector, so any changes inside the visualizer to the program do not reflect back to any future compiler passes
// add option to display cycle edges

// -- IN PROGRESS ---
// add option to draw horizontal lines between qubits
// representing the gates as waveforms
// allow collapsing the three qubit lines into one with an option
// change size_t cycle to Cycle cycle in GateProperties (maybe)
// re-organize the attributes in the config file
// what happens when a cycle range is cut, but one or more gates still running within that range finish earlier than the longest running gate 
// 		comprising the entire range?

// --- FUTURE WORK ---
// TODO: remove drawBitLine and replace it with drawCycle drawing the lines in the cycle itself (maybe)
// TODO: when measurement connections are not shown, allow overlap of measurement gates
// TODO: GateProperties validation on construction
// TODO: the visualizer should probably be a class
// TODO: when gate is skipped due to whatever reason, maybe show a dummy gate outline indicating where the gate is?
// TODO: display wait/barrier gate (need wait gate fix first)
// TODO: add classical bit number to measurement connection when classical lines are grouped
// TODO: implement measurement symbol (to replace the M on measurement gates)
// TODO: generate default gate visuals from the configuration file
// TODO: add option to save the image and/or open the window

#ifndef WITH_VISUALIZER

void visualize(const ql::quantum_program* program, const std::string& configPath)
{
	WOUT("The visualizer is disabled. If this was not intended, the X11 library might be missing and the visualizer has disabled itself.");
}

#else

using json = nlohmann::json;

// ======================================================= //
// =                     CircuitData                     = //
// ======================================================= //

CircuitData::CircuitData(std::vector<GateProperties>& gates, const Layout layout, const int cycleDuration) :
	cycleDuration(cycleDuration),
	amountOfQubits(calculateAmountOfBits(gates, &GateProperties::operands)),
	amountOfClassicalBits(calculateAmountOfBits(gates, &GateProperties::creg_operands)),
	cycles(generateCycles(gates, cycleDuration))
{
	if (layout.cycles.compressCycles)				compressCycles();
	if (layout.cycles.partitionCyclesWithOverlap)	partitionCyclesWithOverlap();
	if (layout.cycles.cutEmptyCycles)				cutEmptyCycles(layout);
}

int CircuitData::calculateAmountOfBits(const std::vector<GateProperties> gates, const std::vector<size_t> GateProperties::* operandType) const
{
	DOUT("Calculating amount of bits...");

	//TODO: handle circuits not starting at a c- or qbit with index 0
	int minAmount = std::numeric_limits<int>::max();
	int maxAmount = 0;

	// Find the minimum and maximum index of the operands.
	for (const GateProperties& gate : gates)
	{
		std::vector<size_t>::const_iterator begin = (gate.*operandType).begin();
		const std::vector<size_t>::const_iterator end = (gate.*operandType).end();
		
		for (; begin != end; ++begin)
		{
			const size_t number = *begin;
			if (number < minAmount)
				minAmount = (int) number;
			if (number > maxAmount)
				maxAmount = (int) number;
		}
	}

	// If both minAmount and maxAmount are at their original values, the list of operands for all the gates was empty.
	// This means there are no operands of the given type for these gates and we return 0.
	if (minAmount == std::numeric_limits<int>::max() && maxAmount == 0)
		return 0;
	else
		return 1 + maxAmount - minAmount; // +1 because: max - min = #qubits - 1
}

int CircuitData::calculateAmountOfCycles(const std::vector<GateProperties> gates, const int cycleDuration) const
{
	DOUT("Calculating amount of cycles...");

	// Find the highest cycle in the gate vector.
    int amountOfCycles = 0;
	for (const GateProperties& gate : gates)
	{
		const int gateCycle = (int) gate.cycle;
		if (gateCycle > amountOfCycles)
			amountOfCycles = gateCycle;
	}

	// The last gate requires a different approach, because it might have a duration of multiple cycles.
	// None of those cycles will show up as cycle index on any other gate, so we need to calculate them seperately.
	const int lastGateDuration = (int) gates.at(gates.size() - 1).duration;
	const int lastGateDurationInCycles = lastGateDuration / cycleDuration;
	if (lastGateDurationInCycles > 1)
	{
		amountOfCycles += lastGateDurationInCycles - 1;
	}

    return amountOfCycles + 1; // because the cycles start at zero, we add one to get the true amount of cycles
}

std::vector<Cycle> CircuitData::generateCycles(std::vector<GateProperties>& gates, const int cycleDuration) const
{
	DOUT("Generating cycles...");

	// Generate the cycles.
	std::vector<Cycle> cycles;
	const int amountOfCycles = calculateAmountOfCycles(gates, cycleDuration);
	for (int i = 0; i < amountOfCycles; i++)
	{
		// Generate the first chunk of the gate partition for this cycle.
		// All gates in this cycle will be added to this chunk first, later on they will be divided based on connectivity (if enabled).
		std::vector<std::vector<std::reference_wrapper<GateProperties>>> partition;
		const std::vector<std::reference_wrapper<GateProperties>> firstChunk;
		partition.push_back(firstChunk);
		
		cycles.push_back({i, true, false, partition});
	}
	// Mark non-empty cycles and add gates to their corresponding cycles.
	for (GateProperties& gate : gates)
	{
		cycles[gate.cycle].empty = false;
		cycles[gate.cycle].gates[0].push_back(gate);
	}

	return cycles;
}

void CircuitData::compressCycles()
{
	DOUT("Compressing circuit...");

	// Each non-empty cycle will be added to a new vector. Those cycles will have their index (and the cycle indices of its gates)
	// updated to reflect the position in the compressed cycles vector.
	std::vector<Cycle> compressedCycles;
	int amountOfCompressions = 0;
	for (size_t i = 0; i < cycles.size(); i++)
	{
		// Add each non-empty cycle to the vector and update its relevant attributes.
		if (cycles[i].empty == false)
		{
			Cycle& cycle = cycles[i];
			cycle.index = i - amountOfCompressions;
			// Update the gates in the cycle with the new cycle index.
			for (size_t j = 0; j < cycle.gates.size(); j++)
			{
				for (GateProperties& gate : cycle.gates[j])
				{
					gate.cycle -= amountOfCompressions;
				}
			}
			compressedCycles.push_back(cycle);
		}
		else
		{
			amountOfCompressions++;
		}
	}

	cycles = compressedCycles;
}

void CircuitData::partitionCyclesWithOverlap()
{
	DOUT("Partitioning cycles with connection overlap...");

	// Find cycles with overlapping connections.
	for (Cycle& cycle : cycles)
	{
		if (cycle.gates[0].size() > 1)
		{
			// Find the multi-operand gates in this cycle.
			std::vector<std::reference_wrapper<GateProperties>> candidates;
			for (GateProperties& gate : cycle.gates[0])
			{
				if (gate.operands.size() + gate.creg_operands.size() > 1)
				{
					candidates.push_back(gate);
				}
			}

			// If more than one multi-operand gate has been found in this cycle, check if any of those gates overlap.
			if (candidates.size() > 1)
			{
				std::vector<std::vector<std::reference_wrapper<GateProperties>>> partition;
				for (GateProperties& candidate : candidates)
				{
					// Check if the gate can be placed in an existing chunk.
					bool placed = false;
					for (std::vector<std::reference_wrapper<GateProperties>>& chunk : partition)
					{
						// Check if the gate overlaps with any other gate in the chunk.
						bool gateOverlaps = false;
						const std::pair<GateOperand, GateOperand> edgeOperands1 = calculateEdgeOperands(getGateOperands(candidate), amountOfQubits);
						for (const GateProperties& gateInChunk : chunk)
						{
							const std::pair<GateOperand, GateOperand> edgeOperands2 = calculateEdgeOperands(getGateOperands(gateInChunk), amountOfQubits);
							if (edgeOperands1.first >= edgeOperands2.first && edgeOperands1.first <= edgeOperands2.second || 
								edgeOperands1.second >= edgeOperands2.first && edgeOperands1.second <= edgeOperands2.second)
							{
								gateOverlaps = true;
							}
						}

						// If the gate does not overlap with any gate in the chunk, add the gate to the chunk.
						if (!gateOverlaps)
						{
							chunk.push_back(candidate);
							placed = true;
							break;
						}
					}

					// If the gate has not been added to the chunk, add it to the partition in a new chunk.
					if (!placed)
					{
						partition.push_back({candidate});
					}
				}

				// If the partition has more than one chunk, we replace the original partition in the current cycle.
				if (partition.size() > 1)
				{
					DOUT("Divided cycle " << cycle.index << " into " << partition.size() << " chunks:");
					for (size_t i = 0; i < partition.size(); i++)
					{
						DOUT("Gates in chunk " << i << ":");
						for (const GateProperties& gate : partition[i])
						{
							DOUT("\t" << gate.name);
						}
					}

					cycle.gates = partition;
				}
			}
		}
	}
}

void CircuitData::cutEmptyCycles(const Layout layout)
{
	DOUT("Cutting empty cycles...");
	
	// Find cuttable ranges...
	cutCycleRangeIndices = findCuttableEmptyRanges(layout);
	// ... and cut them.
	for (const EndPoints& range : cutCycleRangeIndices)
	{
		for (int i = range.start; i <= range.end; i++)
		{
			cycles[i].cut = true;
		}
	}
}

std::vector<EndPoints> CircuitData::findCuttableEmptyRanges(const Layout layout) const
{
	DOUT("Checking for empty cycle ranges...");

	// Calculate the empty cycle ranges.
	std::vector<EndPoints> ranges;
	for (int i = 0; i < cycles.size(); i++)
	{
		// If an empty cycle has been found...
		if (cycles[i].empty == true)
		{
			const int start = i;
			int end = cycles.size() - 1;

			int j = i;
			// ... add cycles to the range until a non-empty cycle is found.
			while (j < cycles.size())
			{
				if (cycles[j].empty == false)
				{
					end = j - 1;
					break;
				}
				j++;
			}
			ranges.push_back({start, end});

			// Skip over the found range.
			i = j;
		}
	}

	// Check for empty cycle ranges above the threshold.
	std::vector<EndPoints> rangesAboveThreshold;
	for (const auto& range : ranges)
	{
		const int length = range.end - range.start + 1;
		DOUT("Range from " << range.start << " to " << range.end << " with length " << length << ".");

		if (length >= layout.cycles.emptyCycleThreshold)
		{
			DOUT("Found empty cycle range above threshold.");
			rangesAboveThreshold.push_back(range);
		}
	}

	return rangesAboveThreshold;
}

Cycle CircuitData::getCycle(const int index) const
{
	if (index > cycles.size())
		FATAL("Requested cycle index " << index << " is higher than max cycle " << (cycles.size() - 1) << "!");

	return cycles[index];
}

int CircuitData::getAmountOfCycles() const
{
	return cycles.size();
}

bool CircuitData::isCycleCut(const int cycleIndex) const
{
	return cycles[cycleIndex].cut;
}

bool CircuitData::isCycleFirstInCutRange(const int cycleIndex) const
{
	for (const EndPoints& range : cutCycleRangeIndices)
	{
		if (cycleIndex == range.start)
		{
			return true;
		}
	}

	return false;
}

void CircuitData::printProperties() const
{
	DOUT("[CIRCUIT DATA PROPERTIES]");

	DOUT("amountOfQubits: " << amountOfQubits);
	DOUT("amountOfClassicalBits: " << amountOfClassicalBits);
	DOUT("cycleDuration: " << cycleDuration);

	DOUT("cycles:");
	for (size_t cycle = 0; cycle < cycles.size(); cycle++)
	{
		DOUT("\tcycle: " << cycle << " empty: " << cycles[cycle].empty << " cut: " << cycles[cycle].cut);
	}

	DOUT("cutCycleRangeIndices");
	for (const auto& range : cutCycleRangeIndices)
	{
		DOUT("\tstart: " << range.start << " end: " << range.end);
	}
}

// ======================================================= //
// =                      Structure                      = //
// ======================================================= //

Structure::Structure(const Layout layout, const CircuitData circuitData) :
	layout(layout),
	cellDimensions({layout.grid.cellSize, calculateCellHeight(layout)}),
	cycleLabelsY(layout.grid.borderSize),
	bitLabelsX(layout.grid.borderSize)
{
	generateCellPositions(circuitData);
	generateBitLineSegments(circuitData);

	imageWidth = calculateImageWidth(circuitData);
	imageHeight = calculateImageHeight(circuitData);
}

int Structure::calculateCellHeight(const Layout layout) const
{
	if (layout.pulses.displayGatesAsPulses)
	{
		return layout.pulses.pulseRowHeightMicrowave + layout.pulses.pulseRowHeightFlux + layout.pulses.pulseRowHeightReadout;
	}
	else
	{
		return layout.grid.cellSize;
	}
}

int Structure::calculateImageWidth(const CircuitData circuitData) const
{
	const int amountOfCells = qbitCellPositions.size();
	const int left = amountOfCells > 0 ? getCellPosition(0, 0, QUANTUM).x0 : 0;
	const int right = amountOfCells > 0 ? getCellPosition(amountOfCells - 1, 0, QUANTUM).x1 : 0;
	const int imageWidthFromCells = right - left;

	return layout.bitLines.labelColumnWidth + imageWidthFromCells + layout.grid.borderSize * 2;
}

int Structure::calculateImageHeight(const CircuitData circuitData) const
{
	const int rowsFromQuantum = circuitData.amountOfQubits;
	const int rowsFromClassical = layout.bitLines.showClassicalLines
		? (layout.bitLines.groupClassicalLines ? (circuitData.amountOfClassicalBits > 0 ? 1 : 0) : circuitData.amountOfClassicalBits)
		: 0;
	const int heightFromOperands = (rowsFromQuantum + rowsFromClassical) * cellDimensions.height;

	return layout.cycles.cycleLabelsRowHeight + heightFromOperands + layout.grid.borderSize * 2;
}

void Structure::generateCellPositions(const CircuitData circuitData)
{
	// Calculate cell positions.
	int widthFromCycles = 0;
	for (int column = 0; column < circuitData.getAmountOfCycles(); column++)
	{
		const int amountOfChunks = circuitData.getCycle(column).gates.size();
		const int cycleWidth = (circuitData.isCycleCut(column) ? layout.cycles.cutCycleWidth : (cellDimensions.width * amountOfChunks));

		const int x0 = layout.grid.borderSize + layout.bitLines.labelColumnWidth + widthFromCycles;
		const int x1 = x0 + cycleWidth;

		// Quantum cell positions.
		std::vector<Position4> qColumnCells;
		for (int row = 0; row < circuitData.amountOfQubits; row++)
		{
			const int y0 = layout.grid.borderSize + layout.cycles.cycleLabelsRowHeight + row * cellDimensions.height;
			const int y1 = y0 + cellDimensions.height;
			qColumnCells.push_back({x0, y0, x1, y1});
		}
		qbitCellPositions.push_back(qColumnCells);
		// Classical cell positions.
		std::vector<Position4> cColumnCells;
		for (int row = 0; row < circuitData.amountOfClassicalBits; row++)
		{
			const int y0 = layout.grid.borderSize + layout.cycles.cycleLabelsRowHeight + 
				((layout.bitLines.groupClassicalLines ? 0 : row) + circuitData.amountOfQubits) * cellDimensions.height;
			const int y1 = y0 + cellDimensions.height;
			cColumnCells.push_back({x0, y0, x1, y1});
		}
		cbitCellPositions.push_back(cColumnCells);

		// Add the appropriate amount of width to the total width.
		if (layout.cycles.cutEmptyCycles)
		{
			if (circuitData.isCycleCut(column))
			{
				// if (column != 0 && !circuitData.isCycleCut(column - 1))
				if (column != circuitData.getAmountOfCycles() - 1 && !circuitData.isCycleCut(column + 1))
				{
					widthFromCycles += cellDimensions.width * layout.cycles.cutCycleWidthModifier;
				}
			}
			else
			{
				widthFromCycles += cycleWidth;
			}
		}
		else
		{
			widthFromCycles += cycleWidth;
		}
	}
}

void Structure::generateBitLineSegments(const CircuitData circuitData)
{
	// Calculate the bit line segments.
	for (int i = 0; i < circuitData.getAmountOfCycles(); i++)
	{
		const bool cut = circuitData.isCycleCut(i);
		bool reachedEnd = false;

		// Add more cycles to the segment until we reach a cycle that is cut if the current segment is not cut, or vice versa.
		for (int j = i; j < circuitData.getAmountOfCycles(); j++)
		{
			if (circuitData.isCycleCut(j) != cut)
			{
				const int start = getCellPosition(i, 0, QUANTUM).x0;
				const int end = getCellPosition(j, 0, QUANTUM).x0;
				DOUT("segment > range: [" << i << "," << (j - 1) << "], " << "position: [" << start << "," << end << "], cut: " << cut);
				bitLineSegments.push_back({{start, end}, cut});
				i = j - 1;
				break;
			}

			// Check if the last cycle has been reached, and exit the calculation if so.
			if (j == circuitData.getAmountOfCycles() - 1)
			{
				const int start = getCellPosition(i, 0, QUANTUM).x0;
				const int end = getCellPosition(j, 0, QUANTUM).x1;
				DOUT("segment > range: [" << i << "," << j << "], " << "position: [" << start << "," << end << "], cut: " << cut);
				bitLineSegments.push_back({{start, end}, cut});
				reachedEnd = true;
			}
		}
		
		if (reachedEnd) break;
	}
}

int Structure::getImageWidth() const
{
	return imageWidth;
}

int Structure::getImageHeight() const
{
	return imageHeight;
}

int Structure::getCycleLabelsY() const
{
	return cycleLabelsY;
}

int Structure::getBitLabelsX() const
{
	return bitLabelsX;
}

int Structure::getCircuitTopY() const
{
	return cycleLabelsY;
}

int Structure::getCircuitBotY() const
{
	std::vector<Position4> firstColumnPositions = layout.pulses.displayGatesAsPulses ? qbitCellPositions[0] : cbitCellPositions[0];
	Position4 botPosition = firstColumnPositions[firstColumnPositions.size() - 1];
	return botPosition.y1;
}

Dimensions Structure::getCellDimensions() const
{
	return cellDimensions;
}

Position4 Structure::getCellPosition(const int column, const int row, const BitType bitType) const
{
	switch (bitType)
	{
		case CLASSICAL:
			if (layout.pulses.displayGatesAsPulses)
				FATAL("Cannot get classical cell position when pulse visualization is enabled!");
			if (column >= cbitCellPositions.size())
				FATAL("cycle " << column << " is larger than max cycle " << cbitCellPositions.size() - 1 << " of structure!");
			if (row >= cbitCellPositions[column].size())
				FATAL("classical operand " << row << " is larger than max operand " << cbitCellPositions[column].size() - 1 << " of structure!");
			return cbitCellPositions[column][row];	

		case QUANTUM:
			if (column >= qbitCellPositions.size())
				FATAL("cycle " << column << " is larger than max cycle " << qbitCellPositions.size() - 1 << " of structure!");
			if (row >= qbitCellPositions[column].size())
				FATAL("quantum operand " << row << " is larger than max operand " << qbitCellPositions[column].size() - 1 << " of structure!");
			return qbitCellPositions[column][row];

		default:
			FATAL("Unknown bit type!");
	}
}

std::vector<std::pair<EndPoints, bool>> Structure::getBitLineSegments() const
{
	return bitLineSegments;
}

void Structure::printProperties() const
{
	DOUT("[STRUCTURE PROPERTIES]");

	DOUT("imageWidth: " << imageWidth);
	DOUT("imageHeight: " << imageHeight);

	DOUT("cycleLabelsY: " << cycleLabelsY);
	DOUT("bitLabelsX: " << bitLabelsX);

	DOUT("qbitCellPositions:");
	for (size_t cycle = 0; cycle < qbitCellPositions.size(); cycle++)
	{
		for (size_t operand = 0; operand < qbitCellPositions[cycle].size(); operand++)
		{
			DOUT("\tcell: [" << cycle << "," << operand << "]"
				<< " x0: " << qbitCellPositions[cycle][operand].x0
				<< " x1: " << qbitCellPositions[cycle][operand].x1
				<< " y0: " << qbitCellPositions[cycle][operand].y0
				<< " y1: " << qbitCellPositions[cycle][operand].y1);
		}
	}

	DOUT("cbitCellPositions:");
	for (size_t cycle = 0; cycle < cbitCellPositions.size(); cycle++)
	{
		for (size_t operand = 0; operand < cbitCellPositions[cycle].size(); operand++)
		{
			DOUT("\tcell: [" << cycle << "," << operand << "]"
				<< " x0: " << cbitCellPositions[cycle][operand].x0
				<< " x1: " << cbitCellPositions[cycle][operand].x1
				<< " y0: " << cbitCellPositions[cycle][operand].y0
				<< " y1: " << cbitCellPositions[cycle][operand].y1);
		}
	}

	DOUT("bitLineSegments:");
	for (const auto& segment : bitLineSegments)
	{
		DOUT("\tcut: " << segment.second << " start: " << segment.first.start << " end: " << segment.first.end);
	}
}

// ======================================================= //
// =                      Visualize                      = //
// ======================================================= //

void visualize(const ql::quantum_program* program, const std::string& configPath)
{
    IOUT("Starting visualization...");

	DOUT("Parsing visualizer configuration file.");
	Layout layout = parseConfiguration(configPath);
	
    DOUT("Validating layout...");
	validateLayout(layout);

    // Get the gate list from the program.
    DOUT("Getting gate list...");
    std::vector<GateProperties> gates;
    std::vector<ql::quantum_kernel> kernels = program->kernels;
    for (ql::quantum_kernel kernel : kernels)
    {
        circuit c = kernel.get_circuit();
		for (ql::gate* const gate : c)
		{
			GateProperties gateProperties {gate->name, gate->operands, gate->creg_operands, gate->duration, gate->cycle, gate->type(), gate->visual_type};
			gates.push_back(gateProperties);
		}
    }

	// Calculate circuit properties.
    DOUT("Calculating circuit properties...");
	const int cycleDuration = program->platform.cycle_time;
	DOUT("Cycle duration is: " + std::to_string(cycleDuration) + " ns.");
	fixMeasurementOperands(gates); // fixes measurement gates without classical operands
	CircuitData circuitData(gates, layout, cycleDuration);
	circuitData.printProperties();
    
	// Initialize the structure of the visualization.
	DOUT("Initializing visualization structure...");
	Structure structure(layout, circuitData);
	structure.printProperties();
	
	// Initialize image.
    DOUT("Initializing image...");
	const int numberOfChannels = 3;
	cimg_library::CImg<unsigned char> image(structure.getImageWidth(), structure.getImageHeight(), 1, numberOfChannels);
	image.fill(255);

	// Draw the cycle labels if the option has been set.
	if (layout.cycles.showCycleLabels)
	{
        DOUT("Drawing cycle numbers...");
		drawCycleLabels(image, layout, circuitData, structure);
	}

		// Draw the cycle edges if the option has been set.
	if (layout.cycles.showCycleEdges)
	{
        DOUT("Drawing cycle edges...");
		drawCycleEdges(image, layout, circuitData, structure);
	}

	if (layout.pulses.displayGatesAsPulses == false)
	{
		// Draw the quantum bit lines.
		DOUT("Drawing qubit lines...");
		for (int i = 0; i < circuitData.amountOfQubits; i++)
		{
			drawBitLine(image, layout, QUANTUM, i, circuitData, structure);
		}
			
		// Draw the classical lines if enabled.
		if (layout.bitLines.showClassicalLines)
		{
			// Draw the grouped classical bit lines if the option is set.
			if (circuitData.amountOfClassicalBits > 0 && layout.bitLines.groupClassicalLines)
			{
				DOUT("Drawing grouped classical bit lines...");
				drawGroupedClassicalBitLine(image, layout, circuitData, structure);
			}
			// Otherwise draw each classical bit line seperate.
			else
			{
				DOUT("Drawing ungrouped classical bit lines...");
				for (int i = 0; i < circuitData.amountOfClassicalBits; i++)
				{
					drawBitLine(image, layout, CLASSICAL, i, circuitData, structure);
				}
			}
		}
	}

	// Draw the cycles.
	for (size_t i = 0; i < circuitData.getAmountOfCycles(); i++)
	{
		// Only draw a cut cycle if its the first in its cut range.
		if (circuitData.isCycleCut(i))
		{
			if (i > 0 && !circuitData.isCycleCut(i - 1))
			{
				drawCycle(image, layout, circuitData, structure, circuitData.getCycle(i));
			}
		}
		// If the cycle is not cut, just draw it.
		else
		{
			drawCycle(image, layout, circuitData, structure, circuitData.getCycle(i));
		}
	}

	// Display the image.
    DOUT("Displaying image...");
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
	//TODO: replace these hardcoded assignments by automatic json to layout object mapping (should be possible with nlohmann json)
	if (config.count("cycles") == 1)
	{
		layout.cycles.showCycleLabels = config["cycles"].value("showCycleLabels", layout.cycles.showCycleLabels);
		layout.cycles.showCyclesInNanoSeconds = config["cycles"].value("showCyclesInNanoSeconds", layout.cycles.showCyclesInNanoSeconds);
		layout.cycles.cycleLabelsRowHeight = config["cycles"].value("cycleLabelsRowHeight", layout.cycles.cycleLabelsRowHeight);
		layout.cycles.fontHeight = config["cycles"].value("fontHeight", layout.cycles.fontHeight);
		layout.cycles.fontColor = config["cycles"].value("fontColor", layout.cycles.fontColor);

		layout.cycles.compressCycles = config["cycles"].value("compressCycles", layout.cycles.compressCycles);
		layout.cycles.showCycleEdges = config["cycles"].value("showCycleEdges", layout.cycles.showCycleEdges);
		layout.cycles.cycleEdgeColor = config["cycles"].value("cycleEdgeColor", layout.cycles.cycleEdgeColor);
		layout.cycles.cycleEdgeAlpha = config["cycles"].value("cycleEdgeAlpha", layout.cycles.cycleEdgeAlpha);

		layout.cycles.partitionCyclesWithOverlap = config["cycles"].value("partitionCyclesWithOverlap", layout.cycles.partitionCyclesWithOverlap);

		layout.cycles.cutEmptyCycles = config["cycles"].value("cutEmptyCycles", layout.cycles.cutEmptyCycles);
		layout.cycles.emptyCycleThreshold = config["cycles"].value("emptyCycleThreshold", layout.cycles.emptyCycleThreshold);
		layout.cycles.cutCycleWidth = config["cycles"].value("cutCycleWidth", layout.cycles.cutCycleWidth);
		layout.cycles.cutCycleWidthModifier = config["cycles"].value("cutCycleWidthModifier", layout.cycles.cutCycleWidthModifier);
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

	if (config.count("pulses") == 1)
	{
		layout.pulses.displayGatesAsPulses = config["pulses"].value("displayGatesAsPulses", layout.pulses.displayGatesAsPulses);

		layout.pulses.pulseRowHeightMicrowave = config["pulses"].value("pulseRowHeightMicrowave", layout.pulses.pulseRowHeightMicrowave);
		layout.pulses.pulseRowHeightFlux = config["pulses"].value("pulseRowHeightFlux", layout.pulses.pulseRowHeightFlux);
		layout.pulses.pulseRowHeightReadout = config["pulses"].value("pulseRowHeightReadout", layout.pulses.pulseRowHeightReadout);
		
		layout.pulses.pulseColorMicrowave = config["pulses"].value("pulseColorMicrowave", layout.pulses.pulseColorMicrowave);
		layout.pulses.pulseColorFlux = config["pulses"].value("pulseColorFlux", layout.pulses.pulseColorFlux);
		layout.pulses.pulseColorReadout = config["pulses"].value("pulseColorReadout", layout.pulses.pulseColorReadout);
	}

	// Load the custom instruction visualization parameters.
	if (config.count("instructions") == 1)
	{
		for (const auto& instruction : config["instructions"].items())
		{
			try
			{
				GateVisual gateVisual;
				json content = instruction.value();

				// Load the connection color.
				json connectionColor = content["connectionColor"];
				gateVisual.connectionColor[0] = connectionColor[0];
				gateVisual.connectionColor[1] = connectionColor[1];
				gateVisual.connectionColor[2] = connectionColor[2];
				DOUT("Connection color: [" 
					<< (int)gateVisual.connectionColor[0] << ","
					<< (int)gateVisual.connectionColor[1] << ","
					<< (int)gateVisual.connectionColor[2] << "]");

				// Load the individual nodes.
				json nodes = content["nodes"];
				int amountOfNodes = nodes.size();
				for (int i = 0; i < amountOfNodes; i++)
				{
					json node = nodes[i];
					
					std::array<unsigned char, 3> fontColor = {node["fontColor"][0], node["fontColor"][1], node["fontColor"][2]};
					std::array<unsigned char, 3> backgroundColor = {node["backgroundColor"][0], node["backgroundColor"][1], node["backgroundColor"][2]};
					std::array<unsigned char, 3> outlineColor = {node["outlineColor"][0], node["outlineColor"][1], node["outlineColor"][2]};
					
					NodeType nodeType;
					if (node["type"] == "NONE") {nodeType = NONE;} else
					if (node["type"] == "GATE") {nodeType = GATE;} else
					if (node["type"] == "CONTROL") {nodeType = CONTROL;} else
					if (node["type"] == "NOT") {nodeType = NOT;} else
					if (node["type"] == "CROSS") {nodeType = CROSS;}
					else
					{
						WOUT("Unknown gate display node type! Defaulting to type NONE...");
						nodeType = NONE;
					}
					
					Node loadedNode = 
					{
						nodeType,
						node["radius"],
						node["displayName"],
						node["fontHeight"],
						fontColor,
						backgroundColor,
						outlineColor
					};
					
					gateVisual.nodes.push_back(loadedNode);
					
					DOUT("[type: " << node["type"] << "] "
						<< "[radius: " << gateVisual.nodes.at(i).radius << "] "
						<< "[displayName: " << gateVisual.nodes.at(i).displayName << "] "
						<< "[fontHeight: " << gateVisual.nodes.at(i).fontHeight << "] "
						<< "[fontColor: "
							<< (int)gateVisual.nodes.at(i).fontColor[0] << ","
							<< (int)gateVisual.nodes.at(i).fontColor[1] << ","
							<< (int)gateVisual.nodes.at(i).fontColor[2] << "] "
						<< "[backgroundColor: "
							<< (int)gateVisual.nodes.at(i).backgroundColor[0] << ","
							<< (int)gateVisual.nodes.at(i).backgroundColor[1] << ","
							<< (int)gateVisual.nodes.at(i).backgroundColor[2] << "] "
						<< "[outlineColor: "
							<< (int)gateVisual.nodes.at(i).outlineColor[0] << ","
							<< (int)gateVisual.nodes.at(i).outlineColor[1] << ","
							<< (int)gateVisual.nodes.at(i).outlineColor[2] << "]");
				}

				layout.customGateVisuals.insert({instruction.key(), gateVisual});
			}
			catch (json::exception &e)
			{
				WOUT("Failed to load visualization parameters for instruction: '" << instruction.key() << "' \n\t" << std::string(e.what()));
			}
		}
	}
	else
	{
		WOUT("Did not find 'instructions' attribute! The visualizer will try to fall back on default gate visualizations.");
	}

	return layout;
}

void validateLayout(Layout& layout)
{
	//TODO: add more validation
	
	if (layout.cycles.emptyCycleThreshold < 1)
	{
		WOUT("Adjusting 'emptyCycleThreshold' to minimum value of 1. Value in configuration file is set to " << layout.cycles.emptyCycleThreshold << ".");
		layout.cycles.emptyCycleThreshold = 1;
	}

	if (layout.pulses.displayGatesAsPulses && layout.bitLines.showClassicalLines)
	{
		WOUT("Adjusting 'showClassicalLines' to false. Unable to be true when 'displayGatesAsPulses' is true!");
		layout.bitLines.showClassicalLines = false;
	}

	if (layout.pulses.displayGatesAsPulses && layout.cycles.partitionCyclesWithOverlap)
	{
		WOUT("Adjusting 'partitionCyclesWithOverlap' to false. It is unnecessary to partition cycles when 'displayGatesAsPulses' is true!");
		layout.cycles.partitionCyclesWithOverlap = false;
	}

	if (!layout.bitLines.drawLabels)	layout.bitLines.labelColumnWidth = 0;
	if (!layout.cycles.showCycleLabels)	layout.cycles.cycleLabelsRowHeight = 0;
}

int calculateAmountOfGateOperands(const GateProperties gate)
{
	return (int)gate.operands.size() + (int)gate.creg_operands.size();
}

std::vector<GateOperand> getGateOperands(const GateProperties gate)
{
	std::vector<GateOperand> operands;

	for (const size_t operand : gate.operands)
	{
		operands.push_back({QUANTUM, operand});
	}
	for (const size_t operand : gate.creg_operands)
	{
		operands.push_back({CLASSICAL, operand});
	}

	return operands;
}

std::pair<GateOperand, GateOperand> calculateEdgeOperands(const std::vector<GateOperand> operands, const int amountOfQubits)
{
	if (operands.size() < 2)
	{
		FATAL("Gate operands vector does not have multiple operands!");
	}

	GateOperand minOperand = operands[0];
	GateOperand maxOperand = operands[operands.size() - 1];
	for (const GateOperand& operand : operands)
	{
		const int row = (operand.bitType == QUANTUM) ? operand.index : operand.index + amountOfQubits;
		if (row < minOperand.index)
			minOperand = operand;
		if (row > maxOperand.index)
			maxOperand = operand;
	}

	return {minOperand, maxOperand};
}

void fixMeasurementOperands(std::vector<GateProperties>& gates)
{
	for (GateProperties& gate : gates)
	{
		// Check for a measurement gate without explicitly specified classical operand.
		if (isMeasurement(gate))
		{
			if (calculateAmountOfGateOperands(gate) == 1)
			{
				// Set classical measurement operand to the bit corresponding to the measuremens qubit number.
				DOUT("Found measurement gate with no classical operand. Assuming default classical operand.");
				const int cbit = gate.operands[0];
				gate.creg_operands.push_back(cbit);
			}
		}
	}
}

bool isMeasurement(const GateProperties gate)
{
	//TODO: this method of checking for measurement gates is not very robust and relies entirely on the user naming their instructions in a certain way!
	return (gate.name.find("measure") != std::string::npos);
}

Dimensions calculateTextDimensions(const std::string& text, const int fontHeight, const Layout layout)
{
	const char* chars = text.c_str();
	cimg_library::CImg<unsigned char> imageTextDimensions;
	const char color = 1;
	imageTextDimensions.draw_text(0, 0, chars, &color, 0, 1, fontHeight);

	return Dimensions { (int) imageTextDimensions.width(), (int) imageTextDimensions.height() };
}

void drawCycleLabels(cimg_library::CImg<unsigned char>& image, const Layout layout, const CircuitData circuitData, const Structure structure)
{
	for (int i = 0; i < circuitData.getAmountOfCycles(); i++)
	{
		std::string cycleLabel = "";
		int cellWidth = 0;
		if (circuitData.isCycleCut(i))
		{
			if (!circuitData.isCycleFirstInCutRange(i))
			{
				continue;
			}
			cellWidth = layout.cycles.cutCycleWidth;
			cycleLabel = "...";
		}
		else
		{
			// cellWidth = structure.getCellDimensions().width;
			const Position4 cellPosition = structure.getCellPosition(i, 0, QUANTUM);
			cellWidth = cellPosition.x1 - cellPosition.x0;
			if (layout.cycles.showCyclesInNanoSeconds)
			{
				cycleLabel = std::to_string(i * circuitData.cycleDuration);
			}
			else
			{
				cycleLabel = std::to_string(i);
			}
		}

		Dimensions textDimensions = calculateTextDimensions(cycleLabel, layout.cycles.fontHeight, layout);

		const int xGap = (cellWidth - textDimensions.width) / 2;
		const int yGap = (layout.cycles.cycleLabelsRowHeight - textDimensions.height) / 2;
		const int xCycle = structure.getCellPosition(i, 0, QUANTUM).x0 + xGap;
		const int yCycle = structure.getCycleLabelsY() + yGap;

		image.draw_text(xCycle, yCycle, cycleLabel.c_str(), layout.cycles.fontColor.data(), 0, 1, layout.cycles.fontHeight);
	}
}

void drawCycleEdges(cimg_library::CImg<unsigned char>& image, const Layout layout, const CircuitData circuitData, const Structure structure)
{
	for (size_t i = 0; i < circuitData.getAmountOfCycles(); i++)
	{
		if (i == 0) continue;
		if (circuitData.isCycleCut(i) && circuitData.isCycleCut(i - 1)) continue;

		const int xCycle = structure.getCellPosition(i, 0, QUANTUM).x0;
		const int y0 = structure.getCircuitTopY();
		const int y1 = structure.getCircuitBotY();

		image.draw_line(xCycle, y0, xCycle, y1, layout.cycles.cycleEdgeColor.data(), layout.cycles.cycleEdgeAlpha, 0xF0F0F0F0);
	}
}

void drawBitLine(cimg_library::CImg<unsigned char> &image, const Layout layout, const BitType bitType, const int row, const CircuitData circuitData, const Structure structure)
{
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

	for (const std::pair<EndPoints, bool>& segment : structure.getBitLineSegments())
	{
		const int y = structure.getCellPosition(0, row, bitType).y0 + structure.getCellDimensions().height / 2;
		// Check if the segment is a cut segment.
		if (segment.second == true)
		{
			const int height = structure.getCellDimensions().height / 8;
			const int width = segment.first.end - segment.first.start;
			
			image.draw_line(segment.first.start,					y,			segment.first.start + width / 3,		y - height,	bitLineColor.data());
			image.draw_line(segment.first.start + width / 3,		y - height,	segment.first.start + width / 3 * 2,	y + height,	bitLineColor.data());
			image.draw_line(segment.first.start + width / 3 * 2,	y + height,	segment.first.end,						y,			bitLineColor.data());
		}
		else
		{
			image.draw_line(segment.first.start, y, segment.first.end, y, bitLineColor.data());
		}
	}

	// Draw the bit line label if enabled.
	if (layout.bitLines.drawLabels)
	{
		const std::string bitTypeText = (bitType == CLASSICAL) ? "c" : "q";
		std::string label = bitTypeText + std::to_string(row);
		Dimensions textDimensions = calculateTextDimensions(label, layout.bitLines.fontHeight, layout);

		const int xGap = (structure.getCellDimensions().width - textDimensions.width) / 2;
		const int yGap = (structure.getCellDimensions().height - textDimensions.height) / 2;
		const int xLabel = structure.getBitLabelsX() + xGap;
		const int yLabel = structure.getCellPosition(0, row, bitType).y0 + yGap;

		image.draw_text(xLabel, yLabel, label.c_str(), bitLabelColor.data(), 0, 1, layout.bitLines.fontHeight);
	}
}

void drawGroupedClassicalBitLine(cimg_library::CImg<unsigned char>& image, const Layout layout, const CircuitData circuitData, const Structure structure)
{
	const int y = structure.getCellPosition(0, 0, CLASSICAL).y0 + structure.getCellDimensions().height / 2;

	// Draw the segments of the double line.
	for (const std::pair<EndPoints, bool>& segment : structure.getBitLineSegments())
	{
		// Check if the segment is a cut segment.
		if (segment.second == true)
		{
			const int height = structure.getCellDimensions().height / 8;
			const int width = segment.first.end - segment.first.start;
			
			image.draw_line(segment.first.start, y - layout.bitLines.groupedClassicalLineGap,
							segment.first.start + width / 3, y - height - layout.bitLines.groupedClassicalLineGap,
							layout.bitLines.cBitLineColor.data());
			image.draw_line(segment.first.start + width / 3, y - height - layout.bitLines.groupedClassicalLineGap,
							segment.first.start + width / 3 * 2, y + height - layout.bitLines.groupedClassicalLineGap,
							layout.bitLines.cBitLineColor.data());
			image.draw_line(segment.first.start + width / 3 * 2, y + height - layout.bitLines.groupedClassicalLineGap,
							segment.first.end, y - layout.bitLines.groupedClassicalLineGap,
							layout.bitLines.cBitLineColor.data());

			image.draw_line(segment.first.start, y + layout.bitLines.groupedClassicalLineGap,
							segment.first.start + width / 3, y - height + layout.bitLines.groupedClassicalLineGap,
							layout.bitLines.cBitLineColor.data());
			image.draw_line(segment.first.start + width / 3, y - height + layout.bitLines.groupedClassicalLineGap,
							segment.first.start + width / 3 * 2, y + height + layout.bitLines.groupedClassicalLineGap,
							layout.bitLines.cBitLineColor.data());
			image.draw_line(segment.first.start + width / 3 * 2, y + height + layout.bitLines.groupedClassicalLineGap,
							segment.first.end, y + layout.bitLines.groupedClassicalLineGap,
							layout.bitLines.cBitLineColor.data());
		}
		else
		{
			image.draw_line(segment.first.start, y - layout.bitLines.groupedClassicalLineGap, 
				segment.first.end, y - layout.bitLines.groupedClassicalLineGap, layout.bitLines.cBitLineColor.data());
			image.draw_line(segment.first.start, y + layout.bitLines.groupedClassicalLineGap, 
				segment.first.end, y + layout.bitLines.groupedClassicalLineGap, layout.bitLines.cBitLineColor.data());	
		}
	}

	// Draw the dashed line plus classical bit amount number on the first segment.
	std::pair<EndPoints, bool> firstSegment = structure.getBitLineSegments()[0];
	//TODO: store the dashed line parameters in the layout object
	image.draw_line(firstSegment.first.start + 8, y + layout.bitLines.groupedClassicalLineGap + 2, firstSegment.first.start + 12, y - layout.bitLines.groupedClassicalLineGap - 3, layout.bitLines.cBitLineColor.data());
	const std::string label = std::to_string(circuitData.amountOfClassicalBits);
	//TODO: fix these hardcoded parameters
	const int xLabel = firstSegment.first.start + 8;
	const int yLabel = y - layout.bitLines.groupedClassicalLineGap - 3 - 13;
	image.draw_text(xLabel, yLabel, label.c_str(), layout.bitLines.cBitLabelColor.data(), 0, 1, layout.bitLines.fontHeight);

	// Draw the bit line label if enabled.
	if (layout.bitLines.drawLabels)
	{
		const std::string label = "C";
		Dimensions textDimensions = calculateTextDimensions(label, layout.bitLines.fontHeight, layout);

		const int xGap = (structure.getCellDimensions().width - textDimensions.width) / 2;
		const int yGap = (structure.getCellDimensions().height - textDimensions.height) / 2;
		const int xLabel = structure.getBitLabelsX() + xGap;
		const int yLabel = structure.getCellPosition(0, 0, CLASSICAL).y0 + yGap;

		image.draw_text(xLabel, yLabel, label.c_str(), layout.bitLines.cBitLabelColor.data(), 0, 1, layout.bitLines.fontHeight);
	}
}

void drawWiggle(cimg_library::CImg<unsigned char>& image, const int x0, const int x1, const int y, const int width, const int height, const std::array<unsigned char, 3> color)
{
	image.draw_line(x0,					y,			x0 + width / 3,		y - height,	color.data());
	image.draw_line(x0 + width / 3,		y - height,	x0 + width / 3 * 2,	y + height,	color.data());
	image.draw_line(x0 + width / 3 * 2,	y + height,	x1,					y,			color.data());
}

void drawCycle(cimg_library::CImg<unsigned char>& image, const Layout layout, const CircuitData circuitData, const Structure structure, const Cycle cycle)
{
	int amountOfGates = 0;
	for (const auto& chunk : cycle.gates)
	{
		amountOfGates += chunk.size();
	}
	DOUT("Drawing cycle " << cycle.index << " with " << amountOfGates << " gates:");

	// Draw each of the chunks in the cycle's gate partition.
	for (size_t chunkIndex = 0; chunkIndex < cycle.gates.size(); chunkIndex++)
	{
		const int chunkOffset = chunkIndex * structure.getCellDimensions().width;

		// Visualize the gates as pulses on a microwave, flux and readout line.
		if (layout.pulses.displayGatesAsPulses)
		{
			// Only draw wiggles if the cycle is cut.
			if (circuitData.isCycleCut(cycle.index))
			{
				for (size_t qubitIndex = 0; qubitIndex < circuitData.amountOfQubits; qubitIndex++)
				{
					const Position4 cellPosition = structure.getCellPosition(cycle.index, qubitIndex, QUANTUM);
					
					// Draw wiggle on microwave line.
					drawWiggle(image,
							   cellPosition.x0,
							   cellPosition.x1,
							   cellPosition.y0 + layout.pulses.pulseRowHeightMicrowave / 2,
							   cellPosition.x1 - cellPosition.x0,
							   layout.pulses.pulseRowHeightMicrowave / 8,
							   layout.pulses.pulseColorMicrowave);
					
					// Draw wiggle on flux line.
					drawWiggle(image,
							   cellPosition.x0,
							   cellPosition.x1,
							   cellPosition.y0 + layout.pulses.pulseRowHeightMicrowave + layout.pulses.pulseRowHeightFlux / 2,
							   cellPosition.x1 - cellPosition.x0,
							   layout.pulses.pulseRowHeightFlux / 8,
							   layout.pulses.pulseColorFlux);
					
					// Draw wiggle on readout line.
					drawWiggle(image,
							   cellPosition.x0,
							   cellPosition.x1,
							   cellPosition.y0 + layout.pulses.pulseRowHeightMicrowave + layout.pulses.pulseRowHeightFlux + layout.pulses.pulseRowHeightReadout / 2,
							   cellPosition.x1 - cellPosition.x0,
							   layout.pulses.pulseRowHeightReadout / 8,
							   layout.pulses.pulseColorReadout);
				}
				
				return;
			}

			// These vectors will be used to draw flat lines on microwave, flux or readout lines where no pulse was specified.
			std::vector<bool> flatMicrowaveLines(circuitData.amountOfQubits, true);
			std::vector<bool> flatFluxLines(circuitData.amountOfQubits, true);
			std::vector<bool> flatReadoutLines(circuitData.amountOfQubits, true);

			// Draw a pulse for each of the gates.
			for (const GateProperties& gate : cycle.gates[chunkIndex])
			{
				const std::vector<GateOperand> operands = getGateOperands(gate);

				if (isMeasurement(gate))
				{
					flatReadoutLines[operands[0].index] = false;

					// TODO: draw readout pulse

					continue;
				}
				if (operands.size() == 1)
				{
					flatMicrowaveLines[operands[0].index] = false;

					// TODO: draw microwave pulse

					continue;
				}
				if (operands.size() > 1)
				{
					for (const GateOperand& operand : operands)
					{
						if (operand.bitType == QUANTUM)
						{
							flatFluxLines[operand.index] = false;
						}
					}

					// TODO: draw flux pulse

					continue;
				}
			}

			// Draw each line segment that did not have a pulse.
			for (size_t qubitIndex = 0; qubitIndex < circuitData.amountOfQubits; qubitIndex++)
			{
				const Position4 cellPosition = structure.getCellPosition(cycle.index, qubitIndex, QUANTUM);
				const int cellWidth = circuitData.isCycleCut(cycle.index) ? layout.cycles.cutCycleWidth : structure.getCellDimensions().width;

				// Draw flat microwave line.
				if (flatMicrowaveLines[qubitIndex] == true)
				{
					const int y = cellPosition.y0 + layout.pulses.pulseRowHeightMicrowave / 2;
					image.draw_line(cellPosition.x0 + chunkOffset,
									y,
									cellPosition.x0 + chunkOffset + cellWidth,
									y,
									layout.pulses.pulseColorMicrowave.data());
				}

				// Draw flat flux line.
				if (flatFluxLines[qubitIndex] == true)
				{
					const int y = cellPosition.y0 + layout.pulses.pulseRowHeightMicrowave + layout.pulses.pulseRowHeightFlux / 2;
					image.draw_line(cellPosition.x0 + chunkOffset,
									y,
									cellPosition.x0 + chunkOffset + cellWidth,
									y,
									layout.pulses.pulseColorFlux.data());
				}

				// Draw flat readout line.
				if (flatReadoutLines[qubitIndex] == true)
				{
					const int y = cellPosition.y0 + layout.pulses.pulseRowHeightMicrowave + layout.pulses.pulseRowHeightFlux + layout.pulses.pulseRowHeightReadout / 2;
					image.draw_line(cellPosition.x0 + chunkOffset,
									y,
									cellPosition.x0 + chunkOffset + cellWidth,
									y,
									layout.pulses.pulseColorReadout.data());
				}
			}
		}
		// Visualize the gates as abstract representations.
		else
		{
			// Draw each of the gates in the current chunk.
			for (const GateProperties& gate : cycle.gates[chunkIndex])
			{
				drawGate(image, layout, circuitData, gate, structure, chunkOffset);
			}
		}
	}
}

void drawGate(cimg_library::CImg<unsigned char>& image, const Layout layout, const CircuitData circuitData, const GateProperties gate, 
	const Structure structure, const int chunkOffset)
{
	// Get the gate visualization parameters.
	GateVisual gateVisual;
	if (gate.type == __custom_gate__)
	{
		if (layout.customGateVisuals.count(gate.visual_type) == 1)
		{
			DOUT("Found visual for custom gate: '" << gate.name << "'");
			gateVisual = layout.customGateVisuals.at(gate.visual_type);
		}
		else
		{
			// TODO: try to recover by matching gate name with a default visual name
			// TODO: if the above fails, display a dummy gate
			WOUT("Did not find visual for custom gate: '" << gate.name << "', skipping gate!");
			return;
		}
	}
	else
	{
		DOUT("Default gate found. Using default visualization!");
		gateVisual = layout.defaultGateVisuals.at(gate.type);
	}

	// Fetch the operands used by this gate.
	DOUT(gate.name);
	std::vector<GateOperand> operands = getGateOperands(gate);
	for (const GateOperand& operand : operands)
	{
		DOUT("bitType: " << operand.bitType << " value: " << operand.index);
	}

	// Check for correct amount of nodes.
	if (operands.size() != gateVisual.nodes.size())
	{
		WOUT("Amount of gate operands: " << operands.size() << " and visualization nodes: " << gateVisual.nodes.size() << " are not equal. Skipping gate with name: '" << gate.name << "' ...");
		return;
	}

	if (operands.size() > 1)
	{
		// Draw the lines between each node. If this is done before drawing the nodes, there is no need to calculate line segments, we can just draw one
		// big line between the nodes and the nodes will be drawn on top of those.
		// Note: does not work with transparent nodes! If those are ever implemented, the connection line drawing will need to be changed!

		DOUT("Setting up multi-operand gate...");
		std::pair<GateOperand, GateOperand> edgeOperands = calculateEdgeOperands(operands, circuitData.amountOfQubits);
		GateOperand minOperand = edgeOperands.first;
		GateOperand maxOperand = edgeOperands.second;

		const int column = (int)gate.cycle;
		DOUT("minOperand.bitType: " << minOperand.bitType << " minOperand.operand " << minOperand.index);
		DOUT("maxOperand.bitType: " << maxOperand.bitType << " maxOperand.operand " << maxOperand.index);
		DOUT("cycle: " << column);

		const Position4 topCellPosition = structure.getCellPosition(column, minOperand.index, minOperand.bitType);
		const Position4 bottomCellPosition = structure.getCellPosition(column, maxOperand.index, maxOperand.bitType);
		const Position4 connectionPosition =
		{
			topCellPosition.x0 + chunkOffset + structure.getCellDimensions().width / 2,
			topCellPosition.y0 + structure.getCellDimensions().height / 2,
			bottomCellPosition.x0 + chunkOffset + structure.getCellDimensions().width / 2,
			bottomCellPosition.y0 + structure.getCellDimensions().height / 2,
		};

		//TODO: probably have connection line type as part of a gate's visual definition
		if (isMeasurement(gate))
		{
			if (layout.measurements.drawConnection && layout.bitLines.showClassicalLines)
			{
				const int groupedClassicalLineOffset = layout.bitLines.groupClassicalLines ? layout.bitLines.groupedClassicalLineGap : 0;

				image.draw_line(connectionPosition.x0 - layout.measurements.lineSpacing, connectionPosition.y0,
					connectionPosition.x1 - layout.measurements.lineSpacing, connectionPosition.y1 - layout.measurements.arrowSize - groupedClassicalLineOffset,
					gateVisual.connectionColor.data());

				image.draw_line(connectionPosition.x0 + layout.measurements.lineSpacing, connectionPosition.y0,
					connectionPosition.x1 + layout.measurements.lineSpacing, connectionPosition.y1 - layout.measurements.arrowSize - groupedClassicalLineOffset,
					gateVisual.connectionColor.data());

				const int x0 = connectionPosition.x1 - layout.measurements.arrowSize / 2;
				const int y0 = connectionPosition.y1 - layout.measurements.arrowSize - groupedClassicalLineOffset;
				const int x1 = connectionPosition.x1 + layout.measurements.arrowSize / 2;
				const int y1 = connectionPosition.y1 - layout.measurements.arrowSize - groupedClassicalLineOffset;
				const int x2 = connectionPosition.x1;
				const int y2 = connectionPosition.y1 - groupedClassicalLineOffset;
				image.draw_triangle(x0, y0, x1, y1, x2, y2, gateVisual.connectionColor.data(), 1);
			}
		}
		else
		{
			image.draw_line(connectionPosition.x0, connectionPosition.y0, connectionPosition.x1, connectionPosition.y1, gateVisual.connectionColor.data());
		}
		DOUT("Finished setting up multi-operand gate");
	}

	// Draw the gate duration outline if the option has been set.
	if (!layout.cycles.compressCycles && layout.cycles.showGateDurationOutline)
	{
		DOUT("Drawing gate duration outline...");
		const int gateDurationInCycles = ((int)gate.duration) / circuitData.cycleDuration;
		// Only draw the gate outline if the gate takes more than one cycle.
		if (gateDurationInCycles > 1)
		{
			for (int i = 0; i < operands.size(); i++)
			{
				const int columnStart = (int)gate.cycle;
				const int columnEnd = columnStart + gateDurationInCycles - 1;
				const int row = (i >= gate.operands.size()) ? gate.creg_operands[i - gate.operands.size()] : gate.operands[i];
				DOUT("i: " << i << " size: " << gate.operands.size() << " value: " << gate.operands[i]);

				const int x0 = structure.getCellPosition(columnStart, row, QUANTUM).x0 + chunkOffset + layout.cycles.gateDurationGap;
				const int y0 = structure.getCellPosition(columnStart, row, QUANTUM).y0 + layout.cycles.gateDurationGap;
				const int x1 = structure.getCellPosition(columnEnd, row, QUANTUM).x1 - layout.cycles.gateDurationGap;
				const int y1 = structure.getCellPosition(columnEnd, row, QUANTUM).y1 - layout.cycles.gateDurationGap;

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
	DOUT("Drawing gate nodes...");
	for (int i = 0; i < operands.size(); i++)
	{
		DOUT("Drawing gate node with index: " << i << "...");
		//TODO: change the try-catch later on! the gate config will be read from somewhere else than the default layout
		try
		{
			const Node node = gateVisual.nodes.at(i);
			const BitType operandType = (i >= gate.operands.size()) ? CLASSICAL : QUANTUM;
			const int index = (operandType == QUANTUM) ? i : (i - (int)gate.operands.size());

			const Cell cell =
			{
				(int)gate.cycle,
				operandType == CLASSICAL ? (int)gate.creg_operands.at(index) + circuitData.amountOfQubits : (int)gate.operands.at(index),
				chunkOffset,
				operandType
			};

			switch (node.type)
			{
				case NONE:		DOUT("node.type = NONE"); break; // Do nothing.
				case GATE:		DOUT("node.type = GATE"); drawGateNode(image, layout, structure, node, cell); break;
				case CONTROL:	DOUT("node.type = CONTROL"); drawControlNode(image, layout, structure, node, cell); break;
				case NOT:		DOUT("node.type = NOT"); drawNotNode(image, layout, structure, node, cell); break;
				case CROSS:		DOUT("node.type = CROSS"); drawCrossNode(image, layout, structure, node, cell); break;
				default:        WOUT("Unknown gate display node type!"); break;
			}
		}
		catch (const std::out_of_range& e)
		{
			WOUT(std::string(e.what()));
			return;
		}
		
		DOUT("Finished drawing gate node with index: " << i << "...");
	}
}

void drawGateNode(cimg_library::CImg<unsigned char>& image, const Layout layout, const Structure structure, const Node node, const Cell cell)
{
	const int xGap = (structure.getCellDimensions().width - node.radius * 2) / 2;
	const int yGap = (structure.getCellDimensions().height - node.radius * 2) / 2;

	const Position4 cellPosition = structure.getCellPosition(cell.col, cell.row, cell.bitType);
	const Position4 position =
	{
		cellPosition.x0 + cell.chunkOffset + xGap,
		cellPosition.y0 + yGap,
		cellPosition.x0 + cell.chunkOffset + structure.getCellDimensions().width - xGap,
		cellPosition.y1 - yGap
	};

	// Draw the gate background.
	image.draw_rectangle(position.x0, position.y0, position.x1, position.y1, node.backgroundColor.data());
	image.draw_rectangle(position.x0, position.y0, position.x1, position.y1, node.outlineColor.data(), 1, 0xFFFFFFFF);

	// Draw the gate symbol. The width and height of the symbol are calculated first to correctly position the symbol within the gate.
	Dimensions textDimensions = calculateTextDimensions(node.displayName, node.fontHeight, layout);
	image.draw_text(position.x0 + (node.radius * 2 - textDimensions.width) / 2, position.y0 + (node.radius * 2 - textDimensions.height) / 2, node.displayName.c_str(), node.fontColor.data(), 0, 1, node.fontHeight);
}

void drawControlNode(cimg_library::CImg<unsigned char>& image, const Layout layout, const Structure structure, const Node node, const Cell cell)
{
	const Position4 cellPosition = structure.getCellPosition(cell.col, cell.row, cell.bitType);
	const Position2 position =
	{
		cellPosition.x0 + cell.chunkOffset + structure.getCellDimensions().width / 2,
		cellPosition.y0 + cell.chunkOffset + structure.getCellDimensions().height / 2
	};

	image.draw_circle(position.x, position.y, node.radius, node.backgroundColor.data());
}

void drawNotNode(cimg_library::CImg<unsigned char>& image, const Layout layout, const Structure structure, const Node node, const Cell cell)
{
	// TODO: allow for filled not node instead of only an outline not node

	const Position4 cellPosition = structure.getCellPosition(cell.col, cell.row, cell.bitType);
	const Position2 position =
	{
		cellPosition.x0 + cell.chunkOffset + structure.getCellDimensions().width / 2,
		cellPosition.y0 + cell.chunkOffset + structure.getCellDimensions().height / 2
	};

	// Draw the outlined circle.
	image.draw_circle(position.x, position.y, node.radius, node.backgroundColor.data(), 1, 0xFFFFFFFF);

	// Draw two lines to represent the plus sign.
	const int xHor0 = position.x - node.radius;
	const int xHor1 = position.x + node.radius;
	const int yHor = position.y;

	const int xVer = position.x;
	const int yVer0 = position.y - node.radius;
	const int yVer1 = position.y + node.radius;

	image.draw_line(xHor0, yHor, xHor1, yHor, node.backgroundColor.data());
	image.draw_line(xVer, yVer0, xVer, yVer1, node.backgroundColor.data());
}

void drawCrossNode(cimg_library::CImg<unsigned char>& image, const Layout layout, const Structure structure, const Node node, const Cell cell)
{
	const Position4 cellPosition = structure.getCellPosition(cell.col, cell.row, cell.bitType);
	const Position2 position =
	{
		cellPosition.x0 + cell.chunkOffset + structure.getCellDimensions().width / 2,
		cellPosition.y0 + cell.chunkOffset + structure.getCellDimensions().height / 2
	};

	// Draw two diagonal lines to represent the cross.
	const int x0 = position.x - node.radius;
	const int y0 = position.y - node.radius;
	const int x1 = position.x + node.radius;
	const int y1 = position.y + node.radius;

	image.draw_line(x0, y0, x1, y1, node.backgroundColor.data());
	image.draw_line(x0, y1, x1, y0, node.backgroundColor.data());
}

#endif //WITH_VISUALIZER

} // ql
