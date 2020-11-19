/**
 * @file    datapath_cc.h
 * @date    20201119
 * @author  Wouter Vlothuizen (wouter.vlothuizen@tno.nl)
 * @brief   handling of Central Controller datapath (input MUX, Distributed Shared Memory, output PL)
 * @note
 */

#pragma once

#include <string>
#include <cstddef>
#include <iomanip>
#include <sstream>
#include <utils.h>

namespace ql {

class datapath_cc
{
public: // types

public: // functions
    datapath_cc() = default;
    ~datapath_cc() = default;

	void programStart()
	{
		datapathSection << std::left;    // assumed by emit()
		emit(".DATAPATH", "");
	}

	int allocateSmBit(size_t cop, size_t instrIdx)
	{
		static int smBit = 0;

		smBit++;	// FIXME

		return smBit;
	}

	static int getSizeTag(int numReadouts) {
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

	std::string getDatapathSection() { return datapathSection.str(); }


	// FIXME: make private
	void emit(const std::string &sel, const std::string &statement, const std::string &comment="")
	{
		datapathSection << std::setw(16) << sel << std::setw(16) << statement << std::setw(24) << comment << std::endl;
	}

	void comment(const std::string &cmnt) {
	    datapathSection << cmnt << std::endl;
	}

private:    // vars
	std::stringstream datapathSection;                          // the data path configuration generated
}; // class

} // namespace ql
