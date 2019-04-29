/**
 * @file   metrics.h
 * @date   04/2019 - now
 * @author Diogo Valada
 * @brief  OpenQl circuit fidelity estimator
 */

#pragma once
#include <cmath> 
#include <vector>
#include "gate.h"
#include "ql/utils.h"
#include "ql/options.h"
#include "ql/platform.h"

namespace ql
{
namespace metrics
{
class Metrics {

private:
	size_t Nqubits;
	double gatefid_1 = 0.001; //Hardcoded for testing purposes
	double gatefid_2 = 0.01; //Hardcoded for testing purposes
	double decoherence_time = 4500.0/20; //Hardcoded for testing purposes
	std::string fidelity_estimator;
	std::string output_mode;
	// ql::quantum_platform* platform;
	json qubit_attributes;


	double gaussian_pdf(double x, double mean, double sigma)
	{
		/*constexpr*/ double my_pi = 3.14159265358979323846;
		return (1.0/(sigma*std::sqrt(2 * my_pi)))*std::exp(-0.5*(x-mean)/sigma*(x-mean)/sigma);
	}


public:

	//double (Metrics::*compute_score)(ql::circuit &, std::vector<double> &  ); //TODO FIX THIS

	//EVERYTHING SHOULD BE IN CYCLES (gate duration, decoherence time, etc)
	Metrics( /*double gatefid_1, double gatefid_2, double decoherence_time */)
	{
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
		


	};

	Metrics( double gatefid_1, double gatefid_2, double decoherence_time, std::string estimator = "bounded_fidelity", std::string output_mode = "worst" )
	{
		fidelity_estimator = ql::options::get("metrics_fidelity_estimator");
		output_mode = ql::options::get("metrics_output_mode");

		// if (fidelity_estimator == "bounded_fidelity")
		// 	compute_score = bounded_fidelity;
		// else if (fidelity_estimator == "depolarizing")
		//  	compute_score = & depolarizing;
		// else		

		EOUT("Invalid metrics_fidelity_estimator provided: " << fidelity_estimator);
		throw ql::exception("invalid metrics_fidelity_estimator", false);

		if (output_mode != "worst" || output_mode != "gaussian")
		{
			EOUT("Invalid metrics_output_method provided: " << output_mode);
			throw ql::exception("invalid metrics_output_mode", false);
		}

		this->gatefid_1 = gatefid_1;
		this->gatefid_2 = gatefid_2;
		this->decoherence_time = decoherence_time;
	};

	void Init(size_t Nqubits, ql::quantum_platform* platform)
	{
		this->Nqubits = Nqubits;

		//TODO test if json has qubit relaxation times / gate error rate in this function 
		//TODO load from config_file

		// qubit_attributes = platform->qubit_attributes;
		
		// if (qubit_attributes.count("relaxation_times") == 0 || qubit_attributes.count("gate_error_rates") == 0)
		// {
		// 	EOUT("'qubit_attributes' section on hardware config file doesn't contain qubit relaxation times or gate error rates!");
        //     throw ql::exception("[x] error : ql::hardware_configuration::load() : 'relaxation_times' or 'gate_error_rates' sections not specified in the hardware config file !",false);
		// }

		


		

		

	}


	double create_output (std::vector<double> &fids)
	{
		if (output_mode == "worst")
			return *std::min_element(fids.begin(),fids.end());
		else if (output_mode == "gaussian")
		{
 			double min = *std::min_element(fids.begin(),fids.end());
			double sigma = (1.0 - min)/2;

			double sum = 0;
			for (auto x : fids)
			{
				sum += x*gaussian_pdf(x, min, sigma); //weight the fidelities
			} 
			return 2*sum; // *2 to normalize (we use half gaussian). divide by Nqubits?
		}
	} 
	
	
	double bounded_fidelity(ql::circuit& circ, std::vector<double> &fids)
	{ 
		//this function considers the primitive gates! each operand undergoing a 2-qubit operation is always considered to have the same latency
		//same end fidelity considered for the two operands of the same 2-qubit gate

		//TODO - URGENT!!!! Confirm whether gate.duration is defined in cycles or ns!!! Considering cycles currently
		

		if (fids.size() == 0)
		{
			fids.reserve(Nqubits);
			// std::fill_n(fids, Nqubits, 1.0); //Initiallize a fidelity vector, if one is not provided

			for (int i=0; i<fids.size(); i++)
			{
				fids[i]=1.0;
			}
		}

		std::vector<size_t> last_op_endtime(Nqubits, 0); //TODO FIX THIS!!! 

		for (auto gate : circ)
		{

			if (gate->name == "measure")
				continue;
			
			unsigned char type_op = gate->operands.size(); // type of operation (1-qubit/2-qubit)
			if (type_op == 1)
			{
				size_t qubit = gate->operands[0];
				size_t last_time = last_op_endtime[qubit];
				size_t idled_time = gate->cycle - last_time; //get idlying time to introduce decoherence. This assumes "cycle" starts at zero, otherwise gate->cycle-> (gate->cycle - 1)
				last_op_endtime[qubit] = gate->cycle  + gate->duration; //This assumes "cycle" starts at zero, otherwise gate->cycle-> (gate->cycle - 1)
				
				fids[qubit] *= std::exp(-decoherence_time*idled_time); // Update fidelity with idling-caused decoherence
				fids[qubit] *= gatefid_1; //Update fidelity after gate
			}
			else if (type_op == 2)
			{
				size_t qubit_c = gate->operands[0];
				size_t qubit_t = gate->operands[1];
				
				size_t last_time_c = last_op_endtime[qubit_c];
				size_t last_time_t = last_op_endtime[qubit_t];
				size_t idled_time_c = gate->cycle - last_time_c;
				size_t idled_time_t = gate->cycle - last_time_t; //get idlying time to introduce decoherence. This assumes "cycle" starts at zero, otherwise gate->cycle-> (gate->cycle - 1)
				last_op_endtime[qubit_c] = gate->cycle  + gate->duration; //This assumes "cycle" starts at zero, otherwise gate->cycle-> (gate->cycle - 1)
				last_op_endtime[qubit_t] = gate->cycle  + gate->duration; //This assumes "cycle" starts at zero, otherwise gate->cycle-> (gate->cycle - 1)
				
				fids[qubit_c] *= std::exp(-decoherence_time*idled_time_c); // Update fidelity with idling-caused decoherence
				fids[qubit_t] *= std::exp(-decoherence_time*idled_time_t); // Update fidelity with idling-caused decoherence


				fids[qubit_c] *=  fids[qubit_t] * gatefid_2; //Update fidelity after gate
				fids[qubit_t] = fids[qubit_c];  					//Update fidelity after gate

				//TODO - Convert the code into a for loop with range 2, to get the compiler's for optimization (and possible paralellization?)
			}
		}

		//Concatenating data into a single value, to serve as metric
		return create_output(fids);
	};




}; //class end








// double heuristic_metric()
// {
// 	//Maybe try to understand how the fidelity of each operand evolves
// 	//Extra: as a function of the initial and final qubit fidelity
// 	return 0;
// };

class QubitSet
{
	//Each physical qubit should be associated to only one QubitSet
	private:


	public:
	QubitSet()
	{
	}

};

// double depolarizing_model(ql::circuit& circ, size_t Nqubits, double gatefid_1, double gatefid_2, double decoherence_time, std::vector<double> &fids = std::vector<double>(), std::string output_mode="worst")
// {
// 	//Depolarizing model

// };














} //metrics namespace end
} //ql namespace end
