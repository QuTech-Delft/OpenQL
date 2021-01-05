/**
 * @file    datapath_cc.h
 * @date    20201119
 * @author  Wouter Vlothuizen (wouter.vlothuizen@tno.nl)
 * @brief   handling of Central Controller datapath (input MUX, Distributed Shared Memory, output PL)
 * @note
 */

#pragma once

#include "bundle_info.h"

#include "gate.h"
#include "utils/vec.h"

#include <string>
#include <cstddef>
#include <iomanip>
#include <sstream>
#include <utils/logger.h>

namespace ql {

namespace { using namespace utils; }

// NB: types shared with codegen_cc
typedef struct {
	unsigned int smBit;
	unsigned int bit;
	const BundleInfo *bi;									// used for annotation only
} tFeedbackInfo;											// information for feedback on single instrument group
using tFeedbackMap = std::map<int, tFeedbackInfo>;			// NB: key is instrument group

typedef struct {
	cond_type_t condition;
	Vec<UInt> cond_operands;
	uint32_t groupDigOut;
} tCondGateInfo;											// information for conditional gate on single instrument group
using tCondGateMap = std::map<int, tCondGateInfo>;			// NB: key is instrument group



class datapath_cc
{
public: // types

public: // functions
    datapath_cc() = default;
    ~datapath_cc() = default;

	void programStart();
	void programFinish();

	unsigned int allocateSmBit(size_t breg_operand, size_t instrIdx);
	unsigned int getSmBit(size_t breg_operand, size_t instrIdx);
	unsigned int getOrAssignMux(size_t instrIdx, const tFeedbackMap &feedbackMap);
	unsigned int getOrAssignPl(size_t instrIdx, const tCondGateMap &condGateMap);
	static unsigned int getSizeTag(unsigned int numReadouts);
	void emitMux(unsigned int mux, const tFeedbackMap &feedbackMap, size_t instrIdx, int slot);
	static unsigned int getMuxSmAddr(const tFeedbackMap &feedbackMap);
	unsigned int emitPl(unsigned int pl, const tCondGateMap &condGateMap, size_t instrIdx, int slot);

	std::string getDatapathSection() { return datapathSection.str(); }

	void comment(const std::string &cmnt) {
	    datapathSection << cmnt << std::endl;
	}

private:	// functions
	void emit(const std::string &sel, const std::string &statement, const std::string &comment="") {
		datapathSection << std::setw(16) << sel << std::setw(16) << statement << std::setw(24) << comment << std::endl;
	}
	void emit(int sel, const std::string &statement, const std::string &comment="") {
		emit(QL_SS2S("[" << sel << "]"), statement, comment);
	}

private:    // vars
    static const int MUX_CNT = 512;                            	// number of MUX configurations
    static const int MUX_SM_WIN_SIZE = 16;						// number of MUX bits in single view (currently, using a ZI UHFQA)
    static const int PL_CNT = 512;                            	// number of PL configurations
    static const int PL_SM_WIN_SIZE = 128;						// number of SM bits in single view
    static const int SM_BIT_CNT = 1024;                         // number of SM bits
    static const int MAX_DSM_XFER_SIZE = 16;                    // current max (using a ZI UHFQA)

	std::stringstream datapathSection;                          // the data path configuration generated

	// state for allocateSmBit/getSmBit
	unsigned int lastSmBit = 0;
	size_t smBitLastInstrIdx = 0;
	std::map<size_t, unsigned int> mapBregToSmBit;

	// other state
	unsigned int lastMux[MAX_INSTRS] = {0};
	unsigned int lastPl[MAX_INSTRS] = {0};
}; // class

} // namespace ql
