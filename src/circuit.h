/**
 * @file   circuit.h
 * @date   11/2016
 * @author Nader Khammassi
 * @brief  circuit (i.e. gate container) implementation
 */

#pragma once

#include <vector>

#include "gate.h"

namespace ql {

typedef std::vector<gate*> circuit;

void print(const circuit &c);

/**
 * generate qasm for a given circuit
 */
std::string qasm(const circuit &c);

std::vector<circuit*> split_circuit(circuit &x);

/**
 * detect measurements and qubit preparations
 */
bool contains_measurements(const circuit &x);

} // namespace ql
