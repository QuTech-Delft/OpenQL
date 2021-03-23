/** \file
 * Wrapper for the CImg library.
 */

#ifdef WITH_VISUALIZER

#include "visualizer_cimg.h"
#include "visualizer_types.h"
#include "CImg.h"
#include "utils/num.h"
#include "utils/str.h"

namespace ql {

using namespace utils;

Image::Image(const Int imageWidth, const Int imageHeight) : cimg((int) imageWidth, (int) imageHeight, 1, 3) {
    // empty
}

void Image::fill(const Color color) {
    cimg.fill(255);
    drawFilledRectangle(0, 0, cimg.width(), cimg.height(), color, 1.0f);
}

void Image::drawLine(const Int x0, const Int y0, const Int x1, const Int y1, const Color color, const Real alpha, const LinePattern pattern) {
    cimg.draw_line((int) x0, (int) y0, (int) x1, (int) y1, color.data(), (float) alpha, static_cast<unsigned int>(pattern));
}

void Image::drawText(const Int x, const Int y, const Str &text, const Int height, const Color color) {
    cimg.draw_text((int) x, (int) y, text.c_str(), color.data(), 0, 1, (int) height);
}

void Image::drawFilledCircle(const Int centerX, const Int centerY, const Int radius,
                             const Color color, const Real alpha) {
    cimg.draw_circle((int) centerX, (int) centerY, (int) radius, color.data(), (float) alpha);
}

void Image::drawOutlinedCircle(const Int centerX, const Int centerY, const Int radius,
                               const Color color, const Real alpha, const LinePattern pattern) {
    cimg.draw_circle((int) centerX, (int) centerY, (int) radius, color.data(), (float) alpha, static_cast<unsigned int>(pattern));
}

void Image::drawFilledTriangle(const Int x0, const Int y0, const Int x1, const Int y1, const Int x2, const Int y2,
                               const Color color, const Real alpha) {
    cimg.draw_triangle((int) x0, (int) y0, (int) x1, (int) y1, (int) x2, (int) y2, color.data(), (float) alpha);
}

void Image::drawOutlinedTriangle(const Int x0, const Int y0, const Int x1, const Int y1, const Int x2, const Int y2,
                                 const Color color, const Real alpha, const LinePattern pattern) {
    cimg.draw_triangle((int) x0, (int) y0, (int) x1, (int) y1, (int) x2, (int) y2, color.data(), (float) alpha, static_cast<unsigned int>(pattern));
}

void Image::drawFilledRectangle(const Int x0, const Int y0, const Int x1, const Int y1,
                                const Color color, const Real alpha) {
    cimg.draw_rectangle((int) x0, (int) y0, (int) x1, (int) y1, color.data(), (float) alpha);
}

void Image::drawOutlinedRectangle(const Int x0, const Int y0, const Int x1, const Int y1,
                                  const Color color, const Real alpha, const LinePattern pattern) {
    cimg.draw_rectangle((int) x0, (int) y0, (int) x1, (int) y1, color.data(), (float) alpha, static_cast<unsigned int>(pattern));
}

void Image::save(const Str &filename) {
    cimg.save(static_cast<std::string>(filename).c_str());
}

void Image::display(const Str &caption) {
    cimg.display(static_cast<std::string>(caption).c_str());
}

Dimensions calculateTextDimensions(const Str &text, const Int fontHeight) {
    const char* chars = text.c_str();
    cimg_library::CImg<unsigned char> imageTextDimensions;
    const char color = 1;
    imageTextDimensions.draw_text(0, 0, chars, &color, 0, 1, (int) fontHeight);

    return Dimensions { imageTextDimensions.width(), imageTextDimensions.height() };
}

} // namespace ql

#endif // WITH_VISUALIZER