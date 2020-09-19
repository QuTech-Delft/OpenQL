/**
 * @file   gate_visual.h
 * @date   08/2020
 * @author Tim van der Meer
 * @brief  contains the parameters used to control a gate's visualization
 */

#ifndef QL_GATE_VISUAL_H
#define QL_GATE_VISUAL_H

namespace ql
{

enum NodeType {NONE, GATE, CONTROL, NOT, CROSS};

struct Node
{
	NodeType type;

	int radius;

	std::string displayName;
	int fontHeight;
	std::array<unsigned char, 3> fontColor;

	std::array<unsigned char, 3> backgroundColor;
	std::array<unsigned char, 3> outlineColor;
};

struct GateVisual
{
	std::array<unsigned char, 3> connectionColor;
	std::vector<Node> nodes;
};

} // ql

#endif //QL_GATE_VISUAL_H