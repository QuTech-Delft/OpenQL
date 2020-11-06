/**
 * @file   gate_visual.h
 * @date   08/2020
 * @author Tim van der Meer
 * @brief  contains the parameters used to control a gate's visualization
 */

#pragma once

typedef std::array<unsigned char, 3> Color;

namespace ql {

enum NodeType {NONE, GATE, CONTROL, NOT, CROSS};

struct Node {
    NodeType type;

    int radius;

    std::string displayName;
    int fontHeight;
    Color fontColor;

    Color backgroundColor;
    Color outlineColor;
};

struct GateVisual {
    Color connectionColor;
    std::vector<Node> nodes;
};

} // namespace ql