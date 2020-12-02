/** \file
 * Declaration of the public visualizer interface.
 */
 
#pragma once

#include "utils/num.h"
#include "utils/str.h"
#include "utils/vec.h"
#include "utils/map.h"
#include "program.h"

#include <cstdint>

namespace ql {

struct VisualizerConfiguration {
    const utils::Str &visualizerConfigPath;
    const utils::Str &waveformMappingPath;
};

void visualize(const quantum_program* program, const utils::Str &visualizationType, const VisualizerConfiguration &configuration);

#ifdef WITH_VISUALIZER

void assertPositive(utils::Int argument, const utils::Str &parameter);
void assertPositive(utils::Real argument, const utils::Str &parameter);

#endif //WITH_VISUALIZER

} // namespace ql