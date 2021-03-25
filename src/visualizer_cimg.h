/** \file
 * Wrapper for the CImg library.
 */

#pragma once

#ifdef WITH_VISUALIZER

#include "visualizer_types.h"
#include "utils/num.h"
#include "utils/str.h"

#include "CImg.h"

// Undef garbage left behind by CImg.
#undef cimg_use_opencv
#undef Bool
#undef True
#undef False
#undef IN
#undef OUT

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
    Image(const utils::Int imageWidth, const utils::Int imageHeight);

    void fill(const Color color);

    void drawLine(const utils::Int x0, const utils::Int y0, const utils::Int x1, const utils::Int y1,
                  const Color color = black, const utils::Real alpha = 1, const LinePattern pattern = LinePattern::UNBROKEN);

    void drawText(const utils::Int x, const utils::Int y, const utils::Str &text, const utils::Int height,
                  const Color color = black);

    void drawFilledCircle(const utils::Int centerX, const utils::Int centerY, const utils::Int radius,
                          const Color color = black, const utils::Real alpha = 1);
    void drawOutlinedCircle(const utils::Int centerX, const utils::Int centerY, const utils::Int radius,
                            const Color color = black, const utils::Real alpha = 1, const LinePattern pattern = LinePattern::UNBROKEN);

    void drawFilledTriangle(const utils::Int x0, const utils::Int y0, const utils::Int x1, const utils::Int y1, const utils::Int x2, const utils::Int y2,
                            const Color color = black, const utils::Real alpha = 1);
    void drawOutlinedTriangle(const utils::Int x0, const utils::Int y0, const utils::Int x1, const utils::Int y1, const utils::Int x2, const utils::Int y2,
                              const Color color = black, const utils::Real alpha = 1, const LinePattern pattern = LinePattern::UNBROKEN);

    void drawFilledRectangle(const utils::Int x0, const utils::Int y0, const utils::Int x1, const utils::Int y1,
                             const Color color = black, const utils::Real alpha = 1);
    void drawOutlinedRectangle(const utils::Int x0, const utils::Int y0, const utils::Int x1, const utils::Int y1,
                               const Color color = black, const utils::Real alpha = 1, const LinePattern pattern = LinePattern::UNBROKEN);
    
    void save(const utils::Str &filename);
    void display(const utils::Str &caption);
};

Dimensions calculateTextDimensions(const utils::Str &text, const utils::Int fontHeight);

} // namespace ql

#endif // WITH_VISUALIZER