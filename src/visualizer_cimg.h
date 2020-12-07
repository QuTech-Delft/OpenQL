/** \file
 * Wrapper for the CImg library.
 */

#pragma once

#ifdef WITH_VISUALIZER

#include "visualizer_types.h"
#include "CImg.h"
#include "utils/num.h"
#include "utils/str.h"

namespace ql {

typedef std::array<utils::Byte, 3> Color;

enum class LinePattern : unsigned int {
    UNBROKEN = 0xFFFFFFFF,
    DASHED = 0xF0F0F0F0
};

class Image {
private:
    cimg_library::CImg<unsigned char> cimg;

public:
    Image(const utils::Int imageWidth, const utils::Int imageHeight, const utils::Int numberOfChannels);

    void fill(const utils::Int rgb);
    void drawLine(const utils::Int x0, const utils::Int y0, const utils::Int x1, const utils::Int y1,
                  const Color color = black, const utils::Real alpha = 1, const LinePattern pattern = LinePattern::UNBROKEN);
    void drawText(const utils::Int x, const utils::Int y, const utils::Str &text, const utils::Int height, const Color color = black);

    void drawCircle(centerX, centerY, radius, color, alpha, )

    void drawFilledCircle(const utils::Int centerX, const utils::Int centerY, const utils::Int radius, const Color color, const utils::Real opacity);
    void drawOutlinedCircle(const utils::Int centerX, const utils::Int centerY, const utils::Int radius, const Color color, const utils::Real opacity, const LinePattern pattern);
    void drawTriangle(const utils::Int x0, const utils::Int y0, const utils::Int x1, const utils::Int y1, const utils::Int x2, const utils::Int y2, const Color color);
    void drawFilledRectangle(const utils::Int x0, const utils::Int y0, const utils::Int x1, const utils::Int y1, const Color color, const utils::Real alpha);
    void drawOutlinedRectangle(const utils::Int x0, const utils::Int y0, const utils::Int x1, const utils::Int y1, const Color color, const utils::Real alpha, const LinePattern pattern);
    void save(const utils::Str &filename);
    void display(const utils::Str &caption);
};

Dimensions calculateTextDimensions(const utils::Str &text, const utils::Int fontHeight);

} // namespace ql

#endif // WITH_VISUALIZER