/**
 * @file   metrics.h
 * @date   04/2019 - now
 * @author Diogo Valada
 * @brief  OpenQl circuit fidelity estimator
 */

//TODO: ATM, this does not support operations after measurement (eg. preparing again and reusing a qubit)
//The above will produce undefined behaviour

#pragma once
#include <cmath> 
#include <vector>
#include <map>
#include <unordered_map>
#include <random>
#include <chrono>
#include "gate.h"
#include "ql/utils.h"
#include "ql/options.h"
#include "ql/platform.h"



#define CYCLE_TIME 20
//TEMP: because gate->duration is in ns for now

#include <iostream>
#include <sstream>
#define PRINTER(x) my_print(x, #x)
template <typename T>

void my_print(std::vector<T> const &input, const char *id_name)
{
	std::stringstream output;
	output << id_name << "(" << input.size() << ")= ";
	for (auto const& i: input) {
		output << i << " ";
	}
	// output << "\n";
	IOUT(output.str());
}

namespace ql
{
namespace metrics
{



class Metrics {

private:
	size_t Nqubits;
	double gatefid_1 = 0.999; //Hardcoded for testing purposes
	double gatefid_2 = 0.99; //Hardcoded for testing purposes
	double decoherence_time = 4500.0/20; //Hardcoded for testing purposes
	std::string fidelity_estimator;
	std::string output_mode;
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

	Metrics(size_t Nqubits, double gatefid_1, double gatefid_2, double decoherence_time, std::string estimator = "bounded_fidelity", std::string output_mode = "gaussian" )
	{
		// fidelity_estimator = ql::options::get("metrics_fidelity_estimator");
		// output_mode = ql::options::get("metrics_output_mode");
		this -> Nqubits = Nqubits;
		this->output_mode=output_mode;

		// if (fidelity_estimator == "bounded_fidelity")
		// 	compute_score = bounded_fidelity;
		// else if (fidelity_estimator == "depolarizing")
		//  	compute_score = & depolarizing;
		// else		

		// EOUT("Invalid metrics_fidelity_estimator provided: " << fidelity_estimator);
		// throw ql::exception("invalid metrics_fidelity_estimator", false);

		if (output_mode != "worst" && output_mode != "gaussian")
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
		else
		{
			return 500;
		}
		
	} 
	
	
	double bounded_fidelity(const ql::circuit& circ, std::vector<double> &fids)
	{ 
		//this function considers the primitive gates! each operand undergoing a 2-qubit operation is always considered to have the same latency
		//same end fidelity considered for the two operands of the same 2-qubit gate
		//TODO - URGENT!! Check if gate->cycle starts in zero;
		//TODO - URGENT!! do not consider the fidelity of non-used qubits (set to 2/-1?)

		if (fids.size() == 0)
		{
			IOUT("EMPTY VECTOR - Initializing. Nqubits = " + std::to_string(Nqubits));
			fids.resize(Nqubits, 1.0); //Initiallize a fidelity vector, if one is not provided
			//TODO: non initialized qubits should have undefined fidelity. It shouldn't be taken into account.
		}

		std::vector<size_t> last_op_endtime(Nqubits, 1); //First cycle has index 1

		PRINTER(fids);
		PRINTER(last_op_endtime);
		IOUT("\n\n");

		for (auto &gate : circ)
		{

			if (gate->name == "measure")
				continue;
			else if (gate->name == "prep_z")
			{
				size_t qubit = gate->operands[0]; 
				fids[qubit] = 1.0;
				last_op_endtime[qubit] = gate->cycle + gate->duration / CYCLE_TIME;
				continue;
			}
			
			if (gate->duration > CYCLE_TIME*2 && gate->name!="prep_z" && gate->name!="measure" )
			{
				EOUT("Gate with duration larger than CYCLE_TIME*20 detected! Non primitive?: " << output_mode);
    			throw ql::exception("Check for non primitive gates at cycle "  + std::to_string(gate->cycle) + "!", false);
			}

			
			unsigned char type_op = gate->operands.size(); // type of operation (1-qubit/2-qubit)
			if (type_op == 1)
			{
				size_t qubit = gate->operands[0];
				size_t last_time = last_op_endtime[qubit];
				IOUT("Gate " + gate->name + "("+ std::to_string(gate->operands[0]) +") at cycle " + std::to_string(gate->cycle) + " with duration " + std::to_string(gate->duration));
				size_t idled_time = gate->cycle - last_time; //get idlying time to introduce decoherence. This assumes "cycle" starts at zero, otherwise gate->cycle-> (gate->cycle - 1)
				
				last_op_endtime[qubit] = gate->cycle  + gate->duration / CYCLE_TIME; //This assumes "cycle" starts at zero, otherwise gate->cycle-> (gate->cycle - 1)
				
				IOUT("Idled time:" + std::to_string(idled_time));


				fids[qubit] *= std::exp(-((double)idled_time)/decoherence_time); // Update fidelity with idling-caused decoherence
				
				fids[qubit] *= gatefid_1; //Update fidelity after gate
				IOUT("METRICS - one qubit gate - END");

			}
			else if (type_op == 2)
			{
				IOUT("METRICS - TWO qubit gate");
				size_t qubit_c = gate->operands[0];
				size_t qubit_t = gate->operands[1];
				
				size_t last_time_c = last_op_endtime[qubit_c];
				size_t last_time_t = last_op_endtime[qubit_t];
				size_t idled_time_c = gate->cycle - last_time_c;
				size_t idled_time_t = gate->cycle - last_time_t; //get idlying time to introduce decoherence. This assumes "cycle" starts at zero, otherwise gate->cycle-> (gate->cycle - 1)
				last_op_endtime[qubit_c] = gate->cycle  + gate->duration / CYCLE_TIME; //This assumes "cycle" starts at zero, otherwise gate->cycle-> (gate->cycle - 1)
				last_op_endtime[qubit_t] = gate->cycle  + gate->duration / CYCLE_TIME ; //This assumes "cycle" starts at zero, otherwise gate->cycle-> (gate->cycle - 1)
				
				IOUT("Gate " + gate->name + "("+ std::to_string(gate->operands[0]) + ", " + std::to_string(gate->operands[1]) +") at cycle " + std::to_string(gate->cycle) + " with duration " + std::to_string(gate->duration));
				IOUT("Idled time q_c:" + std::to_string(idled_time_c));
				IOUT("Idled time q_t:" + std::to_string(idled_time_t));
				IOUT("Decoherence time: " + std::to_string(decoherence_time));

				fids[qubit_c] *= std::exp(-(double) idled_time_c/decoherence_time); // Update fidelity with idling-caused decoherence
				fids[qubit_t] *= std::exp(-(double)idled_time_t/decoherence_time); // Update fidelity with idling-caused decoherence

				IOUT("Fidelity after idlying: ");
				PRINTER(fids);

				fids[qubit_c] *=  fids[qubit_t] * gatefid_2; //Update fidelity after gate
				fids[qubit_t] = fids[qubit_c];  					//Update fidelity after gate

				//TODO - Convert the code into a for loop with range 2, to get the compiler's for optimization (and possible paralellization?)
			}
				PRINTER(fids);
				PRINTER(last_op_endtime);
				
				IOUT("\n NEXT GATE");

		}
		size_t end_cycle = circ.back()->cycle + circ.back()->duration/CYCLE_TIME; 
		for (size_t i=0; i < Nqubits; i++ )
		{
			size_t idled_time_final = end_cycle - last_op_endtime[i];
			fids[i] *= std::exp(-(double) idled_time_final/decoherence_time);
		}



		IOUT(" \n\n THE END \n\n ");
		IOUT("Fidelity after idlying: ");
		PRINTER(fids);
		//Concatenating data into a single value, to serve as metric
		return create_output(fids);
	};

}; //class end


// const unsigned char transition_matrix[4][4]  = {{ 0, 1, 2, 3 },  //[input_state][new_error]
// 					   						    { 1, 0, 3, 2 },  //I = 0, X = 1, Y = 2, Z = 3
// 											    { 2, 3, 0, 1 },
// 											    { 3, 2, 1, 0 }};

// unsigned char new_state_calc(char old_state, char new_error) 
// {
// 	return transition_matrix[old_state][new_error]; 
// }


// class QubitSet
// {
// 	//Each physical qubit should be associated to only one QubitSet
// 	private:
// 		std::unordered_map<std::string, double> error_list; //the string contains the error status of each Qubit. Ex: IIXYXXI for a qubit set with 7 qubits 
// 		//use map instead if all error strings are generated and map pattern is known? (map is also unordered apparently, use vector of std::pairs instead)
// 		std::vector<size_t> qubits; //Contains the qubits ordered as they would appear in a error string

// 	public:
// 	QubitSet( std::vector<size_t> qubits )
// 	{
// 		error_list.reserve( qubits.size() );
// 		this->qubits = qubits;
// 	}


// 	// void introduce_wait_cycles(size_t cycles, size_t qubit)
// 	// {
// 	// 	for (int i = 0 ; i<cycles ; i++)
// 	// 	{
// 	// 		single_qubit_gate(qubit, fid_dec);
// 	// 	}
// 	// }

// 	size_t qubit_index(size_t qubit)
// 	{

// 		std::vector<size_t>::iterator it = std::find(qubits.begin(), qubits.end(), 22);

// 		if (it == qubits.end())
// 		{
// 			DOUT << "Qubit not found in expected QubitSet" << std::endl;
// 			throw ql::exception("Qubit not found in expected QubitSet", false);
// 		}

// 		return std::distance(qubits.begin(), it);
// 	}

// 	void introduce_single_qubit_error(size_t qubit, double fid)
// 	{
// 		double axis_flip_prob = (1-fid)/3;
		
// 		for (auto & error : error_list)
// 		{
// 			char current_state = error.first.at( qubit_index(qubit) ); //Gets error status for qubit at a error string. Replace .at() for [] for optimization


// 			for (auto error_type = 1; error_type < 4; error_type++)
// 			{
// 				unsigned char new_state = new_state_calc(current_state, error_type);
// 				double new_state_prob = error.second * axis_flip_prob;


// 				std::string new_error_syndrome(error.first);
// 				new_error_syndrome.at( qubit_index(qubit) ) = new_state; //string with the new error syndrome, for each case
				
// 				auto ret = error_list.insert(std::pair<std::string, double>(new_error_syndrome, new_prob)); //only adds if error syndrome not on the list
// 				if (ret.second == false) //If a key already existed, then
// 					error_list[new_error_syndrome] += new_state_prob; //This is wrong, I still need the un-updated probability.

		



// 			}
			
// 			//Update error value for the case of an Identity error gate
// 			error.second *= fid;

// 		}
// 	}



// };

// class Depolarizing_model
// {
// 	private:
// 		std::vector< QubitSet * > qubit_partition;
// 		double gatefid1;
// 		double gatefid2;
// 		size_t Nqubits;

// 	public:
// 		Depolarizing_model(size_t Nqubits, double gatefid1, double gatefid2): gatefid1(gatefid1), gatefid2(gatefid2)
// 		{
// 			qubit_partition.reserve(Nqubits);
// 			for (size_t i = 0; i<Nqubits; i++)
// 				qubit_partition.push_back(new QubitSet);
// 			this->Nqubits = Nqubits;
// 		}

// 		// char error_sampler(double fid)
// 		// {
// 		// 	char error;
// 		// 	double sample = dice();
// 		// 	if (sample > fid)
// 		// 		error = 'I';
// 		// 	else if (error > 2*fid/3)
// 		// 		error = 'X';
// 		// 	else if (error > fid/3)
// 		// 		error = 'Y';
// 		// 	else
// 		// 		error = 'Z';
// 		// 	return error;
// 		// }

// 		char introduce_error(double fid)
// 		{
// 			char error;
// 			double sample = dice();
// 			if (sample > fid)
// 				error = 'I';
// 			else if (error > 2*fid/3)
// 				error = 'X';
// 			else if (error > fid/3)
// 				error = 'Y';
// 			else
// 				error = 'Z';
// 			return error;
// 		}

// 		double run_circuit(ql::circuit &circ)
// 		{
// 		}

// };
















} //metrics namespace end
} //ql namespace end
