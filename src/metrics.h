/** \file
 * OpenQl circuit fidelity estimator.
 */

//TODO: ATM, this does not support operations after measurement (eg. preparing again and reusing a qubit)
//The above will produce undefined behaviour

#pragma once

#include "utils/num.h"
#include "utils/str.h"
#include "utils/vec.h"
#include "utils/list.h"
#include "platform.h"
#include "circuit.h"

namespace ql {

class Metrics {
private:
	utils::UInt Nqubits;
	utils::Real gatefid_1 = 0.999; //Hardcoded for testing purposes
	utils::Real gatefid_2 = 0.99; //Hardcoded for testing purposes
	utils::Real decoherence_time = 4500.0 / 20; //Hardcoded for testing purposes
	utils::Str fidelity_estimator;
	utils::Str output_mode;
    utils::Json qubit_attributes;

	static utils::Real gaussian_pdf(utils::Real x, utils::Real mean, utils::Real sigma);

public:

	//utils::Double (Metrics::*compute_score)(circuit &, utils::Vec<utils::Double> &  ); //TODO FIX THIS

	//EVERYTHING SHOULD BE IN CYCLES (gate duration, decoherence time, etc)
	// Metrics( /*utils::Double gatefid_1, utils::Double gatefid_2, utils::Double decoherence_time */)
	// {
		// fidelity_estimator = options::get("metrics_fidelity_estimator");
		// output_mode = options::get("metrics_output_mode");

		// if (fidelity_estimator == "bounded_fidelity")
		// 	compute_score = &bounded_fidelity;
		// // else if (fidelity_estimator == "depolarizing")
		// // 	compute_score = & depolarizing;
		// else
    	// 	EOUT("Invalid metrics_fidelity_estimator provided: " << fidelity_estimator);
    	// 	throw exception("invalid metrics_fidelity_estimator", false);

		// if (output_mode != "worst" || output_mode != "gaussian")
		// {
		// 	EOUT("Invalid metrics_output_method provided: " << output_mode);
    	// 	throw exception("invalid metrics_output_mode", false);
		// }
	// };

	Metrics(
	    utils::UInt Nqubits,
	    utils::Real gatefid_1 = 0.999,
	    utils::Real gatefid_2 = 0.99,
	    utils::Real decoherence_time = 3000 / 20,
	    const utils::Str &estimator = "bounded_fidelity",
	    const utils::Str &output_mode = "average"
    );

	void Init(utils::UInt Nqubits, quantum_platform *platform);
	utils::Real create_output(const utils::Vec<utils::Real> &fids);
	utils::Real bounded_fidelity(const circuit &circ, utils::Vec<utils::Real> &fids);

};

utils::Real quick_fidelity(const utils::List<gate*> &gate_list);
utils::Real quick_fidelity_circuit(const circuit &circuit);
utils::Real quick_fidelity(const circuit &circuit);

} // namespace ql
