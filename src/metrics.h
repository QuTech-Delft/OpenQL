/**
 * @file   metrics.h
 * @date   04/2019 - now
 * @author Diogo Valada
 * @brief  OpenQl circuit fidelity estimator
 */

//TODO: ATM, this does not support operations after measurement (eg. preparing again and reusing a qubit)
//The above will produce undefined behaviour

#pragma once

#include <string>
#include <list>
#include "platform.h"
#include "circuit.h"

namespace ql {

class Metrics {
private:
	size_t Nqubits;
	double gatefid_1 = 0.999; //Hardcoded for testing purposes
	double gatefid_2 = 0.99; //Hardcoded for testing purposes
	double decoherence_time = 4500.0/20; //Hardcoded for testing purposes
	std::string fidelity_estimator;
	std::string output_mode;
    utils::json qubit_attributes;

	static double gaussian_pdf(double x, double mean, double sigma);

public:

	//double (Metrics::*compute_score)(ql::circuit &, std::vector<double> &  ); //TODO FIX THIS

	//EVERYTHING SHOULD BE IN CYCLES (gate duration, decoherence time, etc)
	// Metrics( /*double gatefid_1, double gatefid_2, double decoherence_time */)
	// {
		// fidelity_estimator = ql::options::get("metrics_fidelity_estimator");
		// output_mode = ql::options::get("metrics_output_mode");

		// if (fidelity_estimator == "bounded_fidelity")
		// 	compute_score = &bounded_fidelity;
		// // else if (fidelity_estimator == "depolarizing")
		// // 	compute_score = & depolarizing;
		// else
    	// 	EOUT("Invalid metrics_fidelity_estimator provided: " << fidelity_estimator);
    	// 	throw ql::exception("invalid metrics_fidelity_estimator", false);

		// if (output_mode != "worst" || output_mode != "gaussian")
		// {
		// 	EOUT("Invalid metrics_output_method provided: " << output_mode);
    	// 	throw ql::exception("invalid metrics_output_mode", false);
		// }
	// };

	Metrics(
	    size_t Nqubits,
	    double gatefid_1 = 0.999,
	    double gatefid_2 = 0.99,
	    double decoherence_time = 3000/20,
	    const std::string &estimator = "bounded_fidelity",
	    const std::string &output_mode = "average"
    );

	void Init(size_t Nqubits, ql::quantum_platform *platform);
	double create_output(const std::vector<double> &fids);
	double bounded_fidelity(const ql::circuit &circ, std::vector<double> &fids);

};

double quick_fidelity(const std::list<ql::gate*> &gate_list);
double quick_fidelity_circuit(const ql::circuit &circuit);
double quick_fidelity(const ql::circuit &circuit);

} // namespace ql
