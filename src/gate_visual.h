/** \file
 * Contains the parameters used to control a gate's visualization.
 */

#pragma once

#include "utils/num.h"
#include "utils/str.h"
#include "utils/vec.h"

namespace ql {

typedef std::array<utils::Byte, 3> Color;

enum NodeType {NONE, GATE, CONTROL, NOT, CROSS};

struct Node {
    NodeType type;

    utils::Int radius;

    utils::Str displayName;
    utils::Int fontHeight;
    Color fontColor;

    Color backgroundColor;
    Color outlineColor;
};

struct GateVisual {
    Color connectionColor;
    utils::Vec<Node> nodes;
};

} // namespace ql