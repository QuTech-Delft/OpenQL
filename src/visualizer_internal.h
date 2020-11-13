/**
 * @file   visualizer_internal.h
 * @date   09/2020
 * @author Tim van der Meer
 * @brief  declaration of the visualizer's internals
 */

#pragma once

#ifdef WITH_VISUALIZER

#include "visualizer.h"
#include "CImg.h"

// These undefs are necessary to avoid name collisions between CImg and Lemon.
#undef cimg_use_opencv
#undef True

#undef False
#undef IN
#undef OUT

namespace ql {

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

struct Cell {
    const int col;
    const int row;
    const int chunkOffset;
    const BitType bitType;
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

struct Cycle {
    int index;
    bool empty;
    bool cut;
    std::vector<std::vector<std::reference_wrapper<GateProperties>>> gates;

    Cycle() = delete;
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

    int calculateAmountOfBits(const std::vector<GateProperties> gates, const std::vector<int> GateProperties::* operandType) const;
    int calculateAmountOfCycles(const std::vector<GateProperties> gates, const int cycleDuration) const;
    std::vector<Cycle> generateCycles(std::vector<GateProperties> &gates, const int cycleDuration) const;
    std::vector<EndPoints> findCuttableEmptyRanges(const Layout layout) const;

    void compressCycles();
    void partitionCyclesWithOverlap();
    void cutEmptyCycles(const Layout layout);

    public:
    const int amountOfQubits;
    const int amountOfClassicalBits;
    const int cycleDuration;

    CircuitData(std::vector<GateProperties> &gates, const Layout layout, const int cycleDuration);

    Cycle getCycle(const size_t index) const;
    int getAmountOfCycles() const;
    bool isCycleCut(const int cycleIndex) const;
    bool isCycleFirstInCutRange(const int cycleIndex) const;

    void printProperties() const;
};

class Structure {
    private:
    const Layout layout;

    const Dimensions cellDimensions;

    const int cycleLabelsY;
    const int bitLabelsX;

    int imageWidth = 0;
    int imageHeight = 0;

    std::vector<std::vector<Position4>> qbitCellPositions;
    std::vector<std::vector<Position4>> cbitCellPositions;
    std::vector<std::pair<EndPoints, bool>> bitLineSegments;

    int calculateCellHeight(const Layout layout) const;
    int calculateImageWidth(const CircuitData circuitData) const;
    int calculateImageHeight(const CircuitData circuitData) const;

    void generateBitLineSegments(const CircuitData circuitData);
    void generateCellPositions(const CircuitData circuitData);

    public:
    Structure(const Layout layout, const CircuitData circuitData);

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

Layout parseConfiguration(const std::string &configPath);
PulseVisualization parseWaveformMapping(const std::string &waveformMappingPath);
void validateLayout(Layout &layout);

std::vector<GateProperties> parseGates(const ql::quantum_program* program);

int calculateAmountOfGateOperands(const GateProperties gate);
std::vector<GateOperand> getGateOperands(const GateProperties gate);
std::pair<GateOperand, GateOperand> calculateEdgeOperands(const std::vector<GateOperand> operands, const int amountOfQubits);

void fixMeasurementOperands(std::vector<GateProperties> &gates);
bool isMeasurement(const GateProperties gate);

std::vector<QubitLines> generateQubitLines(const std::vector<GateProperties> gates, const PulseVisualization pulseVisualization, const CircuitData circuitData);
double calculateMaxAmplitude(const std::vector<LineSegment> lineSegments);
void insertFlatLineSegments(std::vector<LineSegment> &existingLineSegments, const int amountOfCycles);

Dimensions calculateTextDimensions(const std::string &text, const int fontHeight, const Layout layout);

void drawCycleLabels(cimg_library::CImg<unsigned char> &image, const Layout layout, const CircuitData circuitData, const Structure structure);
void drawCycleEdges(cimg_library::CImg<unsigned char> &image, const Layout layout, const CircuitData circuitData, const Structure structure);
void drawBitLineLabels(cimg_library::CImg<unsigned char> &image, const Layout layout, const CircuitData circuitData, const Structure structure);
void drawBitLineEdges(cimg_library::CImg<unsigned char> &image, const Layout layout, const CircuitData circuitData, const Structure structure);

void drawBitLine(cimg_library::CImg<unsigned char> &image, const Layout layout, const BitType bitType, const int row, const CircuitData circuitData, const Structure structure);
void drawGroupedClassicalBitLine(cimg_library::CImg<unsigned char> &image, const Layout layout, const CircuitData circuitData, const Structure structure);

void drawWiggle(cimg_library::CImg<unsigned char> &image, const int x0, const int x1, const int y, const int width, const int height, const Color color);

void drawLine(cimg_library::CImg<unsigned char> &image, const Structure structure, const int cycleDuration, const Line line, const int qubitIndex, const int y, const int maxLineHeight, const Color color);

void drawCycle(cimg_library::CImg<unsigned char> &image, const Layout layout, const CircuitData circuitData, const Structure structure, const Cycle cycle);
void drawGate(cimg_library::CImg<unsigned char> &image, const Layout layout, const CircuitData circuitData, const GateProperties gate, const Structure structure, const int chunkOffset);
void drawGateNode(cimg_library::CImg<unsigned char> &image, const Layout layout, const Structure structure, const Node node, const Cell cell);
void drawControlNode(cimg_library::CImg<unsigned char> &image, const Layout layout, const Structure structure, const Node node, const Cell cell);
void drawNotNode(cimg_library::CImg<unsigned char> &image, const Layout layout, const Structure structure, const Node node, const Cell cell);
void drawCrossNode(cimg_library::CImg<unsigned char> &image, const Layout layout, const Structure structure, const Node node, const Cell cell);

void printGates(const std::vector<GateProperties> gates);
int safe_int_cast(const size_t argument);

} // namespace ql

#endif //WITH_VISUALIZER
