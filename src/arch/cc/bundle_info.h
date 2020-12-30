#pragma once

#include "options_cc.h"
#include "settings_cc.h"
#include "utils/vec.h"

#include <string>

namespace ql {

namespace { using namespace utils; }

class BundleInfo {
public:	// funcs
	BundleInfo() = default;

public:	// vars
	// output gates
	std::string signalValue;
	unsigned int durationInCycles = 0;
#if OPT_SUPPORT_STATIC_CODEWORDS
	int staticCodewordOverride = settings_cc::NO_STATIC_CODEWORD_OVERRIDE;
#endif
#if OPT_FEEDBACK
	// readout feedback
	bool isMeasFeedback = false;
	Vec<UInt> operands;							// NB: also used by OPT_PRAGMA
	Vec<UInt> creg_operands;
	Vec<UInt> breg_operands;

	// conditional gates
	cond_type_t condition = cond_always;		// FIXME
	Vec<UInt> cond_operands;
#endif
#if OPT_PRAGMA
	// pragma 'gates'
	const Json *pragma = nullptr;
#endif
}; // information for an instrument group (of channels), for a single instruction
// FIXME: rename tInstrInfo, store gate as annotation, move to class cc:IR, together with customGate(), bundleStart(), bundleFinish()?


} // namespace ql
