/** \file
 * Wrapper for the CImg library.
 */

#pragma once

#ifdef WITH_VISUALIZER

#include "CImg.h"
#include "utils/num.h"
#include "utils/str.h"

namespace ql {

typedef std::array<utils::Byte, 3> Color;

enum class LinePattern : unsigned int {
    UNBROKEN = 0xFFFFFFFF
};

class Image {
private:
    cimg_library::CImg<unsigned char> cimg;

public:
    Image(const utils::Int imageWidth, const utils::Int imageHeight, const utils::Int numberOfChannels) {
        cimg((int) imageWidth, (int) imageHeight, 1, (int) numberOfChannels);
    }

    void fill(const utils::Int rgb) {
        cimg.fill((int) rgb);
    }

    void drawLine(const utils::Int x0, const utils::Int y0, const utils::Int x1, const utils::Int y1, const Color color) {
        cimg.draw_line((int) x0, (int) y0, (int) x1, (int) y1, color.data());
    }

    void drawText(const utils::Int x, const utils::Int y, const utils::Str &text, const Color color, const utils::Int height) {
        cimg.draw_text((int) x, (int) y, text.c_str(), color.data(), 0, 1, (int) height);
    }

    void drawFilledCircle(const utils::Int centerX, const utils::Int centerY, const utils::Int radius,
                          const Color color, const utils::Real opacity) {
        cimg.draw_circle((int) centerX, (int) centerY, (int) radius, color.data(), (float) opacity);
    }

    void drawOutlinedCircle(const utils::Int centerX, const utils::Int centerY, const utils::Int radius,
                            const Color color, const utils::Real opacity, const LinePattern pattern) {
        cimg.draw_circle((int) centerX, (int) centerY, (int) radius, color.data(), (float) opacity, static_cast<unsigned int>(pattern));
    }

    void save(const utils::Str &filename) {
        cimg.save(static_cast<std::string>(filename).c_str());
    }

    void display(const utils::Str &caption) {
        cimg.display(static_cast<std::string>(caption).c_str());
    }
};

} // namespace ql

#endif // WITH_VISUALIZER