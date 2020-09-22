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

struct CircuitData
{
	const int amountOfQubits;
	const int amountOfClassicalBits;
	const int amountOfCycles;
	const int cycleDuration;
};

class Structure
{
	private:
		int imageWidth;
		int imageHeight;

		int labelColumnWidth;
		int cycleNumbersRowHeight;

		const Layout layout;
		const CircuitData circuitData;

	public:
		Structure(const Layout layout, const CircuitData circuitData);

		int getImageWidth() const;
		int getImageHeight() const;

		int getCellX(const int col) const;
		int getCellY(const int row) const;

		int getCycleLabelsY() const;
		int getBitLabelsX() const;
		EndPoints Structure::getBitLineEndPoints() const;
};

Layout parseConfiguration(const std::string& configPath);
void validateLayout(const Layout layout);

int calculateAmountOfBits(const std::vector<ql::gate*> gates, const std::vector<size_t> ql::gate::* operandType);
int calculateAmountOfCycles(const std::vector<ql::gate*> gates, const int cycleDuration);
int calculateAmountOfGateOperands(const ql::gate* gate);
void compressCycles(const std::vector<ql::gate*> gates, int& amountOfCycles);
void cutEmptyCycles(const std::vector<ql::gate*> gates, const Layout layout);

void fixMeasurementOperands(const std::vector<ql::gate*> gates);
bool isMeasurement(const ql::gate* gate);

Dimensions calculateTextDimensions(const std::string& text, const int fontHeight, const Layout layout);

void drawCycleLabels(cimg_library::CImg<unsigned char>& image, const Layout layout, const CircuitData circuitData, const Structure structure);
void drawBitLine(cimg_library::CImg<unsigned char>& image, const Layout layout, const BitType bitType, const int row, const CircuitData circuitData, const Structure structure);
void drawGroupedClassicalBitLine(cimg_library::CImg<unsigned char>& image, const Layout layout, const CircuitData circuitData, const Structure structure);
void drawGate(cimg_library::CImg<unsigned char>& image, const Layout layout, const CircuitData circuitData, ql::gate* const gate, const Structure structure);

void drawGateNode(cimg_library::CImg<unsigned char>& image, const Layout layout, const Structure structure, const Node node, const Cell cell);
void drawControlNode(cimg_library::CImg<unsigned char>& image, const Layout layout, const Structure structure, const Node node, const Cell cell);
void drawNotNode(cimg_library::CImg<unsigned char>& image, const Layout layout, const Structure structure, const Node node, const Cell cell);
void drawCrossNode(cimg_library::CImg<unsigned char>& image, const Layout layout, const Structure structure, const Node node, const Cell cell);

} // ql

#endif //WITH_VISUALIZER

#endif //QL_VISUALIZER_INTERNAL_H