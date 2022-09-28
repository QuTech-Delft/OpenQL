/** \file
 * Definition of the circuit visualizer.
 */

#pragma once

#ifdef WITH_VISUALIZER

#include "ql/utils/num.h"
#include "ql/utils/str.h"
#include "ql/utils/pair.h"
#include "ql/utils/vec.h"
#include "ql/utils/map.h"
#include "ql/utils/json.h"
#include "types.h"
#include "image.h"

namespace ql {
namespace pass {
namespace ana {
namespace visualize {
namespace detail {

struct Cycle {
    utils::Int index;
    utils::Bool empty;
    utils::Bool cut;
    utils::Vec<utils::Vec<std::reference_wrapper<GateProperties>>> gates;

    Cycle() = delete;
};

struct Cell {
    const utils::Int col;
    const utils::Int row;
    const utils::Int chunkOffset;
    const BitType bitType;
};

enum LineSegmentType {FLAT, PULSE, CUT};

struct Pulse {
    const utils::Vec<utils::Real> waveform;
    const utils::Int sampleRate;
};

struct LineSegment {
    const LineSegmentType type;
    const EndPoints range;
    const Pulse pulse;
};

struct Line {
    utils::Vec<LineSegment> segments;
    utils::Real maxAmplitude = 0;
};

struct QubitLines {
    Line microwave;
    Line flux;
    Line readout;
};

struct GatePulses {
    utils::Vec<utils::Real> microwave;
    utils::Vec<utils::Real> flux;
    utils::Vec<utils::Real> readout;

    GatePulses() = delete;
};

struct PulseVisualization {
    utils::Int sampleRateMicrowave = 0;
    utils::Int sampleRateFlux = 0;
    utils::Int sampleRateReadout = 0;

    utils::Map<utils::Int, utils::Map<utils::Int, GatePulses>> mapping;
};

class CircuitData {
private:
    utils::Vec<Cycle> cycles;
    utils::Vec<EndPoints> cutCycleRangeIndices;

    utils::Vec<Cycle> generateCycles(utils::Vec<GateProperties> &gates, const utils::Int cycleDuration) const;
    utils::Vec<EndPoints> findCuttableEmptyRanges(const CircuitLayout &layout) const;

    void compressCycles();
    void partitionCyclesWithOverlap();
    void cutEmptyCycles(const CircuitLayout &layout);

public:
    const utils::Int amountOfQubits;
    const utils::Int amountOfClassicalBits;
    const utils::Int cycleDuration;

    CircuitData(utils::Vec<GateProperties> &gates, const CircuitLayout &layout, const utils::Int cycleDuration);

    Cycle getCycle(const utils::UInt index) const;
    utils::Int getAmountOfCycles() const;
    utils::Bool isCycleCut(const utils::Int cycleIndex) const;
    utils::Bool isCycleFirstInCutRange(const utils::Int cycleIndex) const;

    void printProperties() const;
};

class Structure {
private:
    const CircuitLayout layout;

    const Dimensions cellDimensions;

    const utils::Int cycleLabelsY;
    const utils::Int bitLabelsX;

    utils::Int imageWidth = 0;
    utils::Int imageHeight = 0;

    const utils::Vec<utils::Int> minCycleWidths;

    utils::Vec<utils::Vec<Position4>> qbitCellPositions;
    utils::Vec<utils::Vec<Position4>> cbitCellPositions;
    utils::Vec<utils::Pair<EndPoints, utils::Bool>> bitLineSegments;

    utils::Int calculateCellHeight(const CircuitLayout &layout) const;
    utils::Int calculateImageWidth(const CircuitData &circuitData) const;
    utils::Int calculateImageHeight(const CircuitData &circuitData, const utils::Int extendedImageHeight) const;

    void generateBitLineSegments(const CircuitData &circuitData);
    void generateCellPositions(const CircuitData &circuitData);

public:
    Structure(const CircuitLayout &layout, const CircuitData &circuitData, const utils::Vec<utils::Int> minCycleWidths, const utils::Int extendedImageHeight);

    utils::Int getImageWidth() const;
    utils::Int getImageHeight() const;

    utils::Int getCycleLabelsY() const;
    utils::Int getBitLabelsX() const;

    utils::Int getCircuitTopY() const;
    utils::Int getCircuitBotY() const;

    utils::Int getMinCycleWidth() const;
    Dimensions getCellDimensions() const;
    Position4 getCellPosition(const utils::UInt column, const utils::UInt row, const BitType bitType) const;
    utils::Vec<utils::Pair<EndPoints, utils::Bool>> getBitLineSegments() const;

    void printProperties() const;
};

struct ImageOutput {
    Image image;
    const CircuitLayout circuitLayout;
    const CircuitData circuitData;
    const Structure structure;
};

void visualizeCircuit(const ir::compat::ProgramRef &program, const VisualizerConfiguration &configuration);
ImageOutput generateImage(const ir::compat::ProgramRef &program, const VisualizerConfiguration &configuration, const utils::Vec<utils::Int> &minCycleWidths, utils::Int extendedImageHeight);

CircuitLayout parseCircuitConfiguration(utils::Vec<GateProperties> &gates, const utils::Str &configPath, const utils::Json &platformInstructions);
void validateCircuitLayout(CircuitLayout &layout, const utils::Str &visualizationType);
PulseVisualization parseWaveformMapping(const utils::Str &waveformMappingPath);

utils::Vec<QubitLines> generateQubitLines(const utils::Vec<GateProperties> &gates, const PulseVisualization &pulseVisualization, const CircuitData &circuitData);
utils::Real calculateMaxAmplitude(const utils::Vec<LineSegment> &lineSegments);
void insertFlatLineSegments(utils::Vec<LineSegment> &existingLineSegments, utils::Int amountOfCycles);

void drawCycleLabels(Image &image, const CircuitLayout &layout, const CircuitData &circuitData, const Structure &structure);
void drawCycleEdges(Image &image, const CircuitLayout &layout, const CircuitData &circuitData, const Structure &structure);
void drawBitLineLabels(Image &image, const CircuitLayout &layout, const CircuitData &circuitData, const Structure &structure);
void drawBitLineEdges(Image &image, const CircuitLayout &layout, const CircuitData &circuitData, const Structure &structure);

void drawBitLine(Image &image, const CircuitLayout &layout, BitType bitType, utils::Int row, const CircuitData &circuitData, const Structure &structure);
void drawGroupedClassicalBitLine(Image &image, const CircuitLayout &layout, const CircuitData &circuitData, const Structure &structure);

void drawWiggle(Image &image, utils::Int x0, utils::Int x1, utils::Int y, utils::Int width, utils::Int height, Color color);

void drawLine(Image &image, const Structure &structure, utils::Int cycleDuration, const Line &line, utils::Int qubitIndex, utils::Int y, utils::Int maxLineHeight, Color color);

void drawCycle(Image &image, const CircuitLayout &layout, const CircuitData &circuitData, const Structure &structure, const Cycle &cycle);
void drawGate(Image &image, const CircuitLayout &layout, const CircuitData &circuitData, const GateProperties &gate, const Structure &structure, utils::Int chunkOffset);
void drawGateNode(Image &image, const CircuitLayout &layout, const Structure &structure, const Node &node, const Cell &cell);
void drawControlNode(Image &image, const CircuitLayout &layout, const Structure &structure, const Node &node, const Cell &cell);
void drawNotNode(Image &image, const CircuitLayout &layout, const Structure &structure, const Node &node, const Cell &cell);
void drawCrossNode(Image &image, const CircuitLayout &layout, const Structure &structure, const Node &node, const Cell &cell);

} // namespace detail
} // namespace visualize
} // namespace ana
} // namespace pass
} // namespace ql

#endif //WITH_VISUALIZER
