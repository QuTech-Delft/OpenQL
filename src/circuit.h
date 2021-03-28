/** \file
 * Circuit (i.e. gate container) implementation.
 */

#pragma once

#include "ql/utils/num.h"
#include "ql/utils/str.h"
#include "ql/utils/vec.h"
#include "gate.h"

namespace ql {

typedef utils::Vec<gate*> circuit;

void print(const circuit &c);

/**
 * generate qasm for a given circuit
 */
utils::Str qasm(const circuit &c);

utils::Vec<circuit*> split_circuit(circuit &x);

/**
 * detect measurements and qubit preparations
 */
utils::Bool contains_measurements(const circuit &x);

} // namespace ql
