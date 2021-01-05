/**
 * @file    datapath_cc.cc
 * @date    20201119
 * @author  Wouter Vlothuizen (wouter.vlothuizen@tno.nl)
 * @brief   handling of Central Controller datapath (input MUX, Distributed Shared Memory, output PL)
 * @note
 */

#include "datapath_cc.h"
#include "utils/exception.h"	// FIXME: required for FATAL
#include "utils/num.h"

namespace ql {

void datapath_cc::programStart()
{
	datapathSection << std::left;    // assumed by emit()
	emit(".DATAPATH", "");
}


void datapath_cc::programFinish()
{
	emit(".END", "");
}

static unsigned int roundUp(unsigned int val, unsigned int mult) { return (val+mult-1)/mult*mult; }

unsigned int datapath_cc::allocateSmBit(size_t breg_operand, size_t instrIdx)
{
	// Some requirements from hardware:
	// - different instruments must use SM bits located in different DSM transfers
	// - the current maximum required DSM transfer size is 16 bit (using a ZI UHFQA). The
	//   hardware maximum is 32 bit (and may be utilized by e.g. the ZI SHF)
	// - all DSM bits used for the conditional gates of a single bundle must reside in
	//   a 128 bit window, aligned on 128 bit (16 byte)
	// - DSM size is 1024 bits (128 bytes)
	// Other notes:
	// - we don't attempt to be smart about DSM transfer size allocation
	// - new allocations to the same breg_operand overwrite the old mapping
	// - we don't reuse SM bits (thus wasting space)

	unsigned int smBit = 0;
	if(!mapBregToSmBit.empty()) {	// not first alloc
		// perform allocation
		if(instrIdx != smBitLastInstrIdx) {
			smBit = roundUp(lastSmBit+1, MAX_DSM_XFER_SIZE);
		} else {
			smBit = lastSmBit+1;
		}
		if(smBit >= SM_BIT_CNT) {
			QL_FATAL("Exceeded available Shared memory space of " << SM_BIT_CNT << " bits");
		}

		auto it = mapBregToSmBit.find(breg_operand);
		if(it != mapBregToSmBit.end()) {
			QL_IOUT("Overwriting mapping of breg_operand " << it->second);
		}
	}

	QL_DOUT("Mapping breg_operand " << breg_operand << " to smBit " << smBit);
	mapBregToSmBit[breg_operand] = smBit;	// created on demand

	smBitLastInstrIdx = instrIdx;
	lastSmBit = smBit;

	return smBit;
}


unsigned int datapath_cc::getSmBit(size_t breg_operand, size_t instrIdx)
{
	int smBit;

	auto it = mapBregToSmBit.find(breg_operand);
	if(it != mapBregToSmBit.end()) {
		smBit = it->second;
		QL_DOUT("Found mapping: breg_operand " << breg_operand << " to smBit " << smBit);
	} else {
		QL_FATAL("Request for DSM bit of breg_operand " << breg_operand << " that was never assigned by measurement");		// NB: message refers to user perspective (and thus calling semantics)
	}
	return smBit;
}


unsigned int datapath_cc::getOrAssignMux(size_t instrIdx, const tFeedbackMap &feedbackMap)
{
	// We need a different MUX for every new combination of simultaneous readouts (per instrument)
	unsigned int mux = lastMux[instrIdx]++;	// FIXME: no reuse of identical combinations yet
	if(mux == MUX_CNT) {
		QL_FATAL("Maximum number of available CC datapath MUXes exceeded");
	}

	return mux;
}


unsigned int datapath_cc::getOrAssignPl(size_t instrIdx, const tCondGateMap &condGateMap)
{
	// We need a different PL for every new combination of simultaneous gate conditions (per instrument)
	unsigned int pl = lastPl[instrIdx]++;	// FIXME: no reuse of identical combinations yet
	if(pl == PL_CNT) {
		QL_FATAL("Maximum number of available CC datapath PLs exceeded");
	}

	return pl;
}


unsigned int datapath_cc::getSizeTag(unsigned int numReadouts)
{
	int sizeTag;

	if(numReadouts == 0) {
		QL_FATAL("inconsistency in number of readouts");	// FIXME: message refers to caller, this assumes particular semantics for calling this function
	} else if(numReadouts <= 8) {
		sizeTag = 0;			// 0=byte
	} else if(numReadouts <= 16) {
		sizeTag = 1;
	} else if(numReadouts <= 32) {	// NB: should currently not occur since we have a maximum of 16 inputs on UHFQA
		sizeTag = 2;
	} else {
		QL_FATAL("inconsistency detected: too many readouts");
	}
	return sizeTag;
}


static std::string cond_qasm(cond_type_t condition, const Vec<UInt> &cond_operands)
{
	// FIXME: hack
	custom_gate g("foo");
	g.condition = condition;
	g.cond_operands = cond_operands;
	return g.cond_qasm();
}


void datapath_cc::emitMux(unsigned int mux, const tFeedbackMap &feedbackMap, size_t instrIdx, int slot)
{
	if(feedbackMap.empty()) {
		QL_FATAL("feedbackMap must not be empty");
	}

	emit(slot, QL_SS2S(".MUX " << mux));

	for(auto &feedback : feedbackMap) {
		int group = feedback.first;
		tFeedbackInfo fi = feedback.second;

		unsigned int winBit = fi.smBit % MUX_SM_WIN_SIZE;

		emit(
			slot,
			QL_SS2S("SM[" << winBit << "] := I[" << fi.bit << "]"),
			QL_SS2S("# cop " /*FIXME << fi.bi->creg_operands[0]*/ << " = readout(q" << fi.bi->operands[0] << ")")
		);
	}
}


unsigned int datapath_cc::getMuxSmAddr(const tFeedbackMap &feedbackMap)
{
	unsigned int minSmBit = UINT_MAX;
	unsigned int maxSmBit = 0;

	if(feedbackMap.empty()) {
		QL_FATAL("feedbackMap must not be empty");
	}

	for(auto &feedback : feedbackMap) {
		int group = feedback.first;
		tFeedbackInfo fi = feedback.second;

		minSmBit = utils::min(minSmBit, fi.smBit);
		maxSmBit = utils::max(maxSmBit, fi.smBit);
	}

	// perform checks
	if(maxSmBit/MUX_SM_WIN_SIZE != minSmBit/MUX_SM_WIN_SIZE) {
		QL_FATAL("Cannot access DSM bits " << minSmBit << " and " << maxSmBit << " in single MUX configuration");
	}
	return minSmBit/MUX_SM_WIN_SIZE;
}


// FIXME: split like emitMux/getMuxSmAddr
unsigned int datapath_cc::emitPl(unsigned int pl, const tCondGateMap &condGateMap, size_t instrIdx, int slot)
{
	bool minMaxValid = false;	// we may not access SM
	unsigned int minSmBit = UINT_MAX;
	unsigned int maxSmBit = 0;

	if(condGateMap.empty()) {
		QL_FATAL("condGateMap must not be empty");
	}

	emit(slot, QL_SS2S(".PL " << pl));

	for(auto &cg : condGateMap) {
		int group = cg.first;
		tCondGateInfo cgi = cg.second;

		// emit comment for group
		std::string condition = cond_qasm(cgi.condition, cgi.cond_operands);
		emit(
			slot,
			QL_SS2S("# group " << group << ", digOut=0x" << std::hex << std::setfill('0') << std::setw(8) << cgi.groupDigOut << ", condition='" << condition << "'")
		);

		// shorthand
		auto winBit = [this, cgi, instrIdx, &minMaxValid, &minSmBit, &maxSmBit](int i)
		{
			unsigned int smBit = getSmBit(cgi.cond_operands[i], instrIdx);
			minMaxValid = true;
			minSmBit = utils::min(minSmBit, smBit);
			maxSmBit = utils::max(maxSmBit, smBit);
			return smBit % PL_SM_WIN_SIZE;
		};

		// compute RHS of PL expression
		std::string inv;
		std::stringstream rhs;
		switch(cgi.condition) {
			// 0 operands:
			case cond_always:
				rhs << "1";
				break;
			case cond_never:
				rhs << "0";
				break;

			// 1 operand:
			case cond_not:
				inv = "/";
				// fall through
			case cond_unary:
				rhs << "SM[" << winBit(0) << "]";
				break;

			// 2 operands
			case cond_nand:
				inv = "/";
				// fall through
			case cond_and:
				rhs << "SM[" << winBit(0) << "] & SM[" << winBit(1) << "]";
				break;

			case cond_nor:
				inv = "/";
				// fall through
			case cond_or:
				rhs << "SM[" << winBit(0) << "] | SM[" << winBit(1) << "]";
				break;

			case cond_nxor:
				inv = "/";
				// fall through
			case cond_xor:
				rhs << "SM[" << winBit(0) << "] ^ SM[" << winBit(1) << "]";
				break;
		}

		// emit PL logic
		for(int bit=0; bit<32; bit++) {
			if(1<<bit & cgi.groupDigOut) {
				emit(
					slot,
					QL_SS2S(inv << "O[" << bit << "] := " << rhs.str())
				);
			}
		}
	}

	// perform checks
	if(minMaxValid) {
		if(maxSmBit/PL_SM_WIN_SIZE != minSmBit/PL_SM_WIN_SIZE) {
			QL_FATAL("Cannot access DSM bits " << minSmBit << " and " << maxSmBit << " in single PL configuration");
		}
	}
	return minSmBit/PL_SM_WIN_SIZE;	// NB: irrelevant if !minMaxValid since SM is not accessed in that case
}

} // namespace ql
