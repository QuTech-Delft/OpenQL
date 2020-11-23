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

struct VisualizerConfiguration {
    const std::string &visualizerConfigPath;
    const std::string &waveformMappingPath;
};

void visualize(const ql::quantum_program* program, const std::string &visualizationType, const VisualizerConfiguration configuration);

#ifdef WITH_VISUALIZER

void assertPositive(const int argument, const std::string &parameter);
void assertPositive(const double argument, const std::string &parameter);

#endif //WITH_VISUALIZER

} // namespace ql