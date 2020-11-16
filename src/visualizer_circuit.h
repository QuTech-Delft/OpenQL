/**
 * @file   visualizer_circuit.h
 * @date   11/2020
 * @author Tim van der Meer
 * @brief  definition of the visualizer
 */

#pragma once

#ifdef WITH_VISUALIZER
 
#include "visualizer.h"
#include "visualizer_types.h"
#include "CImg.h"

namespace ql {

struct Cycle {
    int index;
    bool empty;
    bool cut;
    std::vector<std::vector<std::reference_wrapper<GateProperties>>> gates;

    Cycle() = delete;
};

struct Cell {
    const int col;
    const int row;
    const int chunkOffset;
    const BitType bitType;
};

enum LineSegmentType {FLAT, PULSE, CUT};

struct Pulse {
    const std::vector<double> waveform;
    const int sampleRate;
};

struct LineSegment {
    const LineSegmentType type;
    const EndPoints range;
    const Pulse pulse;
};

// enum PulseType {MICROWAVE, FLUX, READOUT};

struct Line {
    // LineType type = MICROWAVE;
    std::vector<LineSegment> segments;
    double maxAmplitude = 0;
};

struct QubitLines {
    Line microwave;
    Line flux;
    Line readout;
};

struct GatePulses {
    std::vector<double> microwave;
    std::vector<double> flux;
    std::vector<double> readout;
};

struct PulseVisualization {
    int sampleRateMicrowave = 0;
    int sampleRateFlux = 0;
    int sampleRateReadout = 0;

    std::map<int, std::map<int, GatePulses>> mapping;
};

class CircuitData {
    private:
    std::vector<Cycle> cycles;
    std::vector<EndPoints> cutCycleRangeIndices;

    int calculateAmountOfCycles(const std::vector<GateProperties> gates, const int cycleDuration) const;
    std::vector<Cycle> generateCycles(std::vector<GateProperties> &gates, const int cycleDuration) const;
    std::vector<EndPoints> findCuttableEmptyRanges(const CircuitLayout layout) const;

    void compressCycles();
    void partitionCyclesWithOverlap();
    void cutEmptyCycles(const CircuitLayout layout);

    public:
    const int amountOfQubits;
    const int amountOfClassicalBits;
    const int cycleDuration;

    CircuitData(std::vector<GateProperties> &gates, const CircuitLayout layout, const int cycleDuration);

    Cycle getCycle(const size_t index) const;
    int getAmountOfCycles() const;
    bool isCycleCut(const int cycleIndex) const;
    bool isCycleFirstInCutRange(const int cycleIndex) const;

    void printProperties() const;
};

class Structure {
    private:
    const CircuitLayout layout;

    const Dimensions cellDimensions;

    const int cycleLabelsY;
    const int bitLabelsX;

    int imageWidth = 0;
    int imageHeight = 0;

    std::vector<std::vector<Position4>> qbitCellPositions;
    std::vector<std::vector<Position4>> cbitCellPositions;
    std::vector<std::pair<EndPoints, bool>> bitLineSegments;

    int calculateCellHeight(const CircuitLayout layout) const;
    int calculateImageWidth(const CircuitData circuitData) const;
    int calculateImageHeight(const CircuitData circuitData) const;

    void generateBitLineSegments(const CircuitData circuitData);
    void generateCellPositions(const CircuitData circuitData);

    public:
    Structure(const CircuitLayout layout, const CircuitData circuitData);

    int getImageWidth() const;
    int getImageHeight() const;

    int getCycleLabelsY() const;
    int getBitLabelsX() const;

    int getCircuitTopY() const;
    int getCircuitBotY() const;

    Dimensions getCellDimensions() const;
    Position4 getCellPosition(const size_t column, const size_t row, const BitType bitType) const;
    std::vector<std::pair<EndPoints, bool>> getBitLineSegments() const;

    void printProperties() const;
};

void visualizeCircuit(const ql::quantum_program* program, const VisualizerConfigurationPaths configurationPaths);

CircuitLayout parseCircuitConfiguration(const std::string &configPath);
void validateCircuitLayout(CircuitLayout &layout);
PulseVisualization parseWaveformMapping(const std::string &waveformMappingPath);

std::vector<QubitLines> generateQubitLines(const std::vector<GateProperties> gates, const PulseVisualization pulseVisualization, const CircuitData circuitData);
double calculateMaxAmplitude(const std::vector<LineSegment> lineSegments);
void insertFlatLineSegments(std::vector<LineSegment> &existingLineSegments, const int amountOfCycles);

void drawCycleLabels(cimg_library::CImg<unsigned char> &image, const CircuitLayout layout, const CircuitData circuitData, const Structure structure);
void drawCycleEdges(cimg_library::CImg<unsigned char> &image, const CircuitLayout layout, const CircuitData circuitData, const Structure structure);
void drawBitLineLabels(cimg_library::CImg<unsigned char> &image, const CircuitLayout layout, const CircuitData circuitData, const Structure structure);
void drawBitLineEdges(cimg_library::CImg<unsigned char> &image, const CircuitLayout layout, const CircuitData circuitData, const Structure structure);

void drawBitLine(cimg_library::CImg<unsigned char> &image, const CircuitLayout layout, const BitType bitType, const int row, const CircuitData circuitData, const Structure structure);
void drawGroupedClassicalBitLine(cimg_library::CImg<unsigned char> &image, const CircuitLayout layout, const CircuitData circuitData, const Structure structure);

void drawWiggle(cimg_library::CImg<unsigned char> &image, const int x0, const int x1, const int y, const int width, const int height, const Color color);

void drawLine(cimg_library::CImg<unsigned char> &image, const Structure structure, const int cycleDuration, const Line line, const int qubitIndex, const int y, const int maxLineHeight, const Color color);

void drawCycle(cimg_library::CImg<unsigned char> &image, const CircuitLayout layout, const CircuitData circuitData, const Structure structure, const Cycle cycle);
void drawGate(cimg_library::CImg<unsigned char> &image, const CircuitLayout layout, const CircuitData circuitData, const GateProperties gate, const Structure structure, const int chunkOffset);
void drawGateNode(cimg_library::CImg<unsigned char> &image, const CircuitLayout layout, const Structure structure, const Node node, const Cell cell);
void drawControlNode(cimg_library::CImg<unsigned char> &image, const CircuitLayout layout, const Structure structure, const Node node, const Cell cell);
void drawNotNode(cimg_library::CImg<unsigned char> &image, const CircuitLayout layout, const Structure structure, const Node node, const Cell cell);
void drawCrossNode(cimg_library::CImg<unsigned char> &image, const CircuitLayout layout, const Structure structure, const Node node, const Cell cell);

} // namespace ql

#endif //WITH_VISUALIZER
