/** \file
 * Declaration of the public visualizer interface.
 */
 
#pragma once

#include "program.h"
#include "utils/num.h"
#include "utils/str.h"
#include "utils/vec.h"

namespace ql {

struct VisualizerConfiguration {
    const utils::Str &visualizationType;
    const utils::Str &visualizerConfigPath;
    const utils::Str &waveformMappingPath;
};

void visualize(const quantum_program* program, const VisualizerConfiguration &configuration);

#ifdef WITH_VISUALIZER

void assertPositive(const utils::Int parameterValue, const utils::Str &parameterName);
void assertPositive(const utils::Real parameterValue, const utils::Str &parameterName);

#endif //WITH_VISUALIZER

} // namespace ql