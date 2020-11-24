/**
 * @file    datapath_cc.cc
 * @date    20201119
 * @author  Wouter Vlothuizen (wouter.vlothuizen@tno.nl)
 * @brief   handling of Central Controller datapath (input MUX, Distributed Shared Memory, output PL)
 * @note
 */

#include "datapath_cc.h"

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


int datapath_cc::allocateSmBit(size_t cop, size_t instrIdx)
{
	static int smBit = 0;

	smBit++;	// FIXME

	return smBit;
}


int datapath_cc::getOrAssignMux(size_t instrIdx)
{
	return 0;	// FIXME
}


int datapath_cc::getOrAssignPl(size_t instrIdx)
{
	return 0;	// FIXME
}


int datapath_cc::getSizeTag(int numReadouts) {
	int sizeTag;

	if(numReadouts == 0) {
		FATAL("inconsistency in number of readouts");	// FIXME: message refers to caller, this assumes particular semantics for calling this function
	} else if(numReadouts <= 8) {
		sizeTag = 0;			// 0=byte
	} else if(numReadouts <= 16) {
		sizeTag = 1;
	} else if(numReadouts <= 32) {	// NB: should currently not occur since we have a maximum of 16 inputs on UHF
		sizeTag = 2;
	} else {
		FATAL("inconsistency detected: too many readouts");
	}
	return sizeTag;
}


} // namespace ql
