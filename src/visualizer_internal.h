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

struct EndPoints
{
	const unsigned int start;
	const unsigned int end;
};

struct NodePositionData
{
	const unsigned int gap;
	const unsigned int labelColumnWidth;
	const unsigned int cycleNumbersRowHeight;

	const unsigned int column;
	const unsigned int row;
};

struct TextDimensions
{
	const unsigned int width;
	const unsigned int height;
};

struct CircuitData
{
	const unsigned int amountOfQubits;
	const unsigned int amountOfClassicalBits;
	const unsigned int amountOfCycles;
	const unsigned int cycleDuration;
};

class Structure
{
	private:
		unsigned int imageWidth;
		unsigned int imageHeight;

		unsigned int labelColumnWidth;
		unsigned int cycleNumbersRowHeight;

		const Layout layout;
		const CircuitData circuitData;

	public:
		Structure(const Layout layout, const CircuitData circuitData);

		unsigned int getImageWidth();
		unsigned int getImageHeight();

		unsigned int getCellX(const unsigned int col) const;
		unsigned int getCellY(const unsigned int row) const;

		unsigned int getCycleLabelsY() const;
		unsigned int getBitLabelsX() const;
		EndPoints Structure::getBitLineEndPoints() const;
};

Layout parseConfiguration(const std::string& configPath);
void validateLayout(const Layout layout);

unsigned int calculateAmountOfBits(const std::vector<ql::gate*> gates, const std::vector<size_t> ql::gate::* operandType);
unsigned int calculateAmountOfCycles(const std::vector<ql::gate*> gates, const unsigned int cycleDuration);
unsigned int calculateAmountOfGateOperands(const ql::gate* gate);
void compressCycles(const std::vector<ql::gate*> gates, unsigned int& amountOfCycles);

void fixMeasurementOperands(const std::vector<ql::gate*> gates);
bool isMeasurement(const ql::gate* gate);

TextDimensions calculateTextDimensions(const std::string& text, const unsigned int fontHeight, const Layout layout);

void drawCycleLabels(cimg_library::CImg<unsigned char>& image, const Layout layout, const CircuitData circuitData, const Structure structure);
void drawBitLine(cimg_library::CImg<unsigned char>& image, const Layout layout, const BitType bitType, const unsigned int row, const CircuitData circuitData, const Structure structure);
void drawGroupedClassicalBitLine(cimg_library::CImg<unsigned char>& image, const Layout layout, const CircuitData circuitData);
void drawGate(cimg_library::CImg<unsigned char>& image, const Layout layout, const CircuitData circuitData, ql::gate* const gate);

void drawGateNode(cimg_library::CImg<unsigned char>& image, const Layout layout, const CircuitData circuitData, const Node node, const NodePositionData positionData);
void drawControlNode(cimg_library::CImg<unsigned char>& image, const Layout layout, const CircuitData circuitData, const Node node, const NodePositionData positionData);
void drawNotNode(cimg_library::CImg<unsigned char>& image, const Layout layout, const CircuitData circuitData, const Node node, const NodePositionData positionData);
void drawCrossNode(cimg_library::CImg<unsigned char>& image, const Layout layout, const CircuitData circuitData, const Node node, const NodePositionData positionData);

} // ql

#endif //WITH_VISUALIZER

#endif //QL_VISUALIZER_INTERNAL_H