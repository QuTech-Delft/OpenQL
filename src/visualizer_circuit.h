/** \file
 * Definition of the visualizer.
 */

#pragma once

#ifdef WITH_VISUALIZER

#include "utils/num.h"
#include "utils/str.h"
#include "utils/vec.h"
#include "utils/pair.h"
#include "utils/map.h"
#include "visualizer.h"
#include "visualizer_types.h"
#include "CImg.h"
#include "json.h"

// These undefs are necessary to avoid name collisions.
#undef cimg_use_opencv
#undef Bool
#undef True
#undef False
#undef IN
#undef OUT

namespace ql {

// static const int VISUALIZER_CYCLE_WARNING_THRESHOLD = 100;

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

// enum PulseType {MICROWAVE, FLUX, READOUT};

struct Line {
    // LineType type = MICROWAVE;
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

    utils::Int calculateAmountOfCycles(const utils::Vec<GateProperties> &gates, utils::Int cycleDuration) const;
    utils::Vec<Cycle> generateCycles(utils::Vec<GateProperties> &gates, utils::Int cycleDuration) const;
    utils::Vec<EndPoints> findCuttableEmptyRanges(const CircuitLayout &layout) const;

    void compressCycles();
    void partitionCyclesWithOverlap();
    void cutEmptyCycles(const CircuitLayout &layout);

public:
    const utils::Int amountOfQubits;
    const utils::Int amountOfClassicalBits;
    const utils::Int cycleDuration;

    CircuitData(utils::Vec<GateProperties> &gates, const CircuitLayout &layout, utils::Int cycleDuration);

    Cycle getCycle(utils::UInt index) const;
    utils::Int getAmountOfCycles() const;
    utils::Bool isCycleCut(utils::Int cycleIndex) const;
    utils::Bool isCycleFirstInCutRange(utils::Int cycleIndex) const;

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

    utils::Vec<utils::Vec<Position4>> qbitCellPositions;
    utils::Vec<utils::Vec<Position4>> cbitCellPositions;
    utils::Vec<utils::Pair<EndPoints, utils::Bool>> bitLineSegments;

    utils::Int calculateCellHeight(const CircuitLayout &layout) const;
    utils::Int calculateImageWidth(const CircuitData &circuitData) const;
    utils::Int calculateImageHeight(const CircuitData &circuitData) const;

    void generateBitLineSegments(const CircuitData &circuitData);
    void generateCellPositions(const CircuitData &circuitData);

public:
    Structure(const CircuitLayout &layout, const CircuitData &circuitData);

    utils::Int getImageWidth() const;
    utils::Int getImageHeight() const;

    utils::Int getCycleLabelsY() const;
    utils::Int getBitLabelsX() const;

    utils::Int getCircuitTopY() const;
    utils::Int getCircuitBotY() const;

    Dimensions getCellDimensions() const;
    Position4 getCellPosition(utils::UInt column, utils::UInt row, BitType bitType) const;
    utils::Vec<utils::Pair<EndPoints, utils::Bool>> getBitLineSegments() const;

    void printProperties() const;
};

void visualizeCircuit(const ql::quantum_program* program, const VisualizerConfiguration &configuration);

CircuitLayout parseCircuitConfiguration(utils::Vec<GateProperties> &gates, const utils::Str &configPath, const json platformInstructions);
void validateCircuitLayout(CircuitLayout &layout);

utils::Vec<QubitLines> generateQubitLines(const utils::Vec<GateProperties> &gates, const PulseVisualization &pulseVisualization, const CircuitData &circuitData);
utils::Real calculateMaxAmplitude(const utils::Vec<LineSegment> &lineSegments);
void insertFlatLineSegments(utils::Vec<LineSegment> &existingLineSegments, utils::Int amountOfCycles);

void drawCycleLabels(cimg_library::CImg<utils::Byte> &image, const CircuitLayout &layout, const CircuitData &circuitData, const Structure &structure);
void drawCycleEdges(cimg_library::CImg<utils::Byte> &image, const CircuitLayout &layout, const CircuitData &circuitData, const Structure &structure);
void drawBitLineLabels(cimg_library::CImg<utils::Byte> &image, const CircuitLayout &layout, const CircuitData &circuitData, const Structure &structure);
void drawBitLineEdges(cimg_library::CImg<utils::Byte> &image, const CircuitLayout &layout, const CircuitData &circuitData, const Structure &structure);

void drawBitLine(cimg_library::CImg<utils::Byte> &image, const CircuitLayout &layout, BitType bitType, utils::Int row, const CircuitData &circuitData, const Structure &structure);
void drawGroupedClassicalBitLine(cimg_library::CImg<utils::Byte> &image, const CircuitLayout &layout, const CircuitData &circuitData, const Structure &structure);

void drawWiggle(cimg_library::CImg<utils::Byte> &image, utils::Int x0, utils::Int x1, utils::Int y, utils::Int width, utils::Int height, Color color);

void drawLine(cimg_library::CImg<utils::Byte> &image, const Structure &structure, utils::Int cycleDuration, const Line &line, utils::Int qubitIndex, utils::Int y, utils::Int maxLineHeight, Color color);

void drawCycle(cimg_library::CImg<utils::Byte> &image, const CircuitLayout &layout, const CircuitData &circuitData, const Structure &structure, const Cycle &cycle);
void drawGate(cimg_library::CImg<utils::Byte> &image, const CircuitLayout &layout, const CircuitData &circuitData, const GateProperties &gate, const Structure &structure, utils::Int chunkOffset);
void drawGateNode(cimg_library::CImg<utils::Byte> &image, const CircuitLayout &layout, const Structure &structure, const Node &node, const Cell &cell);
void drawControlNode(cimg_library::CImg<utils::Byte> &image, const CircuitLayout &layout, const Structure &structure, const Node &node, const Cell &cell);
void drawNotNode(cimg_library::CImg<utils::Byte> &image, const CircuitLayout &layout, const Structure &structure, const Node &node, const Cell &cell);
void drawCrossNode(cimg_library::CImg<utils::Byte> &image, const CircuitLayout &layout, const Structure &structure, const Node &node, const Cell &cell);

} // namespace ql

#endif //WITH_VISUALIZER
