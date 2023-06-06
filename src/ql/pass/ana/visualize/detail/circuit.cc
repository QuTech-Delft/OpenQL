/** \file
 * Definition of the circuit visualizer.
 */

#ifdef WITH_VISUALIZER

#include "circuit.h"

#include <regex>
#include "ql/utils/exception.h"
#include "common.h"
#include "ql/ir/ir.h"

namespace ql {
namespace pass {
namespace ana {
namespace visualize {
namespace detail {

using namespace utils;

// ======================================================= //
// =                     CircuitData                     = //
// ======================================================= //

CircuitData::CircuitData(Vec<GateProperties> &gates, const CircuitLayout &layout) :
    cycles(generateCycles(gates)),
    amountOfQubits(calculateAmountOfBits(gates, &GateProperties::operands)),
    amountOfClassicalBits(calculateAmountOfBits(gates, &GateProperties::creg_operands))
{
    if (layout.cycles.areCompressed())      compressCycles();
    if (layout.cycles.arePartitioned())     partitionCyclesWithOverlap();
    if (layout.cycles.cutting.isEnabled())  cutEmptyCycles(layout);
}

Vec<Cycle> CircuitData::generateCycles(Vec<GateProperties> &gates) const {
    QL_DOUT("Generating cycles...");

    // Calculate the amount of cycles. If there are gates with undefined cycle
    // indices, visualize the circuit sequentially.
    Vec<Cycle> cycles{};
    Int amountOfCycles = calculateAmountOfCycles(gates);

    // Generate the cycles.
    for (Int i = 0; i < amountOfCycles; i++) {
        // Generate the first chunk of the gate partition for this cycle.
        // All gates in this cycle will be added to this chunk first, later on
        // they will be divided based on connectivity (if enabled).
        Vec<Vec<std::reference_wrapper<GateProperties>>> partition;
        const Vec<std::reference_wrapper<GateProperties>> firstChunk;
        partition.push_back(firstChunk);

        cycles.emplace_back(i, true, false, partition);
    }
    // Mark non-empty cycles and add gates to their corresponding cycles.
    for (GateProperties &gate : gates) {
        cycles[gate.cycle].empty = false;
        cycles[gate.cycle].gates[0].push_back(gate);
    }

    return cycles;
}

void CircuitData::compressCycles() {
    QL_DOUT("Compressing circuit...");

    // Each non-empty cycle will be added to a new vector. Those cycles will
    // have their index (and the cycle indices of its gates) updated to reflect
    // the position in the compressed cycles vector.
    Vec<Cycle> compressedCycles;
    Int amountOfCompressions = 0;
    for (UInt i = 0; i < cycles.size(); i++) {
        // Add each non-empty cycle to the vector and update its relevant
        // attributes.
        if (!cycles[i].empty) {
            Cycle &cycle = cycles[i];
            cycle.index = utoi(i) - amountOfCompressions;
            // Update the gates in the cycle with the new cycle index.
            for (UInt j = 0; j < cycle.gates.size(); j++) {
                for (GateProperties &gate : cycle.gates[j]) {
                    gate.cycle -= amountOfCompressions;
                }
            }
            compressedCycles.push_back(cycle);
        } else {
            amountOfCompressions++;
        }
    }

    cycles = compressedCycles;
}

void CircuitData::partitionCyclesWithOverlap()
{
    QL_DOUT("Partioning cycles with connections overlap...");

    // Find cycles with overlapping connections.
    for (Cycle &cycle : cycles) {
        if (cycle.gates[0].size() > 1) {
            // Find the multi-operand gates in this cycle.
            Vec<std::reference_wrapper<GateProperties>> candidates;
            for (GateProperties &gate : cycle.gates[0]) {
                if (gate.operands.size() + gate.creg_operands.size() > 1) {
                    candidates.push_back(gate);
                }
            }

            // If more than one multi-operand gate has been found in this cycle,
            // check if any of those gates overlap.
            if (candidates.size() > 1) {
                Vec<Vec<std::reference_wrapper<GateProperties>>> partition;
                for (GateProperties &candidate : candidates) {
                    // Check if the gate can be placed in an existing chunk.
                    Bool placed = false;
                    for (Vec<std::reference_wrapper<GateProperties>> &chunk : partition) {
                        // Check if the gate overlaps with any other gate in the
                        // chunk.
                        Bool gateOverlaps = false;
                        const Pair<GateOperand, GateOperand> edgeOperands1 = calculateEdgeOperands(getGateOperands(candidate), amountOfQubits);
                        for (const GateProperties &gateInChunk : chunk) {
                            const Pair<GateOperand, GateOperand> edgeOperands2 = calculateEdgeOperands(getGateOperands(gateInChunk), amountOfQubits);
                            if ((edgeOperands1.first >= edgeOperands2.first && edgeOperands1.first <= edgeOperands2.second) ||
                                (edgeOperands1.second >= edgeOperands2.first && edgeOperands1.second <= edgeOperands2.second))
                            {
                                gateOverlaps = true;
                            }
                        }

                        // If the gate does not overlap with any gate in the
                        // chunk, add the gate to the chunk.
                        if (!gateOverlaps) {
                            chunk.push_back(candidate);
                            placed = true;
                            break;
                        }
                    }

                    // If the gate has not been added to the chunk, add it to
                    // the partition in a new chunk.
                    if (!placed) {
                        partition.push_back({candidate});
                    }
                }

                // If the partition has more than one chunk, we replace the
                // original partition in the current cycle.
                if (partition.size() > 1) {
                    QL_DOUT("Divided cycle " << cycle.index << " Into " << partition.size() << " chunks:");
                    for (UInt i = 0; i < partition.size(); i++) {
                        QL_DOUT("Gates in chunk " << i << ":");
                        for (const GateProperties &gate : partition[i]) {
                            QL_DOUT("\t" << gate.name);
                        }
                    }

                    cycle.gates = partition;
                }
            }
        }
    }
}

void CircuitData::cutEmptyCycles(const CircuitLayout &layout) {
    QL_DOUT("Cutting empty cycles...");

    if (layout.pulses.areEnabled()) {
        //TODO: an empty cycle as defined in pulse visualization is a cycle in
        //      which no lines for each qubit have a pulse going
        //TODO: implement checking for the above and mark those cycles as cut

        QL_WOUT("Cycle cutting is not yet implemented for pulse visualization.");
        return;
    }

    // Find cuttable ranges...
    cutCycleRangeIndices = findCuttableEmptyRanges(layout);
    // ... and cut them.
    for (const EndPoints &range : cutCycleRangeIndices) {
        for (Int i = range.start; i <= range.end; i++) {
            cycles[i].cut = true;
        }
    }
}

Vec<EndPoints> CircuitData::findCuttableEmptyRanges(const CircuitLayout &layout) const {
    QL_DOUT("Finding cuttable empty cycle ranges...");

    // Calculate the empty cycle ranges.
    Vec<EndPoints> ranges;
    for (UInt i = 0; i < cycles.size(); i++) {
        // If an empty cycle has been found...
        if (cycles[i].empty) {
            const Int start = utoi(i);
            Int end = utoi(cycles.size()) - 1;

            UInt j = i;
            // ... add cycles to the range until a non-empty cycle is found.
            while (j < cycles.size()) {
                if (!cycles[j].empty) {
                    end = utoi(j) - 1;
                    break;
                }
                j++;
            }
            ranges.push_back( {start, end} );

            // Skip over the found range.
            i = j;
        }
    }

    // Check for empty cycle ranges above the threshold.
    Vec<EndPoints> rangesAboveThreshold;
    for (const auto &range : ranges) {
        const Int length = range.end - range.start + 1;
        if (length >= layout.cycles.cutting.getEmptyCycleThreshold()) {
            rangesAboveThreshold.push_back(range);
        }
    }

    return rangesAboveThreshold;
}

Cycle CircuitData::getCycle(const UInt index) const {
    if (index > cycles.size())
        QL_FATAL("Requested cycle index " << index << " is higher than max cycle " << (cycles.size() - 1) << "!");

    return cycles[index];
}

Int CircuitData::getAmountOfCycles() const {
    return utoi(cycles.size());
}

Bool CircuitData::isCycleCut(const Int cycleIndex) const {
    return cycles[cycleIndex].cut;
}

Bool CircuitData::isCycleFirstInCutRange(const Int cycleIndex) const {
    for (const EndPoints &range : cutCycleRangeIndices) {
        if (cycleIndex == range.start) {
            return true;
        }
    }

    return false;
}

void CircuitData::printProperties() const {
    QL_DOUT("[CIRCUIT DATA PROPERTIES]");

    QL_DOUT("amountOfQubits: " << amountOfQubits);
    QL_DOUT("amountOfClassicalBits: " << amountOfClassicalBits);

    QL_DOUT("cycles:");
    for (UInt cycle = 0; cycle < cycles.size(); cycle++) {
        QL_DOUT("\tcycle: " << cycle << " empty: " << cycles[cycle].empty << " cut: " << cycles[cycle].cut);
    }

    QL_DOUT("cutCycleRangeIndices");
    for (const auto &range : cutCycleRangeIndices)
    {
        QL_DOUT("\tstart: " << range.start << " end: " << range.end);
    }
}

// ======================================================= //
// =                      Structure                      = //
// ======================================================= //

Structure::Structure(const CircuitLayout &layout, const CircuitData &circuitData, const Vec<Int> minCycleWidths, const Int extendedImageHeight) :
    layout(layout),
    cellDimensions({layout.grid.getCellSize(), calculateCellHeight(layout)}),
    cycleLabelsY(layout.grid.getBorderSize()),
    bitLabelsX(layout.grid.getBorderSize()),
    minCycleWidths(minCycleWidths)
{
    generateCellPositions(circuitData);
    generateBitLineSegments(circuitData);

    imageWidth = calculateImageWidth(circuitData);
    imageHeight = calculateImageHeight(circuitData, extendedImageHeight);
}

Int Structure::calculateCellHeight(const CircuitLayout &layout) const {
    QL_DOUT("Calculating cell height...");

    if (layout.pulses.areEnabled()) {
        return layout.pulses.getPulseRowHeightMicrowave() 
               + layout.pulses.getPulseRowHeightFlux()
               + layout.pulses.getPulseRowHeightReadout();
    } else {
        return layout.grid.getCellSize();
    }
}

Int Structure::calculateImageWidth(const CircuitData &circuitData) const {
    QL_DOUT("Calculating image width...");

    const Int amountOfCells = utoi(qbitCellPositions.size());
    const Int left = amountOfCells > 0 ? getCellPosition(0, 0, QUANTUM).x0 : 0;
    const Int right = amountOfCells > 0 ? getCellPosition(amountOfCells - 1, 0, QUANTUM).x1 : 0;
    const Int imageWidthFromCells = right - left;

    return layout.bitLines.labels.getColumnWidth() + imageWidthFromCells + layout.grid.getBorderSize() * 2;
}

Int Structure::calculateImageHeight(const CircuitData &circuitData, const Int extendedImageHeight) const {
    QL_DOUT("Calculating image height...");
    
    const Int rowsFromQuantum = circuitData.amountOfQubits;
    // Here be nested ternary operators.
    const Int rowsFromClassical = 
        layout.bitLines.classical.isEnabled()
            ? (layout.bitLines.classical.isGrouped() ? (circuitData.amountOfClassicalBits > 0 ? 1 : 0) : circuitData.amountOfClassicalBits)
            : 0;
    const Int heightFromOperands = 
        (rowsFromQuantum + rowsFromClassical) *
        (cellDimensions.height + (layout.bitLines.edges.areEnabled() ? layout.bitLines.edges.getThickness() : 0));

    return layout.cycles.labels.getRowHeight() + heightFromOperands + layout.grid.getBorderSize() * 2 + extendedImageHeight;
}

void Structure::generateCellPositions(const CircuitData &circuitData) {
    QL_DOUT("Generating cell positions...");

    // Calculate cell positions.
    Int widthFromCycles = 0;
    for (Int column = 0; column < circuitData.getAmountOfCycles(); column++) {
        const Int amountOfChunks = utoi(circuitData.getCycle(column).gates.size());
        const Int cycleWidth = max(minCycleWidths[column],
            (circuitData.isCycleCut(column) ? layout.cycles.cutting.getCutCycleWidth() : (cellDimensions.width * amountOfChunks)));

        const Int x0 = layout.grid.getBorderSize() + layout.bitLines.labels.getColumnWidth() + widthFromCycles;
        const Int x1 = x0 + cycleWidth;

        // Quantum cell positions.
        Vec<Position4> qColumnCells;
        for (Int row = 0; row < circuitData.amountOfQubits; row++) {
            const Int y0 = layout.grid.getBorderSize() + layout.cycles.labels.getRowHeight() +
                row * (cellDimensions.height + (layout.bitLines.edges.areEnabled() ? layout.bitLines.edges.getThickness() : 0));
            const Int y1 = y0 + cellDimensions.height;
            qColumnCells.push_back({x0, y0, x1, y1});
        }
        qbitCellPositions.push_back(qColumnCells);
        // Classical cell positions.
        Vec<Position4> cColumnCells;
        if (circuitData.amountOfClassicalBits > 0) {
            for (Int row = 0; row < circuitData.amountOfClassicalBits; row++) {
                const Int y0 = layout.grid.getBorderSize() + layout.cycles.labels.getRowHeight() + 
                    ((layout.bitLines.classical.isGrouped() ? 0 : row) + circuitData.amountOfQubits) *
                    (cellDimensions.height + (layout.bitLines.edges.areEnabled() ? layout.bitLines.edges.getThickness() : 0));
                const Int y1 = y0 + cellDimensions.height;
                cColumnCells.push_back( {x0, y0, x1, y1} );
            }
        } else {
            const Int y0 = layout.grid.getBorderSize() + layout.cycles.labels.getRowHeight() + 
                circuitData.amountOfQubits *
                (cellDimensions.height + (layout.bitLines.edges.areEnabled() ? layout.bitLines.edges.getThickness() : 0));
            const Int y1 = y0;
            cColumnCells.push_back( {x0, y0, x1, y1} );
        }
        cbitCellPositions.push_back(cColumnCells);

        // Add the appropriate amount of width to the total width.
        if (layout.cycles.cutting.isEnabled()) {
            if (circuitData.isCycleCut(column)) {
                if (column != circuitData.getAmountOfCycles() - 1 && !circuitData.isCycleCut(column + 1)) {
                    widthFromCycles += (Int) (cellDimensions.width * layout.cycles.cutting.getCutCycleWidthModifier());
                }
            } else {
                widthFromCycles += cycleWidth;
            }
        } else {
            widthFromCycles += cycleWidth;
        }
    }
}

void Structure::generateBitLineSegments(const CircuitData &circuitData) {
    QL_DOUT("Generating bit line segments...");

    // Calculate the bit line segments.
    for (Int i = 0; i < circuitData.getAmountOfCycles(); i++) {
        const Bool cut = circuitData.isCycleCut(i);
        Bool reachedEnd = false;

        // Add more cycles to the segment until we reach a cycle that is cut if
        // the current segment is not cut, or vice versa.
        for (Int j = i; j < circuitData.getAmountOfCycles(); j++) {
            if (circuitData.isCycleCut(j) != cut) {
                const Int start = getCellPosition(i, 0, QUANTUM).x0;
                const Int end = getCellPosition(j, 0, QUANTUM).x0;
                bitLineSegments.push_back({{start, end}, cut});
                i = j - 1;
                break;
            }

            // Check if the last cycle has been reached, and exit the
            // calculation if so.
            if (j == circuitData.getAmountOfCycles() - 1) {
                const Int start = getCellPosition(i, 0, QUANTUM).x0;
                const Int end = getCellPosition(j, 0, QUANTUM).x1;
                bitLineSegments.push_back({{start, end}, cut});
                reachedEnd = true;
            }
        }
        
        if (reachedEnd) break;
    }
}

Int Structure::getImageWidth() const {
    return imageWidth;
}

Int Structure::getImageHeight() const {
    return imageHeight;
}

Int Structure::getCycleLabelsY() const {
    return cycleLabelsY;
}

Int Structure::getBitLabelsX() const {
    return bitLabelsX;
}

Int Structure::getCircuitTopY() const {
    return cycleLabelsY;
}

Int Structure::getCircuitBotY() const {
    const Vec<Position4> firstColumnPositions = (layout.pulses.areEnabled() || !layout.bitLines.classical.isEnabled()) ? qbitCellPositions[0] : cbitCellPositions[0];
    Position4 botPosition = firstColumnPositions[firstColumnPositions.size() - 1];
    return botPosition.y1;
}

Dimensions Structure::getCellDimensions() const {
    return cellDimensions;
}

Position4 Structure::getCellPosition(const UInt column, const UInt row, const BitType bitType) const {
    switch (bitType) {
        case CLASSICAL:
            if (layout.pulses.areEnabled())
                QL_FATAL("Cannot get classical cell position when pulse visualization is enabled!");
            if (column >= cbitCellPositions.size())
                QL_FATAL("cycle " << column << " is larger than max cycle " << cbitCellPositions.size() - 1 << " of structure!");
            if (row >= cbitCellPositions[column].size())
                QL_FATAL("classical operand " << row << " is larger than max operand " << cbitCellPositions[column].size() - 1 << " of structure!");
            return cbitCellPositions[column][row];    

        case QUANTUM:
            if (column >= qbitCellPositions.size())
                QL_FATAL("cycle " << column << " is larger than max cycle " << qbitCellPositions.size() - 1 << " of structure!");
            if (row >= qbitCellPositions[column].size())
                QL_FATAL("quantum operand " << row << " is larger than max operand " << qbitCellPositions[column].size() - 1 << " of structure!");
            return qbitCellPositions[column][row];

        default:
            QL_FATAL("Unknown bit type!");
    }
}

Vec<Pair<EndPoints, Bool>> Structure::getBitLineSegments() const {
    return bitLineSegments;
}

void Structure::printProperties() const {
    QL_DOUT("[STRUCTURE PROPERTIES]");

    QL_DOUT("imageWidth: " << imageWidth);
    QL_DOUT("imageHeight: " << imageHeight);

    QL_DOUT("cycleLabelsY: " << cycleLabelsY);
    QL_DOUT("bitLabelsX: " << bitLabelsX);

    QL_DOUT("qbitCellPositions:");
    for (UInt cycle = 0; cycle < qbitCellPositions.size(); cycle++) {
        for (UInt operand = 0; operand < qbitCellPositions[cycle].size(); operand++) {
            QL_DOUT("\tcell: [" << cycle << "," << operand << "]"
                << " x0: " << qbitCellPositions[cycle][operand].x0
                << " x1: " << qbitCellPositions[cycle][operand].x1
                << " y0: " << qbitCellPositions[cycle][operand].y0
                << " y1: " << qbitCellPositions[cycle][operand].y1);
        }
    }

    QL_DOUT("cbitCellPositions:");
    for (UInt cycle = 0; cycle < cbitCellPositions.size(); cycle++) {
        for (UInt operand = 0; operand < cbitCellPositions[cycle].size(); operand++) {
            QL_DOUT("\tcell: [" << cycle << "," << operand << "]"
                << " x0: " << cbitCellPositions[cycle][operand].x0
                << " x1: " << cbitCellPositions[cycle][operand].x1
                << " y0: " << cbitCellPositions[cycle][operand].y0
                << " y1: " << cbitCellPositions[cycle][operand].y1);
        }
    }

    QL_DOUT("bitLineSegments:");
    for (const auto &segment : bitLineSegments) {
        QL_DOUT("\tcut: " << segment.second << " start: " << segment.first.start << " end: " << segment.first.end);
    }
}

void visualizeCircuit(const ir::Ref &ir, const VisualizerConfiguration &configuration) {
    const Vec<GateProperties> gates = parseGates(ir);
    const Int amountOfCycles = calculateAmountOfCycles(gates);
    const Vec<Int> minCycleWidths(amountOfCycles, 0);

    // Generate the image.
    ImageOutput imageOutput = generateImage(ir, configuration, minCycleWidths, 0);

    // Save the image if enabled.
    if (imageOutput.circuitLayout.saveImage || !configuration.interactive) {
        imageOutput.image.save(configuration.output_prefix + ".bmp");
    }

    // Display the image if enabled.
    if (configuration.interactive) {
        QL_DOUT("Displaying image...");
        imageOutput.image.display("Quantum Circuit (" + configuration.pass_name + ")");
    }
}

ImageOutput generateImage(const ir::Ref &ir, const VisualizerConfiguration &configuration, const Vec<Int> &minCycleWidths, const utils::Int extendedImageHeight) {
    // Get the gate list from the program.
    QL_DOUT("Getting gate list...");
    Vec<GateProperties> gates = parseGates(ir);
    if (gates.size() == 0) {
        QL_FATAL("Quantum program contains no gates!");
    }

    // Parse and validate the layout and instruction configuration file.
    CircuitLayout layout = parseCircuitConfiguration(gates, configuration.visualizerConfigPath, *ir->platform);
    validateCircuitLayout(layout, configuration.visualizationType);

    // Calculate circuit properties.
    QL_DOUT("Calculating circuit properties...");
    // Fix measurement gates without classical operands.
    fixMeasurementOperands(gates);

    // Initialize the circuit properties.
    CircuitData circuitData(gates, layout);
    circuitData.printProperties();

    // Initialize the structure of the visualization.
    QL_DOUT("Initializing visualization structure...");
    Structure structure(layout, circuitData, minCycleWidths, extendedImageHeight);
    structure.printProperties();

    // Initialize image.
    QL_DOUT("Initializing image...");
    Image image(structure.getImageWidth(), structure.getImageHeight());
    image.fill(layout.backgroundColor);

    // Draw the cycle labels if the option has been set.
    if (layout.cycles.labels.areEnabled()) {
        drawCycleLabels(image, layout, circuitData, structure);
    }

    // Draw the cycle edges if the option has been set.
    if (layout.cycles.edges.areEnabled()) {
        drawCycleEdges(image, layout, circuitData, structure);
    }

    // Draw the bit line edges if enabled.
    if (layout.bitLines.edges.areEnabled()) {
        drawBitLineEdges(image, layout, circuitData, structure);
    }
    
    // Draw the bit line labels if enabled.
    if (layout.bitLines.labels.areEnabled()) {
        drawBitLineLabels(image, layout, circuitData, structure);
    }

    // Draw the circuit as pulses if enabled.
    if (layout.pulses.areEnabled()) {
        PulseVisualization pulseVisualization = parseWaveformMapping(configuration.waveformMappingPath);
        const Vec<QubitLines> linesPerQubit = generateQubitLines(gates, pulseVisualization, circuitData);

        // Draw the lines of each qubit.
        QL_DOUT("Drawing qubit lines for pulse visualization...");
        for (Int qubitIndex = 0; qubitIndex < circuitData.amountOfQubits; qubitIndex++) {
            const Int yBase = structure.getCellPosition(0, qubitIndex, QUANTUM).y0;

            drawLine(image, structure, linesPerQubit[qubitIndex].microwave, qubitIndex,
                yBase,
                layout.pulses.getPulseRowHeightMicrowave(),
                layout.pulses.getPulseColorMicrowave());

            drawLine(image, structure, linesPerQubit[qubitIndex].flux, qubitIndex,
                yBase + layout.pulses.getPulseRowHeightMicrowave(),
                layout.pulses.getPulseRowHeightFlux(),
                layout.pulses.getPulseColorFlux());

            drawLine(image, structure, linesPerQubit[qubitIndex].readout, qubitIndex,
                yBase + layout.pulses.getPulseRowHeightMicrowave() + layout.pulses.getPulseRowHeightFlux(),
                layout.pulses.getPulseRowHeightReadout(),
                layout.pulses.getPulseColorReadout());
        }

        // // Visualize the gates as pulses on a microwave, flux and readout line.
        // if (layout.pulses.displayGatesAsPulses)
        // {
        //     // Only draw wiggles if the cycle is cut.
        //     if (circuitData.isCycleCut(cycle.index))
        //     {
        //         for (Int qubitIndex = 0; qubitIndex < circuitData.amountOfQubits; qubitIndex++)
        //         {
        //             const Position4 cellPosition = structure.getCellPosition(cycle.index, qubitIndex, QUANTUM);
                    
        //             // Draw wiggle on microwave line.
        //             drawWiggle(image,
        //                        cellPosition.x0,
        //                        cellPosition.x1,
        //                        cellPosition.y0 + layout.pulses.pulseRowHeightMicrowave / 2,
        //                        cellPosition.x1 - cellPosition.x0,
        //                        layout.pulses.pulseRowHeightMicrowave / 8,
        //                        layout.pulses.pulseColorMicrowave);
                    
        //             // Draw wiggle on flux line.
        //             drawWiggle(image,
        //                        cellPosition.x0,
        //                        cellPosition.x1,
        //                        cellPosition.y0 + layout.pulses.pulseRowHeightMicrowave + layout.pulses.pulseRowHeightFlux / 2,
        //                        cellPosition.x1 - cellPosition.x0,
        //                        layout.pulses.pulseRowHeightFlux / 8,
        //                        layout.pulses.pulseColorFlux);
                    
        //             // Draw wiggle on readout line.
        //             drawWiggle(image,
        //                        cellPosition.x0,
        //                        cellPosition.x1,
        //                        cellPosition.y0 + layout.pulses.pulseRowHeightMicrowave + layout.pulses.pulseRowHeightFlux + layout.pulses.pulseRowHeightReadout / 2,
        //                        cellPosition.x1 - cellPosition.x0,
        //                        layout.pulses.pulseRowHeightReadout / 8,
        //                        layout.pulses.pulseColorReadout);
        //         }
                
        //         return;
        //     }
    } else {
        // Pulse visualization is not enabled, so we draw the circuit as an abstract entity.

        // Draw the quantum bit lines.
        QL_DOUT("Drawing qubit lines...");
        for (Int i = 0; i < circuitData.amountOfQubits; i++) {
            drawBitLine(image, layout, QUANTUM, i, circuitData, structure);
        }
            
        // Draw the classical lines if enabled.
        if (layout.bitLines.classical.isEnabled()) {
            // Draw the grouped classical bit lines if the option is set.
            if (circuitData.amountOfClassicalBits > 0 && layout.bitLines.classical.isGrouped()) {
                drawGroupedClassicalBitLine(image, layout, circuitData, structure);
            } else {
                // Otherwise draw each classical bit line seperate.
                QL_DOUT("Drawing ungrouped classical bit lines...");
                for (Int i = 0; i < circuitData.amountOfClassicalBits; i++) {
                    drawBitLine(image, layout, CLASSICAL, i, circuitData, structure);
                }
            }
        }

        // Draw the cycles.
        QL_DOUT("Drawing cycles...");
        for (Int i = 0; i < circuitData.getAmountOfCycles(); i++) {
            // Only draw a cut cycle if its the first in its cut range.
            if (circuitData.isCycleCut(i)) {
                if (i > 0 && !circuitData.isCycleCut(i - 1)) {
                    drawCycle(image, layout, circuitData, structure, circuitData.getCycle(i));
                }
            } else {
                // If the cycle is not cut, just draw it.
                drawCycle(image, layout, circuitData, structure, circuitData.getCycle(i));
            }
        }
    }

    return {image, layout, circuitData, structure};
}

CircuitLayout parseCircuitConfiguration(Vec<GateProperties> &gates,
                                        const Str &visualizerConfigPath,
                                        const ir::Platform &platform) {
    QL_DOUT("Parsing visualizer configuration file for circuit visualization...");

    // Load the relevant instruction parameters.
    static const std::regex comma_space_pattern("\\s*,\\s*");
    static const std::regex trim_pattern("^(\\s)+|(\\s)+$");
    static const std::regex multiple_space_pattern("(\\s)+");

    // Load the visualizer configuration file.
    Json visualizerConfig;
    try {
        visualizerConfig = load_json(visualizerConfigPath);
    } catch (Json::exception &e) {
        QL_FATAL("Failed to load the visualization config file: \n\t" << Str(e.what()));
    }

    // Load the circuit visualization parameters.
    Json circuitConfig;
    if (visualizerConfig.count("circuit") == 1) {
        circuitConfig = visualizerConfig["circuit"];
    } else {
        QL_WOUT("Could not find circuit configuration in visualizer configuration file. Is it named correctly?");
    }

    // Fill the layout object with the values from the config file. Any missing values will assume the default values hardcoded in the layout object.
    CircuitLayout layout;

    struct VisualParameters {
        Str visual_type;
        Vec<Int> codewords;
    };

    Map<Str, VisualParameters> parameterMapping;
    for (const auto& instruction: platform.instructions) {
        Str gateName = instruction->name;

        auto isGateUsed = std::find_if(gates.begin(), gates.end(), [gateName](const GateProperties& g) { return g.name == gateName; }) != gates.end();
        if (!isGateUsed) {
            continue;
        }

        gateName = utils::to_lower(gateName);
        gateName = std::regex_replace(gateName, trim_pattern, "");
        gateName = std::regex_replace(gateName, multiple_space_pattern, " ");
        gateName = std::regex_replace(gateName, comma_space_pattern, ",");

        // Load the visual type of the instruction if provided.
        Str visual_type;
        if ((*instruction->data).count("visual_type") == 1) {
            visual_type = (*instruction->data)["visual_type"].get<Str>();
            QL_DOUT("visual_type: '" << visual_type);
        } else {
            QL_WOUT("Did not find 'visual_type' attribute for instruction: '" << gateName << "'!");
        }

        Vec<Int> codewords;
        // Load the codewords of the instruction if provided.
        if ((*instruction->data).count("visual_codeword") == 1) {
            codewords.push_back((*instruction->data)["visual_codeword"]);
            QL_DOUT("codewords: " << codewords[0]);
        } else {
            if ((*instruction->data).count("visual_right_codeword") == 1 && (*instruction->data).count("visual_left_codeword") == 1) {
                codewords.push_back((*instruction->data)["visual_right_codeword"]);
                codewords.push_back((*instruction->data)["visual_left_codeword"]);
                QL_DOUT("codewords: " << codewords[0] << "," << codewords[1]);
            } else {
                if (circuitConfig.count("pulses") == 1) {
                    if (circuitConfig["pulses"].count("displayGatesAsPulses") == 1) {
                        if (circuitConfig["pulses"]["displayGatesAsPulses"]) {
                            QL_WOUT("Did not find any codeword attributes for instruction: '" << gateName << "'!");
                        }
                    }
                }
            }
        }

        parameterMapping.set(gateName) = {visual_type, codewords};
    }

    // Match the visualization parameters from the hardware configuration with the existing gates.
    for (GateProperties &gate : gates) {
        bool found = false;
        for (const auto &mapping : parameterMapping) {
            if (mapping.first == gate.name) {
                found = true;
                gate.visual_type = mapping.second.visual_type;
                gate.codewords = mapping.second.codewords;
            }
        }
        if (!found) {
            QL_WOUT("Did not find visual type and codewords for gate: " << gate.name << "!");
        }
    }

    // Check if the image should be saved to disk.
    if (visualizerConfig.count("saveImage") == 1) {
        layout.saveImage = visualizerConfig["saveImage"];
    }

    // Load background fill color.
    if (visualizerConfig.count("backgroundColor") == 1) {
        layout.backgroundColor = visualizerConfig["backgroundColor"];
    }

    // -------------------------------------- //
    // -               CYCLES               - //
    // -------------------------------------- //
    if (circuitConfig.count("cycles") == 1) {
        Json cycles = circuitConfig["cycles"];

        // LABELS
        if (cycles.count("labels") == 1) {
            Json labels = cycles["labels"];

            if (labels.count("show") == 1)          layout.cycles.labels.setEnabled(labels["show"]);
            if (labels.count("rowHeight") == 1)     layout.cycles.labels.setRowHeight(labels["rowHeight"]);
            if (labels.count("fontHeight") == 1)    layout.cycles.labels.setFontHeight(labels["fontHeight"]);
            if (labels.count("fontColor") == 1)     layout.cycles.labels.setFontColor(labels["fontColor"]);
        }

        // EDGES
        if (cycles.count("edges") == 1) {
            Json edges = cycles["edges"];

            if (edges.count("show") == 1)   layout.cycles.edges.setEnabled(edges["show"]);
            if (edges.count("color") == 1)  layout.cycles.edges.setColor(edges["color"]);
            if (edges.count("alpha") == 1)  layout.cycles.edges.setAlpha(edges["alpha"]);
        }

        // CUTTING
        if (cycles.count("cutting") == 1) {
            Json cutting = cycles["cutting"];

            if (cutting.count("cut") == 1)                      layout.cycles.cutting.setEnabled(cutting["cut"]);
            if (cutting.count("emptyCycleThreshold") == 1)      layout.cycles.cutting.setEmptyCycleThreshold(cutting["emptyCycleThreshold"]);
            if (cutting.count("cutCycleWidth") == 1)            layout.cycles.cutting.setCutCycleWidth(cutting["cutCycleWidth"]);
            if (cutting.count("cutCycleWidthModifier") == 1)    layout.cycles.cutting.setCutCycleWidthModifier(cutting["cutCycleWidthModifier"]);
        }
        
        if (cycles.count("compress") == 1)                      layout.cycles.setCompressed(cycles["compress"]);
        if (cycles.count("partitionCyclesWithOverlap") == 1)    layout.cycles.setPartitioned(cycles["partitionCyclesWithOverlap"]);
    }

    // -------------------------------------- //
    // -              BIT LINES             - //
    // -------------------------------------- //
    if (circuitConfig.count("bitLines") == 1)
    {
        Json bitLines = circuitConfig["bitLines"];

        // LABELS
        if (bitLines.count("labels") == 1) {
            Json labels = bitLines["labels"];

            if (labels.count("show") == 1)          layout.bitLines.labels.setEnabled(labels["show"]);
            if (labels.count("columnWidth") == 1)   layout.bitLines.labels.setColumnWidth(labels["columnWidth"]);
            if (labels.count("fontHeight") == 1)    layout.bitLines.labels.setFontHeight(labels["fontHeight"]);
            if (labels.count("qbitColor") == 1)     layout.bitLines.labels.setQbitColor(labels["qbitColor"]);
            if (labels.count("cbitColor") == 1)     layout.bitLines.labels.setCbitColor(labels["cbitColor"]);
        }

        // QUANTUM
        if (bitLines.count("quantum") == 1) {
            Json quantum = bitLines["quantum"];

            if (quantum.count("color") == 1) layout.bitLines.quantum.setColor(quantum["color"]);
        }

        // CLASSICAL
        if (bitLines.count("classical") == 1) {
            Json classical = bitLines["classical"];

            if (classical.count("show") == 1)           layout.bitLines.classical.setEnabled(classical["show"]);
            if (classical.count("group") == 1)          layout.bitLines.classical.setGrouped(classical["group"]);
            if (classical.count("groupedLineGap") == 1) layout.bitLines.classical.setGroupedLineGap(classical["groupedLineGap"]);
            if (classical.count("color") == 1)          layout.bitLines.classical.setColor(classical["color"]);
        }

        // EDGES
        if (bitLines.count("edges") == 1) {
            Json edges = bitLines["edges"];

            if (edges.count("show") == 1)       layout.bitLines.edges.setEnabled(edges["show"]);
            if (edges.count("thickness") == 1)  layout.bitLines.edges.setThickness(edges["thickness"]);
            if (edges.count("color") == 1)      layout.bitLines.edges.setColor(edges["color"]);
            if (edges.count("alpha") == 1)      layout.bitLines.edges.setAlpha(edges["alpha"]);
        }
    }

    // -------------------------------------- //
    // -                GRID                - //
    // -------------------------------------- //
    if (circuitConfig.count("grid") == 1) {
        Json grid = circuitConfig["grid"];

        if (grid.count("cellSize") == 1)    layout.grid.setCellSize(grid["cellSize"]);
        if (grid.count("borderSize") == 1)  layout.grid.setBorderSize(grid["borderSize"]);
    }

    // -------------------------------------- //
    // -       GATE DURATION OUTLINES       - //
    // -------------------------------------- //
    if (circuitConfig.count("gateDurationOutlines") == 1) {
        Json gateDurationOutlines = circuitConfig["gateDurationOutlines"];

        if (gateDurationOutlines.count("show") == 1)         layout.gateDurationOutlines.setEnabled(gateDurationOutlines["show"]);
        if (gateDurationOutlines.count("gap") == 1)          layout.gateDurationOutlines.setGap(gateDurationOutlines["gap"]);
        if (gateDurationOutlines.count("fillAlpha") == 1)    layout.gateDurationOutlines.setFillAlpha(gateDurationOutlines["fillAlpha"]);
        if (gateDurationOutlines.count("outlineAlpha") == 1) layout.gateDurationOutlines.setOutlineAlpha(gateDurationOutlines["outlineAlpha"]);
        if (gateDurationOutlines.count("outlineColor") == 1) layout.gateDurationOutlines.setOutlineColor(gateDurationOutlines["outlineColor"]);
    }

    // -------------------------------------- //
    // -            MEASUREMENTS            - //
    // -------------------------------------- //
    if (circuitConfig.count("measurements") == 1) {
        Json measurements = circuitConfig["measurements"];

        if (measurements.count("drawConnection") == 1)  layout.measurements.enableDrawConnection(measurements["drawConnection"]);
        if (measurements.count("lineSpacing") == 1)     layout.measurements.setLineSpacing(measurements["lineSpacing"]);
        if (measurements.count("arrowSize") == 1)       layout.measurements.setArrowSize(measurements["arrowSize"]);
    }

    // -------------------------------------- //
    // -               PULSES               - //
    // -------------------------------------- //
    if (circuitConfig.count("pulses") == 1) {
        Json pulses = circuitConfig["pulses"];

        if (pulses.count("displayGatesAsPulses") == 1)      layout.pulses.setEnabled(pulses["displayGatesAsPulses"]);
        if (pulses.count("pulseRowHeightMicrowave") == 1)   layout.pulses.setPulseRowHeightMicrowave(pulses["pulseRowHeightMicrowave"]);
        if (pulses.count("pulseRowHeightFlux") == 1)        layout.pulses.setPulseRowHeightFlux(pulses["pulseRowHeightFlux"]);
        if (pulses.count("pulseRowHeightReadout") == 1)     layout.pulses.setPulseRowHeightReadout(pulses["pulseRowHeightReadout"]);
        if (pulses.count("pulseColorMicrowave") == 1)       layout.pulses.setPulseColorMicrowave(pulses["pulseColorMicrowave"]);
        if (pulses.count("pulseColorFlux") == 1)            layout.pulses.setPulseColorFlux(pulses["pulseColorFlux"]);
        if (pulses.count("pulseColorReadout") == 1)         layout.pulses.setPulseColorReadout(pulses["pulseColorReadout"]);
    }

    // Load the custom instruction visualization parameters.
    if (circuitConfig.count("instructions") == 1) {
        for (const auto &instruction : circuitConfig["instructions"].items()) {
            try {
                GateVisual gateVisual;
                Json content = instruction.value();

                // Load the connection color.
                Json connectionColor = content["connectionColor"];
                gateVisual.connectionColor[0] = connectionColor[0];
                gateVisual.connectionColor[1] = connectionColor[1];
                gateVisual.connectionColor[2] = connectionColor[2];
                QL_DOUT("Connection color: [" 
                    << (int)gateVisual.connectionColor[0] << ","
                    << (int)gateVisual.connectionColor[1] << ","
                    << (int)gateVisual.connectionColor[2] << "]");

                // Load the individual nodes.
                Json nodes = content["nodes"];
                for (size_t i = 0; i < nodes.size(); i++) {
                    Json node = nodes[i];
                    
                    Color fontColor = {node["fontColor"][0], node["fontColor"][1], node["fontColor"][2]};
                    Color backgroundColor = {node["backgroundColor"][0], node["backgroundColor"][1], node["backgroundColor"][2]};
                    Color outlineColor = {node["outlineColor"][0], node["outlineColor"][1], node["outlineColor"][2]};
                    
                    NodeType nodeType;
                    if (node["type"] == "NONE") {
                        nodeType = NONE;
                    } else if (node["type"] == "GATE") {
                        nodeType = GATE;
                    } else if (node["type"] == "CONTROL") {
                        nodeType = CONTROL;
                    } else if (node["type"] == "NOT") {
                        nodeType = NOT;
                    } else if (node["type"] == "CROSS") {
                        nodeType = CROSS;
                    } else {
                        QL_WOUT("Unknown gate display node type! Defaulting to type NONE...");
                        nodeType = NONE;
                    }
                    
                    Node loadedNode = {
                        nodeType,
                        node["radius"],
                        node["displayName"],
                        node["fontHeight"],
                        fontColor,
                        backgroundColor,
                        outlineColor
                    };
                    
                    gateVisual.nodes.push_back(loadedNode);
                    
                    QL_DOUT("[type: " << node["type"] << "] "
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

                layout.gateVisuals.insert({instruction.key(), gateVisual});
            } catch (Json::exception &e) {
                QL_WOUT("Failed to load visualization parameters for instruction: '" << instruction.key()
                    << "' \n\t" << Str(e.what()));
            }
        }
    } else {
        QL_WOUT("Did not find 'instructions' attribute! The visualizer will try to fall back on default gate visualizations.");
    }

    return layout;
}

void validateCircuitLayout(CircuitLayout &layout, const Str &visualizationType) {
    QL_DOUT("Validating layout...");

    //TODO: add more validation

    // Disable pulse visualization and cycle cutting for the mapping graph visualization.
    if (visualizationType == "MAPPING_GRAPH") {
        layout.cycles.cutting.setEnabled(false);
        layout.pulses.setEnabled(false);
    }

    if (layout.cycles.cutting.getEmptyCycleThreshold() < 1) {
        QL_WOUT("Adjusting 'emptyCycleThreshold' to minimum value of 1. Value in configuration file is set to "
            << layout.cycles.cutting.getEmptyCycleThreshold() << ".");
        layout.cycles.cutting.setEmptyCycleThreshold(1);
    }

    if (layout.pulses.areEnabled()) {
        if (layout.bitLines.classical.isEnabled()) {
            QL_WOUT("Adjusting 'showClassicalLines' to false. Unable to show classical lines when 'displayGatesAsPulses' is true!");
            layout.bitLines.classical.setEnabled(false);
        }
        if (layout.cycles.arePartitioned()) {
            QL_WOUT("Adjusting 'partitionCyclesWithOverlap' to false. It is unnecessary to partition cycles when 'displayGatesAsPulses' is true!");
            layout.cycles.setPartitioned(false);
        }
        if (layout.cycles.areCompressed()) {
            QL_WOUT("Adjusting 'compressCycles' to false. Cannot compress cycles when 'displayGatesAsPulses' is true!");
            layout.cycles.setCompressed(false);
        }
    }

    if (!layout.bitLines.labels.areEnabled())   layout.bitLines.labels.setColumnWidth(0);
    if (!layout.cycles.labels.areEnabled())     layout.cycles.labels.setRowHeight(0);
}

PulseVisualization parseWaveformMapping(const Str &waveformMappingPath) {
    QL_DOUT("Parsing waveform mapping configuration file...");

    // Read the waveform mapping Json file.
    Json waveformMapping;
    try {
        waveformMapping = load_json(waveformMappingPath);
    } catch (Json::exception &e) {
        QL_FATAL("Failed to load the visualization waveform mapping file:\n\t" << Str(e.what()));
    }

    PulseVisualization pulseVisualization;

    // Parse the sample rates.
    if (waveformMapping.count("samplerates") == 1) {
        try {
            if (waveformMapping["samplerates"].count("microwave") == 1)
                pulseVisualization.sampleRateMicrowave = waveformMapping["samplerates"]["microwave"];
            else
                QL_FATAL("Missing 'samplerateMicrowave' attribute in waveform mapping file!");

            if (waveformMapping["samplerates"].count("flux") == 1)
                pulseVisualization.sampleRateFlux = waveformMapping["samplerates"]["flux"];
            else
                QL_FATAL("Missing 'samplerateFlux' attribute in waveform mapping file!");

            if (waveformMapping["samplerates"].count("readout") == 1)
                pulseVisualization.sampleRateReadout = waveformMapping["samplerates"]["readout"];
            else
                QL_FATAL("Missing 'samplerateReadout' attribute in waveform mapping file!");
        } catch (const Exception &e) {
            QL_FATAL("Exception while parsing sample rates from waveform mapping file:\n\t" << e.what()
                 << "\n\tMake sure the sample rates are Integers!" );
        }
    } else {
        QL_FATAL("Missing 'samplerates' attribute in waveform mapping file!");
    }

    // Parse the codeword mapping.
    if (waveformMapping.count("codewords") == 1) {
        // For each codeword...
        for (const auto &codewordMapping : waveformMapping["codewords"].items()) {
            // ... get the index and the qubit pulse mappings it contains.
            Int codewordIndex = 0;
            try {
                codewordIndex = parse_int(codewordMapping.key());
            } catch (const Exception &e) {
                QL_FATAL("Exception while parsing key to codeword mapping " << codewordMapping.key()
                     << " in waveform mapping file:\n\t" << e.what() << "\n\tKey should be an Integer!");
            }
            Map<Int, GatePulses> qubitMapping;

            // For each qubit in the codeword...
            for (const auto &qubitMap : codewordMapping.value().items()) {
                // ... get the index and the pulse mapping.
                Int qubitIndex = 0;
                try {
                    qubitIndex = parse_int(qubitMap.key());
                } catch (const Exception &e) {
                    QL_FATAL("Exception while parsing key to qubit mapping " << qubitMap.key() << " in waveform mapping file:\n\t"
                         << e.what() << "\n\tKey should be an Integer!");
                }
                auto gatePulsesMapping = qubitMap.value();

                // Read the pulses from the pulse mapping.
                Vec<Real> microwave;
                Vec<Real> flux;
                Vec<Real> readout;
                try {
                    if (gatePulsesMapping.contains("microwave")) microwave = gatePulsesMapping["microwave"].get<Vec<Real>>();
                    if (gatePulsesMapping.contains("flux")) flux = gatePulsesMapping["flux"].get<Vec<Real>>();
                    if (gatePulsesMapping.contains("readout")) readout = gatePulsesMapping["readout"].get<Vec<Real>>();
                } catch (const Exception &e) {
                    QL_FATAL("Exception while parsing waveforms from waveform mapping file:\n\t" << e.what()
                         << "\n\tMake sure the waveforms are arrays of Integers!" );
                }
                GatePulses gatePulses {microwave, flux, readout};

                // Insert the pulse mapping Into the qubit.
                qubitMapping.insert({qubitIndex, gatePulses});
            }

            // Insert the mapping for the qubits Into the codeword.
            pulseVisualization.mapping.insert({codewordIndex, qubitMapping});
        }
    } else {
        QL_FATAL("Missing 'codewords' attribute in waveform mapping file!");
    }

    // // PrInt the waveform mapping.
    // for (const Pair<Int, Map<Int, GatePulses>>& codeword : pulseVisualization.mapping)
    // {
    //     IOUT("codeword: " << codeword.first);
    //     for (const Pair<Int, GatePulses>& gatePulsesMapping : codeword.second)
    //     {
    //         const Int qubitIndex = gatePulsesMapping.first;
    //         IOUT("\tqubit: " << qubitIndex);
    //         const GatePulses gatePulses = gatePulsesMapping.second;

    //         Str microwaveString = "[ ";
    //         for (const Int amplitude : gatePulses.microwave)
    //         {
    //             microwaveString += to_string(amplitude) + " ";
    //         }
    //         microwaveString += "]";
    //         IOUT("\t\tmicrowave: " << microwaveString);

    //         Str fluxString = "[ ";
    //         for (const Int amplitude : gatePulses.flux)
    //         {
    //             fluxString += to_string(amplitude) + " ";
    //         }
    //         fluxString += "]";
    //         IOUT("\t\tflux: " << fluxString);

    //         Str readoutString = "[ ";
    //         for (const Int amplitude : gatePulses.readout)
    //         {
    //             readoutString += to_string(amplitude) + " ";
    //         }
    //         readoutString += "]";
    //         IOUT("\t\treadout: " << readoutString);
    //     }
    // }

    return pulseVisualization;
}

Vec<QubitLines> generateQubitLines(const Vec<GateProperties> &gates,
                                   const PulseVisualization &pulseVisualization,
                                   const CircuitData &circuitData) {
    QL_DOUT("Generating qubit lines for pulse visualization...");

    // Find the gates per qubit.
    Vec<Vec<GateProperties>> gatesPerQubit(circuitData.amountOfQubits);
    for (const GateProperties &gate : gates) {
        for (const GateOperand &operand : getGateOperands(gate)) {
            if (operand.bitType == QUANTUM) {
                gatesPerQubit[operand.index].push_back(gate);
            }
        }
    }

    // Calculate the line segments for each qubit.
    Vec<QubitLines> linesPerQubit(circuitData.amountOfQubits);
    for (Int qubitIndex = 0; qubitIndex < circuitData.amountOfQubits; qubitIndex++) {
        // Find the cycles with pulses for each line.
        Line microwaveLine;
        Line fluxLine;
        Line readoutLine;

        for (const GateProperties &gate : gatesPerQubit[qubitIndex]) {
            const EndPoints gateCycles {gate.cycle, gate.cycle + gate.durationInCycles - 1};
            if (!gate.codewords.empty()) {
                const Int codeword = gate.codewords[0];
                try {
                    const GatePulses gatePulses = pulseVisualization.mapping.at(codeword).at(qubitIndex);

                    if (!gatePulses.microwave.empty())
                        microwaveLine.segments.push_back({PULSE, gateCycles, {gatePulses.microwave, pulseVisualization.sampleRateMicrowave}});

                    if (!gatePulses.flux.empty())
                        fluxLine.segments.push_back({PULSE, gateCycles, {gatePulses.flux, pulseVisualization.sampleRateFlux}});

                    if (!gatePulses.readout.empty())
                        readoutLine.segments.push_back({PULSE, gateCycles, {gatePulses.readout, pulseVisualization.sampleRateReadout}});
                } catch (const Exception &e) {
                    QL_WOUT("Missing codeword and/or qubit in waveform mapping file for gate: " << gate.name << "! Replacing pulse with flat line...\n\t" <<
                         "Indices are: codeword = " << codeword << " and qubit = " << qubitIndex);
                }
            }
        }

        microwaveLine.maxAmplitude = calculateMaxAmplitude(microwaveLine.segments);
        fluxLine.maxAmplitude = calculateMaxAmplitude(fluxLine.segments);
        readoutLine.maxAmplitude = calculateMaxAmplitude(readoutLine.segments);

        // Find the empty ranges between the existing segments and insert flat
        // segments there.
        insertFlatLineSegments(microwaveLine.segments, circuitData.getAmountOfCycles());
        insertFlatLineSegments(fluxLine.segments, circuitData.getAmountOfCycles());
        insertFlatLineSegments(readoutLine.segments, circuitData.getAmountOfCycles());

        // Construct the QubitLines object at the specified qubit index.
        linesPerQubit[qubitIndex] = { microwaveLine, fluxLine, readoutLine };

        // QL_DOUT("qubit: " << qubitIndex);
        // Str microwaveOutput = "\tmicrowave segments: ";
        // for (const LineSegment& segment : microwaveLineSegments)
        // {
        //     Str type;
        //     switch (segment.type)
        //     {
        //         case FLAT: type = "FLAT"; break;
        //         case PULSE: type = "PULSE"; break;
        //         case CUT: type = "CUT"; break;
        //     }
        //     microwaveOutput += " [" + type + " (" + to_string(segment.range.start) + "," + to_string(segment.range.end) + ")]";
        // }
        // QL_DOUT(microwaveOutput);

        // Str fluxOutput = "\tflux segments: ";
        // for (const LineSegment& segment : fluxLineSegments)
        // {
        //     Str type;
        //     switch (segment.type)
        //     {
        //         case FLAT: type = "FLAT"; break;
        //         case PULSE: type = "PULSE"; break;
        //         case CUT: type = "CUT"; break;
        //     }
        //     fluxOutput += " [" + type + " (" + to_string(segment.range.start) + "," + to_string(segment.range.end) + ")]";
        // }
        // QL_DOUT(fluxOutput);

        // Str readoutOutput = "\treadout segments: ";
        // for (const LineSegment& segment : readoutLineSegments)
        // {
        //     Str type;
        //     switch (segment.type)
        //     {
        //         case FLAT: type = "FLAT"; break;
        //         case PULSE: type = "PULSE"; break;
        //         case CUT: type = "CUT"; break;
        //     }
        //     readoutOutput += " [" + type + " (" + to_string(segment.range.start) + "," + to_string(segment.range.end) + ")]";
        // }
        // QL_DOUT(readoutOutput);
    }

    return linesPerQubit;
}

Real calculateMaxAmplitude(const Vec<LineSegment> &lineSegments) {
    Real maxAmplitude = 0;

    for (const LineSegment &segment : lineSegments) {
        const Vec<Real> waveform = segment.pulse.waveform;
        Real maxAmplitudeInSegment = 0;
        for (const Real amplitude : waveform) {
            const Real absAmplitude = abs(amplitude);
            if (absAmplitude > maxAmplitudeInSegment)
                maxAmplitudeInSegment = absAmplitude;
        }
        if (maxAmplitudeInSegment > maxAmplitude)
            maxAmplitude = maxAmplitudeInSegment;
    }

    return maxAmplitude;
}

void insertFlatLineSegments(Vec<LineSegment> &existingLineSegments, const Int amountOfCycles) {
    const Int minCycle = 0;
    const Int maxCycle = amountOfCycles - 1;
    for (Int i = minCycle; i <= maxCycle; i++) {
        for (Int j = i; j <= maxCycle; j++) {    
            if (j == maxCycle) {
                existingLineSegments.push_back( { FLAT, {i, j}, {{}, 0} } );
                i = maxCycle + 1;
                break;
            }

            Bool foundEndOfEmptyRange = false;
            for (const LineSegment &segment : existingLineSegments) {
                if (j == segment.range.start) {
                    foundEndOfEmptyRange = true;
                    // If the start of the new search for an empty range is also
                    // the start of a new non-empty range, skip adding a
                    // segment.
                    if (j != i) {
                        existingLineSegments.push_back( { FLAT, {i, j - 1}, {{}, 0} } );
                    }
                    i = segment.range.end;
                    break;
                }
            }

            if (foundEndOfEmptyRange) break;
        }
    }
}

void drawCycleLabels(Image &image,
                     const CircuitLayout &layout,
                     const CircuitData &circuitData,
                     const Structure &structure) {
    QL_DOUT("Drawing cycle labels...");

    for (Int i = 0; i < circuitData.getAmountOfCycles(); i++) {
        Str cycleLabel = "";
        Int cellWidth = 0;
        if (circuitData.isCycleCut(i)) {
            if (!circuitData.isCycleFirstInCutRange(i))
                continue;
            cellWidth = layout.cycles.cutting.getCutCycleWidth();
            cycleLabel = "...";
        } else {
            // cellWidth = structure.getCellDimensions().width;
            const Position4 cellPosition = structure.getCellPosition(i, 0, QUANTUM);
            cellWidth = cellPosition.x1 - cellPosition.x0;
            cycleLabel = to_string(i);
        }

        Dimensions textDimensions = calculateTextDimensions(cycleLabel, layout.cycles.labels.getFontHeight());

        const Int xGap = (cellWidth - textDimensions.width) / 2;
        const Int yGap = (layout.cycles.labels.getRowHeight() - textDimensions.height) / 2;
        const Int xCycle = structure.getCellPosition(i, 0, QUANTUM).x0 + xGap;
        const Int yCycle = structure.getCycleLabelsY() + yGap;

        image.drawText(xCycle, yCycle, cycleLabel, layout.cycles.labels.getFontHeight(), layout.cycles.labels.getFontColor());
    }
}

void drawCycleEdges(Image &image,
                    const CircuitLayout &layout,
                    const CircuitData &circuitData,
                    const Structure &structure) {
    QL_DOUT("Drawing cycle edges...");

    for (Int i = 0; i < circuitData.getAmountOfCycles(); i++) {
        if (i == 0) continue;
        if (circuitData.isCycleCut(i) && circuitData.isCycleCut(i - 1)) continue;

        const Int xCycle = structure.getCellPosition(i, 0, QUANTUM).x0;
        const Int y0 = structure.getCircuitTopY();
        const Int y1 = structure.getCircuitBotY();

        QL_DOUT("drawing edge at x = " << xCycle << ", from y0 = " << y0 << " and y1 = " << y1);

        image.drawLine(xCycle, y0, xCycle, y1, layout.cycles.edges.getColor(), layout.cycles.edges.getAlpha(), LinePattern::DASHED);
    }
}

void drawBitLineLabels(Image &image,
                       const CircuitLayout &layout,
                       const CircuitData &circuitData,
                       const Structure &structure)
{
    QL_DOUT("Drawing bit line labels...");

    for (Int bitIndex = 0; bitIndex < circuitData.amountOfQubits; bitIndex++) {
        const Str label = "q" + to_string(bitIndex);
        const Dimensions textDimensions = calculateTextDimensions(label, layout.bitLines.labels.getFontHeight());

        const Int xGap = (structure.getCellDimensions().width - textDimensions.width) / 2;
        const Int yGap = (structure.getCellDimensions().height - textDimensions.height) / 2;
        const Int xLabel = structure.getBitLabelsX() + xGap;
        const Int yLabel = structure.getCellPosition(0, bitIndex, QUANTUM).y0 + yGap;

        image.drawText(xLabel, yLabel, label, layout.bitLines.labels.getFontHeight(), layout.bitLines.labels.getQbitColor());
    }

    if (layout.bitLines.classical.isEnabled()) {
        if (layout.bitLines.classical.isGrouped()) {
            const Str label = "C";
            const Dimensions textDimensions = calculateTextDimensions(label, layout.bitLines.labels.getFontHeight());

            const Int xGap = (structure.getCellDimensions().width - textDimensions.width) / 2;
            const Int yGap = (structure.getCellDimensions().height - textDimensions.height) / 2;
            const Int xLabel = structure.getBitLabelsX() + xGap;
            const Int yLabel = structure.getCellPosition(0, 0, CLASSICAL).y0 + yGap;

            image.drawText(xLabel, yLabel, label, layout.bitLines.labels.getFontHeight(), layout.bitLines.labels.getCbitColor());
        } else {
            for (Int bitIndex = 0; bitIndex < circuitData.amountOfClassicalBits; bitIndex++) {
                const Str label = "c" + to_string(bitIndex);
                const Dimensions textDimensions = calculateTextDimensions(label, layout.bitLines.labels.getFontHeight());

                const Int xGap = (structure.getCellDimensions().width - textDimensions.width) / 2;
                const Int yGap = (structure.getCellDimensions().height - textDimensions.height) / 2;
                const Int xLabel = structure.getBitLabelsX() + xGap;
                const Int yLabel = structure.getCellPosition(0, bitIndex, CLASSICAL).y0 + yGap;

                image.drawText(xLabel, yLabel, label, layout.bitLines.labels.getFontHeight(), layout.bitLines.labels.getCbitColor());
            }
        }
    }
}

void drawBitLineEdges(Image &image,
                      const CircuitLayout &layout,
                      const CircuitData &circuitData,
                      const Structure &structure)
{
    QL_DOUT("Drawing bit line edges...");

    const Int x0 = structure.getCellPosition(0, 0, QUANTUM).x0 - layout.grid.getBorderSize() / 2;
    const Int x1 = structure.getCellPosition(circuitData.getAmountOfCycles() - 1, 0, QUANTUM).x1 + layout.grid.getBorderSize() / 2;
    const Int yOffsetStart = -1 * layout.bitLines.edges.getThickness();

    for (Int bitIndex = 0; bitIndex < circuitData.amountOfQubits; bitIndex++) {
        if (bitIndex == 0) continue;

        const Int y = structure.getCellPosition(0, bitIndex, QUANTUM).y0;
        for (Int yOffset = yOffsetStart; yOffset < yOffsetStart + layout.bitLines.edges.getThickness(); yOffset++) {
            image.drawLine(x0, y + yOffset, x1, y + yOffset, layout.bitLines.edges.getColor(), layout.bitLines.edges.getAlpha());
        }
    }

    if (layout.bitLines.classical.isEnabled()) {
        if (layout.bitLines.classical.isGrouped()) {
            const Int y = structure.getCellPosition(0, 0, CLASSICAL).y0;
            for (Int yOffset = yOffsetStart; yOffset < yOffsetStart + layout.bitLines.edges.getThickness(); yOffset++) {
                image.drawLine(x0, y + yOffset, x1, y + yOffset, layout.bitLines.edges.getColor(), layout.bitLines.edges.getAlpha());
            }
        } else {
            for (Int bitIndex = 0; bitIndex < circuitData.amountOfClassicalBits; bitIndex++) {
                if (bitIndex == 0) continue;

                const Int y = structure.getCellPosition(0, bitIndex, CLASSICAL).y0;
                for (Int yOffset = yOffsetStart; yOffset < yOffsetStart + layout.bitLines.edges.getThickness(); yOffset++) {
                    image.drawLine(x0, y + yOffset, x1, y + yOffset, layout.bitLines.edges.getColor(), layout.bitLines.edges.getAlpha());
                }
            }
        }
    }
}

void drawBitLine(Image &image,
                 const CircuitLayout &layout,
                 const BitType bitType,
                 const Int row,
                 const CircuitData &circuitData,
                 const Structure &structure) {
    Color bitLineColor;
    Color bitLabelColor;
    switch (bitType) {
        case CLASSICAL:
            bitLineColor = layout.bitLines.classical.getColor();
            bitLabelColor = layout.bitLines.labels.getCbitColor();
            break;
        case QUANTUM:
            bitLineColor = layout.bitLines.quantum.getColor();
            bitLabelColor = layout.bitLines.labels.getQbitColor();
            break;
    }

    for (const Pair<EndPoints, Bool> &segment : structure.getBitLineSegments()) {
        const Int y = structure.getCellPosition(0, row, bitType).y0 + structure.getCellDimensions().height / 2;
        // Check if the segment is a cut segment.
        if (segment.second) {
            const Int height = structure.getCellDimensions().height / 8;
            const Int width = segment.first.end - segment.first.start;

            drawWiggle(image, segment.first.start, segment.first.end, y, width, height, bitLineColor);
        } else {
            image.drawLine(segment.first.start, y, segment.first.end, y, bitLineColor);
        }
    }
}

void drawGroupedClassicalBitLine(Image &image,
                                 const CircuitLayout &layout,
                                 const CircuitData &circuitData,
                                 const Structure &structure) {
    QL_DOUT("Drawing grouped classical bit lines...");

    const Int y = structure.getCellPosition(0, 0, CLASSICAL).y0 + structure.getCellDimensions().height / 2;

    // Draw the segments of the Real line.
    for (const Pair<EndPoints, Bool> &segment : structure.getBitLineSegments()) {
        // Check if the segment is a cut segment.
        if (segment.second) {
            const Int height = structure.getCellDimensions().height / 8;
            const Int width = segment.first.end - segment.first.start;
            
            drawWiggle(image, segment.first.start, segment.first.end, y - layout.bitLines.classical.getGroupedLineGap(),
                width, height, layout.bitLines.classical.getColor());           
            drawWiggle(image, segment.first.start, segment.first.end, y + layout.bitLines.classical.getGroupedLineGap(),
                width, height, layout.bitLines.classical.getColor());
        } else {
            image.drawLine(segment.first.start, y - layout.bitLines.classical.getGroupedLineGap(),
                segment.first.end, y - layout.bitLines.classical.getGroupedLineGap(), layout.bitLines.classical.getColor());
            image.drawLine(segment.first.start, y + layout.bitLines.classical.getGroupedLineGap(),
                segment.first.end, y + layout.bitLines.classical.getGroupedLineGap(), layout.bitLines.classical.getColor());
        }
    }

    // Draw the dashed line plus classical bit amount number on the first
    // segment.
    Pair<EndPoints, Bool> firstSegment = structure.getBitLineSegments()[0];
    //TODO: store the dashed line parameters in the layout object
    image.drawLine(firstSegment.first.start + 8, y + layout.bitLines.classical.getGroupedLineGap() + 2,
        firstSegment.first.start + 12, y - layout.bitLines.classical.getGroupedLineGap() - 3, layout.bitLines.classical.getColor());
    const Str label = to_string(circuitData.amountOfClassicalBits);
    //TODO: fix these hardcoded parameters
    const Int xLabel = firstSegment.first.start + 8;
    const Int yLabel = y - layout.bitLines.classical.getGroupedLineGap() - 3 - 13;
    image.drawText(xLabel, yLabel, label, layout.bitLines.labels.getFontHeight(), layout.bitLines.labels.getCbitColor());
}

void drawWiggle(Image &image,
                const Int x0,
                const Int x1,
                const Int y,
                const Int width,
                const Int height,
                const Color color) {
    image.drawLine(x0,                  y,          x0 + width / 3,     y - height, color);
    image.drawLine(x0 + width / 3,      y - height, x0 + width / 3 * 2, y + height, color);
    image.drawLine(x0 + width / 3 * 2,  y + height, x1,                 y,          color);
}

void drawLine(Image &image,
              const Structure &structure,
              const Line &line,
              const Int qubitIndex,
              const Int y,
              const Int maxLineHeight,
              const Color color) {
    for (const LineSegment &segment : line.segments) {
        const Int x0 = structure.getCellPosition(segment.range.start, qubitIndex, QUANTUM).x0;
        const Int x1 = structure.getCellPosition(segment.range.end, qubitIndex, QUANTUM).x1;
        const Int yMiddle = y + maxLineHeight / 2;

        switch (segment.type) {
            case FLAT: {
                image.drawLine(x0, yMiddle, x1, yMiddle, color);
            }
            break;

            case PULSE: {
                // Calculate pulse properties.
                QL_DOUT(" --- PULSE SEGMENT --- ");

                const Real maxAmplitude = line.maxAmplitude;

                const Int segmentWidth = x1 - x0; // pixels
                const Int segmentLengthInCycles = segment.range.end - segment.range.start + 1; // cycles
                static constexpr Int CYCLE_DURATION = 40;
                const Int segmentLengthInNanoSeconds = CYCLE_DURATION * segmentLengthInCycles; // nanoseconds
                QL_DOUT("\tsegment width: " << segmentWidth);
                QL_DOUT("\tsegment length in cycles: " << segmentLengthInCycles);
                QL_DOUT("\tsegment length in nanoseconds: " << segmentLengthInNanoSeconds);

                const Int amountOfSamples = utoi(segment.pulse.waveform.size());
                const Int sampleRate = segment.pulse.sampleRate; // MHz
                const Real samplePeriod = 1000.0f * (1.0f / (Real) sampleRate); // nanoseconds
                const Int samplePeriodWidth = (Int) floor(samplePeriod / (Real) segmentLengthInNanoSeconds * (Real) segmentWidth); // pixels
                const Int waveformWidthInPixels = samplePeriodWidth * amountOfSamples;
                QL_DOUT("\tamount of samples: " << amountOfSamples);
                QL_DOUT("\tsample period in nanoseconds: " << samplePeriod);
                QL_DOUT("\tsample period width in segment: " << samplePeriodWidth);
                QL_DOUT("\ttotal waveform width in pixels: " << waveformWidthInPixels);

                if (waveformWidthInPixels > segmentWidth) {
                    QL_WOUT("The waveform duration in cycles " << segment.range.start << " to " << segment.range.end << " on qubit " << qubitIndex <<
                         " seems to be larger than the duration of those cycles. Please check the sample rate and amount of samples.");
                }

                // Calculate sample positions.
                const Real amplitudeUnitHeight = (Real) maxLineHeight / (maxAmplitude * 2.0f);
                Vec<Position2> samplePositions;
                for (UInt i = 0; i < segment.pulse.waveform.size(); i++) {
                    const Int xSample = x0 + utoi(i) * samplePeriodWidth;

                    const Real amplitude = segment.pulse.waveform[i];
                    const Real adjustedAmplitude = amplitude + maxAmplitude;
                    const Int ySample = max(y, y + maxLineHeight - 1 - (Int) floor(adjustedAmplitude * amplitudeUnitHeight));

                    samplePositions.push_back( {xSample, ySample} );
                }

                // Draw the lines connecting the samples.
                for (UInt i = 0; i < samplePositions.size() - 1; i++) {
                    const Position2 currentSample = samplePositions[i];
                    const Position2 nextSample = samplePositions[i + 1];

                    image.drawLine(currentSample.x, currentSample.y, nextSample.x, nextSample.y, color);
                }
                // Draw line from last sample to next segment.
                const Position2 lastSample = samplePositions[samplePositions.size() - 1];
                image.drawLine(lastSample.x, lastSample.y, x1, yMiddle, color);
            }
            break;

            case CUT: {
                // drawWiggle(image,
                //     cellPosition.x0,
                //     cellPosition.x1,
                //     cellPosition.y0 + layout.pulses.pulseRowHeightMicrowave / 2,
                //     cellPosition.x1 - cellPosition.x0,
                //     layout.pulses.pulseRowHeightMicrowave / 8,
                //     layout.pulses.pulseColorMicrowave);
            }
            break;
        }
    }
}

void drawCycle(Image &image,
               const CircuitLayout &layout,
               const CircuitData &circuitData,
               const Structure &structure,
               const Cycle &cycle)
{
    // Draw each of the chunks in the cycle's gate partition.
    for (UInt chunkIndex = 0; chunkIndex < cycle.gates.size(); chunkIndex++)
    {
        const Int chunkOffset = utoi(chunkIndex) * structure.getCellDimensions().width;

        // Draw each of the gates in the current chunk.
        for (const GateProperties &gate : cycle.gates[chunkIndex])
        {
            drawGate(image, layout, circuitData, gate, structure, chunkOffset);
        }
    }
}

void drawGate(Image &image,
              const CircuitLayout &layout,
              const CircuitData &circuitData,
              const GateProperties &gate,
              const Structure &structure,
              const Int chunkOffset) {
    // Get the gate visualization parameters.
    GateVisual gateVisual;
    if (layout.gateVisuals.count(gate.visual_type) == 1) {
        QL_DOUT("Found visual for gate: '" << gate.name << "'");
        gateVisual = layout.gateVisuals.at(gate.visual_type);
    } else {
        // TODO: if the above fails, display a dummy gate
        QL_WOUT("Did not find visual for gate: '" << gate.name << "' with visual_type " << gate.visual_type << ", skipping gate!");
        return;
    }

    // Fetch the operands used by this gate.
    Vec<GateOperand> operands = getGateOperands(gate);
    for (const GateOperand &operand : operands) {
        QL_DOUT("bitType: " << operand.bitType << " value: " << operand.index);
    }

    // Check for correct amount of nodes.
    if (operands.size() != gateVisual.nodes.size()) {
        QL_WOUT("Amount of gate operands: " << operands.size() << " and visualization nodes: " << gateVisual.nodes.size()
             << " are not equal. Skipping gate with name: '" << gate.name << "' ...");
        return;
    }

    if (operands.size() > 1) {
        // Draw the lines between each node. If this is done before drawing the
        // nodes, there is no need to calculate line segments, we can just draw 
        // one big line between the nodes and the nodes will be drawn on top of
        // those.

        QL_DOUT("Setting up multi-operand gate...");
        Pair<GateOperand, GateOperand> edgeOperands = calculateEdgeOperands(operands, circuitData.amountOfQubits);
        GateOperand minOperand = edgeOperands.first;
        GateOperand maxOperand = edgeOperands.second;

        const Int column = gate.cycle;
        QL_DOUT("minOperand.bitType: " << minOperand.bitType << " minOperand.operand " << minOperand.index);
        QL_DOUT("maxOperand.bitType: " << maxOperand.bitType << " maxOperand.operand " << maxOperand.index);
        QL_DOUT("cycle: " << column);

        const Position4 topCellPosition = structure.getCellPosition(column, minOperand.index, minOperand.bitType);
        const Position4 bottomCellPosition = structure.getCellPosition(column, maxOperand.index, maxOperand.bitType);
        const Position4 connectionPosition = {
            topCellPosition.x0 + chunkOffset + structure.getCellDimensions().width / 2,
            topCellPosition.y0 + structure.getCellDimensions().height / 2,
            bottomCellPosition.x0 + chunkOffset + structure.getCellDimensions().width / 2,
            bottomCellPosition.y0 + structure.getCellDimensions().height / 2,
        };

        //TODO: probably have connection line type as part of a gate's visual definition
        if (isMeasurement(gate)) {
            if (layout.measurements.isConnectionEnabled() && layout.bitLines.classical.isEnabled()) {
                const Int groupedClassicalLineOffset = layout.bitLines.classical.isGrouped() ? layout.bitLines.classical.getGroupedLineGap() : 0;

                image.drawLine(connectionPosition.x0 - layout.measurements.getLineSpacing(),
                    connectionPosition.y0,
                    connectionPosition.x1 - layout.measurements.getLineSpacing(),
                    connectionPosition.y1 - layout.measurements.getArrowSize() - groupedClassicalLineOffset,
                    gateVisual.connectionColor);

                image.drawLine(connectionPosition.x0 + layout.measurements.getLineSpacing(),
                    connectionPosition.y0,
                    connectionPosition.x1 + layout.measurements.getLineSpacing(),
                    connectionPosition.y1 - layout.measurements.getArrowSize() - groupedClassicalLineOffset,
                    gateVisual.connectionColor);

                const Int x0 = connectionPosition.x1 - layout.measurements.getArrowSize() / 2;
                const Int y0 = connectionPosition.y1 - layout.measurements.getArrowSize() - groupedClassicalLineOffset;
                const Int x1 = connectionPosition.x1 + layout.measurements.getArrowSize() / 2;
                const Int y1 = connectionPosition.y1 - layout.measurements.getArrowSize() - groupedClassicalLineOffset;
                const Int x2 = connectionPosition.x1;
                const Int y2 = connectionPosition.y1 - groupedClassicalLineOffset;
                image.drawFilledTriangle(x0, y0, x1, y1, x2, y2, gateVisual.connectionColor);
            }
        } else {
            image.drawLine(connectionPosition.x0, connectionPosition.y0, connectionPosition.x1, connectionPosition.y1, gateVisual.connectionColor);
        }
        QL_DOUT("Finished setting up multi-operand gate");
    }

    // Draw the gate duration outline if the option has been set.
    if (!layout.cycles.areCompressed() && layout.gateDurationOutlines.areEnabled()) {
        QL_DOUT("Drawing gate duration outline...");
        // Only draw the gate outline if the gate takes more than one cycle.
        if (gate.durationInCycles > 1) {
            for (UInt i = 0; i < operands.size(); i++) {
                const Int columnStart = gate.cycle;
                const Int columnEnd = columnStart + gate.durationInCycles - 1;
                const Int row = (i >= gate.operands.size()) ? gate.creg_operands[i - gate.operands.size()] : gate.operands[i];
                QL_DOUT("i: " << i << " size: " << gate.operands.size() << " value: " << gate.operands[i]);

                const Int x0 = structure.getCellPosition(columnStart, row, QUANTUM).x0 + chunkOffset + layout.gateDurationOutlines.getGap();
                const Int y0 = structure.getCellPosition(columnStart, row, QUANTUM).y0 + layout.gateDurationOutlines.getGap();
                const Int x1 = structure.getCellPosition(columnEnd, row, QUANTUM).x1 - layout.gateDurationOutlines.getGap();
                const Int y1 = structure.getCellPosition(columnEnd, row, QUANTUM).y1 - layout.gateDurationOutlines.getGap();

                // Draw the outline in the colors of the node.
                const Node node = gateVisual.nodes.at(i);
                image.drawFilledRectangle(x0, y0, x1, y1, node.backgroundColor, layout.gateDurationOutlines.getFillAlpha());
                image.drawOutlinedRectangle(x0, y0, x1, y1, node.outlineColor, layout.gateDurationOutlines.getOutlineAlpha(), LinePattern::DASHED);
                
                //image.draw_rectangle(x0, y0, x1, y1, layout.cycles.gateDurationOutlineColor.data(), layout.cycles.gateDurationAlpha);
                //image.draw_rectangle(x0, y0, x1, y1, layout.cycles.gateDurationOutlineColor.data(), layout.cycles.gateDurationOutLineAlpha, 0xF0F0F0F0);
            }
        }
    }

    // Draw the nodes.
    QL_DOUT("Drawing gate nodes...");
    for (UInt i = 0; i < operands.size(); i++) {
        QL_DOUT("Drawing gate node with index: " << i << "...");
        //TODO: change the try-catch later on! the gate config will be read from somewhere else than the default layout
        try {
            const Node node = gateVisual.nodes.at(i);
            const BitType operandType = (i >= gate.operands.size()) ? CLASSICAL : QUANTUM;
            const Int index = utoi((operandType == QUANTUM) ? i : (i - gate.operands.size()));

            const Cell cell = {
                gate.cycle,
                operandType == CLASSICAL ? gate.creg_operands.at(index) + circuitData.amountOfQubits : gate.operands.at(index),
                chunkOffset,
                operandType
            };

            switch (node.type) {
                case NONE:      QL_DOUT("node.type = NONE"); break; // Do nothing.
                case GATE:      QL_DOUT("node.type = GATE"); drawGateNode(image, layout, structure, node, cell); break;
                case CONTROL:   QL_DOUT("node.type = CONTROL"); drawControlNode(image, layout, structure, node, cell); break;
                case NOT:       QL_DOUT("node.type = NOT"); drawNotNode(image, layout, structure, node, cell); break;
                case CROSS:     QL_DOUT("node.type = CROSS"); drawCrossNode(image, layout, structure, node, cell); break;
                default:        QL_WOUT("Unknown gate display node type!"); break;
            }
        } catch (const Exception &e) {
            QL_WOUT(Str(e.what()));
            return;
        }
        
        QL_DOUT("Finished drawing gate node with index: " << i << "...");
    }
}

void drawGateNode(Image &image,
                  const CircuitLayout &layout,
                  const Structure &structure,
                  const Node &node,
                  const Cell &cell) {
    const Int xGap = (structure.getCellDimensions().width - node.radius * 2) / 2;
    const Int yGap = (structure.getCellDimensions().height - node.radius * 2) / 2;

    const Position4 cellPosition = structure.getCellPosition(cell.col, cell.row, cell.bitType);
    const Position4 position = {
        cellPosition.x0 + cell.chunkOffset + xGap,
        cellPosition.y0 + yGap,
        cellPosition.x0 + cell.chunkOffset + structure.getCellDimensions().width - xGap,
        cellPosition.y1 - yGap
    };

    // Draw the gate background.
    image.drawFilledRectangle(position.x0, position.y0, position.x1, position.y1, node.backgroundColor, 1);
    image.drawOutlinedRectangle(position.x0, position.y0, position.x1, position.y1, node.outlineColor, 1, LinePattern::UNBROKEN);

    // Draw the gate symbol. The width and height of the symbol are calculated first to correctly position the symbol within the gate.
    Dimensions textDimensions = calculateTextDimensions(node.displayName, node.fontHeight);
    image.drawText(position.x0 + (node.radius * 2 - textDimensions.width) / 2, position.y0 + (node.radius * 2 - textDimensions.height) / 2,
        node.displayName, node.fontHeight, node.fontColor);
}

void drawControlNode(Image &image,
                     const CircuitLayout &layout,
                     const Structure &structure,
                     const Node &node,
                     const Cell &cell) {
    const Position4 cellPosition = structure.getCellPosition(cell.col, cell.row, cell.bitType);
    const Position2 position = {
        cellPosition.x0 + cell.chunkOffset + structure.getCellDimensions().width / 2,
        cellPosition.y0 + cell.chunkOffset + structure.getCellDimensions().height / 2
    };

    image.drawFilledCircle(position.x, position.y, node.radius, node.backgroundColor);
}

void drawNotNode(Image &image,
                 const CircuitLayout &layout,
                 const Structure &structure,
                 const Node &node,
                 const Cell &cell) {
    // TODO: allow for filled not node instead of only an outline not node

    const Position4 cellPosition = structure.getCellPosition(cell.col, cell.row, cell.bitType);
    const Position2 position = {
        cellPosition.x0 + cell.chunkOffset + structure.getCellDimensions().width / 2,
        cellPosition.y0 + cell.chunkOffset + structure.getCellDimensions().height / 2
    };

    // Draw the outlined circle.
    image.drawOutlinedCircle(position.x, position.y, node.radius, node.backgroundColor, 1, LinePattern::UNBROKEN);

    // Draw two lines to represent the plus sign.
    const Int xHor0 = position.x - node.radius;
    const Int xHor1 = position.x + node.radius;
    const Int yHor = position.y;

    const Int xVer = position.x;
    const Int yVer0 = position.y - node.radius;
    const Int yVer1 = position.y + node.radius;

    image.drawLine(xHor0, yHor, xHor1, yHor, node.backgroundColor);
    image.drawLine(xVer, yVer0, xVer, yVer1, node.backgroundColor);
}

void drawCrossNode(Image &image,
                   const CircuitLayout &layout,
                   const Structure &structure,
                   const Node &node,
                   const Cell &cell) {
    const Position4 cellPosition = structure.getCellPosition(cell.col, cell.row, cell.bitType);
    const Position2 position = {
        cellPosition.x0 + cell.chunkOffset + structure.getCellDimensions().width / 2,
        cellPosition.y0 + cell.chunkOffset + structure.getCellDimensions().height / 2
    };

    // Draw two diagonal lines to represent the cross.
    const Int x0 = position.x - node.radius;
    const Int y0 = position.y - node.radius;
    const Int x1 = position.x + node.radius;
    const Int y1 = position.y + node.radius;

    image.drawLine(x0, y0, x1, y1, node.backgroundColor);
    image.drawLine(x0, y1, x1, y0, node.backgroundColor);
}

} // namespace detail
} // namespace visualize
} // namespace ana
} // namespace pass
} // namespace ql

#endif //WITH_VISUALIZER