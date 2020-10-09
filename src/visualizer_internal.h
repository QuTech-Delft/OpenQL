/**
 * @file   visualizer_internal.h
 * @date   09/2020
 * @author Tim van der Meer
 * @brief  declaration of the visualizer's internals
 */

#ifndef QL_VISUALIZER_INTERNAL_H
#define QL_VISUALIZER_INTERNAL_H

#ifdef WITH_VISUALIZER

#include "visualizer.h"
#include "CImg.h"

// These undefs are necessary to avoid name collisions between CImg and Lemon.
#undef cimg_use_opencv
#undef True
#undef False
#undef IN
#undef OUT

namespace ql
{

enum BitType {CLASSICAL, QUANTUM};

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

struct Cell
{
	const int col;
	const int row;
	const BitType bitType;
};

struct EndPoints
{
	const int start;
	const int end;
};

struct Dimensions
{
	const int width;
	const int height;
};

struct Cycle
{
	bool empty;
	bool cut;
	std::vector<ql::gate*> gates;
};

struct GateCopy
{
	std::string name;
    std::vector<size_t> operands;
    std::vector<size_t> creg_operands;
    size_t duration;
    size_t  cycle;
    gate_type_t type;
    std::string visual_type;
};

class CircuitData
{
	private:
		std::vector<Cycle> cycles;
		std::vector<EndPoints> cutCycleRangeIndices;

		int calculateAmountOfBits(const std::vector<ql::gate*> gates, const std::vector<size_t> ql::gate::* operandType) const;
		int calculateAmountOfCycles(const std::vector<ql::gate*> gates, const int cycleDuration) const;
		void compressCycles(const std::vector<ql::gate*> gates, int& amountOfCycles) const;
		std::vector<EndPoints> findCuttableEmptyRanges(const std::vector<ql::gate*> gates, const Layout layout) const;

	public:
		const int amountOfQubits;
		const int amountOfClassicalBits;
		const int cycleDuration;

		CircuitData(const std::vector<ql::gate*> gates, const Layout layout, const int cycleDuration);

		int getAmountOfCycles() const;
		std::vector<EndPoints> getCutCycleRangeIndices() const;
		bool isCycleCut(const int cycleIndex) const;
		bool isCycleFirstInCutRange(const int cycleIndex) const;

		void printProperties() const;
};

class Structure
{
	private:
		int imageWidth;
		int imageHeight;

		int cycleLabelsY;
		int bitLabelsX;

		std::vector<std::vector<Position4>> qbitCellPositions;
		std::vector<std::vector<Position4>> cbitCellPositions;
		
		std::vector<std::pair<EndPoints, bool>> bitLineSegments;

	public:
		Structure(const Layout layout, const CircuitData circuitData);

		int getImageWidth() const;
		int getImageHeight() const;

		int getCycleLabelsY() const;
		int getBitLabelsX() const;

		Position4 getCellPosition(int column, int row, BitType bitType) const;

		std::vector<std::pair<EndPoints, bool>> getBitLineSegments() const;

		void printProperties() const;
};

Layout parseConfiguration(const std::string& configPath);
void validateLayout(Layout& layout);

int calculateAmountOfGateOperands(const ql::gate* gate);

void fixMeasurementOperands(const std::vector<ql::gate*> gates);
bool isMeasurement(const ql::gate* gate);

Dimensions calculateTextDimensions(const std::string& text, const int fontHeight, const Layout layout);

void drawCycleLabels(cimg_library::CImg<unsigned char>& image, const Layout layout, const CircuitData circuitData, const Structure structure);
void drawBitLine(cimg_library::CImg<unsigned char>& image, const Layout layout, const BitType bitType, const int row, const CircuitData circuitData, const Structure structure);
void drawGroupedClassicalBitLine(cimg_library::CImg<unsigned char>& image, const Layout layout, const CircuitData circuitData, const Structure structure);
void drawGate(cimg_library::CImg<unsigned char>& image, const Layout layout, const CircuitData circuitData, const ql::gate* gate, const Structure structure);

void drawGateNode(cimg_library::CImg<unsigned char>& image, const Layout layout, const Structure structure, const Node node, const Cell cell);
void drawControlNode(cimg_library::CImg<unsigned char>& image, const Layout layout, const Structure structure, const Node node, const Cell cell);
void drawNotNode(cimg_library::CImg<unsigned char>& image, const Layout layout, const Structure structure, const Node node, const Cell cell);
void drawCrossNode(cimg_library::CImg<unsigned char>& image, const Layout layout, const Structure structure, const Node node, const Cell cell);

} // ql

#endif //WITH_VISUALIZER

#endif //QL_VISUALIZER_INTERNAL_H