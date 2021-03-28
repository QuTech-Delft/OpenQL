/** \file
 * Declaration of the public visualizer interface.
 */
 
#pragma once

#include "ql/utils/num.h"
#include "ql/utils/str.h"
#include "ql/utils/vec.h"
#include "program.h"

namespace ql {
namespace pass {
namespace ana {
namespace visualize {

struct VisualizerConfiguration {
    const utils::Str &visualizationType;
    const utils::Str &visualizerConfigPath;
    const utils::Str &waveformMappingPath;
};

void visualize(const ir::Program &program, const VisualizerConfiguration &configuration);

// FIXME JvS: WITH_VISUALIZER must never appear in a public header file
#ifdef WITH_VISUALIZER

void assertPositive(const utils::Int parameterValue, const utils::Str &parameterName);
void assertPositive(const utils::Real parameterValue, const utils::Str &parameterName);

#endif //WITH_VISUALIZER

} // namespace visualize
} // namespace ana
} // namespace pass
} // namespace ql
