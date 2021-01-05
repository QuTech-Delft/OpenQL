/**
 * @file    datapath_cc.cc
 * @date    20201119
 * @author  Wouter Vlothuizen (wouter.vlothuizen@tno.nl)
 * @brief   handling of Central Controller datapath (input MUX, Distributed Shared Memory, output PL)
 * @note
 */

#include "datapath_cc.h"
#include <utils/exception.h>	// FIXME: required for FATAL

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


int datapath_cc::allocateSmBit(size_t breg_operand, size_t instrIdx)
{
	// Some requirements from hardware:
	// - different instruments must use SM bits located in different DSM transfers
	// - the maximum required DSM transfer size (currently, using an UHFQA) is 16 bit
	// - all DSM bits used for the conditional gates of a single bundle must reside in
	//   a 128 bit window, aligned on 128 bit (16 byte)
	// - DSM size is 1024 bits (128 bytes)
	// Other notes:
	// - we don't attempt to be smart about DSM transfer size allocation
	// - we don't manage?/allow? multiple allocations to the same breg_operand


	static int smBit = 0;

	smBit++;	// FIXME

	return smBit;
}


int datapath_cc::getSmBit(size_t breg_operand, size_t instrIdx)
{
	return 0;	// FIXME
}


int datapath_cc::getOrAssignMux(size_t instrIdx, const tFeedbackMap &feedbackMap)
{
	// We need a different MUX for every new combination of simultaneous readouts (per instrument)
	int mux = lastMux[instrIdx]++;	// FIXME: no reuse of identical combinations yet
	if(mux == MUX_CNT) {
		QL_FATAL("Maximum number of available CC datapath MUXes exceeded");
	}

	return mux;
}


int datapath_cc::getOrAssignPl(size_t instrIdx, const tCondGateMap &condGateMap)
{
	// We need a different PL for every new combination of simultaneous gate conditions (per instrument)
	int pl = lastPl[instrIdx]++;	// FIXME: no reuse of identical combinations yet
	if(pl == PL_CNT) {
		QL_FATAL("Maximum number of available CC datapath PLs exceeded");
	}

	return pl;
}


int datapath_cc::getSizeTag(int numReadouts)
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


void datapath_cc::emitMux(int mux, int smAddr, const tFeedbackMap &feedbackMap, size_t instrIdx, int slot)
{
	// emit datapath code
	emit(slot, QL_SS2S(".MUX " << mux));
	for(auto &feedback : feedbackMap) {
		int group = feedback.first;
		tFeedbackInfo fi = feedback.second;

		emit(
			slot,
			QL_SS2S("SM[" << fi.smBit << "] := I[" << fi.bit << "]"),
			QL_SS2S("# cop " /*FIXME << fi.bi->creg_operands[0]*/ << " = readout(q" << fi.bi->operands[0] << ")")
		);

		int mySmAddr = fi.smBit / 8;	// byte addressable
	}
}


void datapath_cc::emitPl(int pl, int smAddr, const tCondGateMap &condGateMap, size_t instrIdx, int slot)
{
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

		// emit PL logic
		for(int bit=0; bit<32; bit++) {
			if(1<<bit & cgi.groupDigOut) {
				// FIXME:
				cgi.cond_operands;
				int smBit0 = 0;	// FIXME: get
				int smBit1 = 0;
				std::string inv;
				std::stringstream rhs;

				switch(cgi.condition) {		// FIXME: cleanup
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
						rhs << "I[" << smBit0 << "]";
						break;

					// 2 operands
					case cond_nand:
						inv = "/";
						// fall through
					case cond_and:
						rhs << "I[" << smBit0 << "] & I[" << smBit1 << "]";
						break;

					case cond_nor:
						inv = "/";
						// fall through
					case cond_or:
						rhs << "I[" << smBit0 << "] | I[" << smBit1 << "]";
						break;

					case cond_nxor:
						inv = "/";
						// fall through
					case cond_xor:
						rhs << "I[" << smBit0 << "] ^ I[" << smBit1 << "]";
						break;
				}
				emit(
					slot,
					QL_SS2S(inv << "O[" << bit << "] := " << rhs.str())
				);
			}
		}
	}
}

} // namespace ql
