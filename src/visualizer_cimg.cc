/** \file
 * Wrapper for the CImg library.
 */

#pragma once

#ifdef WITH_VISUALIZER

#include "visualizer_cimg.h"
#include "visualizer_types.h"
#include "CImg.h"
#include "utils/num.h"
#include "utils/str.h"

namespace ql {

using namespace utils;

Image::Image(const Int imageWidth, const Int imageHeight, const Int numberOfChannels) {
    cimg((int) imageWidth, (int) imageHeight, 1, (int) numberOfChannels);
}

void Image::fill(const Int rgb) {
    cimg.fill((int) rgb);
}

void Image::drawLine(const Int x0, const Int y0, const Int x1, const Int y1, const Color color, const utils::Real alpha, const LinePattern pattern) {
    cimg.draw_line((int) x0, (int) y0, (int) x1, (int) y1, color.data(), alpha, static_cast<unsigned int>(pattern));
}

void Image::drawText(const Int x, const Int y, const Str &text, const Int height, const Color color) {
    cimg.draw_text((int) x, (int) y, text.c_str(), color.data(), 0, 1, (int) height);
}

void Image::drawFilledCircle(const Int centerX, const Int centerY, const Int radius,
                        const Color color, const Real opacity) {
    cimg.draw_circle((int) centerX, (int) centerY, (int) radius, color.data(), (float) opacity);
}

void Image::drawOutlinedCircle(const Int centerX, const Int centerY, const Int radius,
                        const Color color, const Real opacity, const LinePattern pattern) {
    cimg.draw_circle((int) centerX, (int) centerY, (int) radius, color.data(), (float) opacity, static_cast<unsigned int>(pattern));
}

void Image::drawTriangle(const Int x0, const Int y0, const Int x1, const Int y1, const Int x2, const Int y2, const Color color) {
    cimg.draw_triangle(x0, y0, x1, y1, x2, y2, color.data(), 1);
}

void Image::drawFilledRectangle(const utils::Int x0, const utils::Int y0, const utils::Int x1, const utils::Int y1, const Color color, const utils::Real alpha) {
    cimg.draw_rectangle(x0, y0, x1, y1, color, alpha, static_cast<unsigned int>(LinePattern::UNBROKEN));
}

void Image::drawOutlinedRectangle(const utils::Int x0, const utils::Int y0, const utils::Int x1, const utils::Int y1, const Color color, const utils::Real alpha, const LinePattern pattern) {
    cimg.draw_rectangle(x0, y0, x1, y1, color, alpha, static_cast<unsigned int>(pattern));
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