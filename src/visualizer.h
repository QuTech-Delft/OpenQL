/**
 * @file   visualizer.h
 * @date   11/2020
 * @author Tim van der Meer
 * @brief  declaration of the public visualizer interface
 */
 
#pragma once

#include "program.h"

#include <cstdint>

namespace ql {

struct VisualizerConfigurationPaths {
    const std::string &config;
    const std::string &waveformMapping;
};

void visualize(const ql::quantum_program* program, const std::string &visualizationType, const VisualizerConfigurationPaths configurationPaths);

#ifdef WITH_VISUALIZER

static const int MAX_ALLOWED_VISUALIZER_CYCLE = 2000;

void assertPositive(const int argument, const std::string &parameter);
void assertPositive(const double argument, const std::string &parameter);

#endif //WITH_VISUALIZER

} // namespace ql