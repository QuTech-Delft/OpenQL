/**
 * @file   metrics.h
 * @date   04/2019 - now
 * @author Diogo Valada
 * @brief  OpenQl circuit fidelity estimator
 */

//TODO: ATM, this does not support operations after measurement (eg. preparing again and reusing a qubit)
//The above will produce undefined behaviour

#ifndef METRICS_H
#define METRICS_H

#include <cmath> 
#include <vector>
#include <map>
#include <random>
#include <chrono>
#include "gate.h"
#include "utils.h" 
#include "options.h"
#include "platform.h"
//#include "mapper.h"



#define CYCLE_TIME 20 //TODO: gate duration is hardcoded for now

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

	Metrics(size_t Nqubits, double gatefid_1 = 0.999, double gatefid_2 = 0.99, double decoherence_time = 3000/20, std::string estimator = "bounded_fidelity", std::string output_mode = "average" )
	{
		// fidelity_estimator = ql::options::get("metrics_fidelity_estimator");
		// output_mode = ql::options::get("metrics_output_mode");
		this->Nqubits = Nqubits;
		this->output_mode=output_mode;

		// if (fidelity_estimator == "bounded_fidelity")
		// 	compute_score = bounded_fidelity;
		// else if (fidelity_estimator == "depolarizing")
		//  	compute_score = & depolarizing;
		// else		

		// EOUT("Invalid metrics_fidelity_estimator provided: " << fidelity_estimator);
		// throw ql::exception("invalid metrics_fidelity_estimator", false);

		if (output_mode != "worst" && output_mode != "gaussian" && output_mode != "average")
		{
			EOUT("Invalid metrics_output_method provided: " << output_mode);
			throw ql::exception("invalid metrics_output_mode", false);
		}

		this->gatefid_1 = gatefid_1;
		this->gatefid_2 = gatefid_2;
		this->decoherence_time = decoherence_time;
	}

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


	double create_output (const std::vector<double> &fids)
	{
		IOUT("Creating output");
		std::vector<double> result_vector;
		result_vector = fids;
		IOUT("Creating output2");

		PRINTER(result_vector);
		IOUT("Creating output2.5");
		//We take out negative fidelities
		// size_t dimension = result_vector.size();
		// for (size_t element = dimension-1; element >=0 ; element--)
		// {
		// 	if (result_vector.at(element) <= 0)
		// 		result_vector.erase(result_vector.begin() + element);
		// PRINTER(result_vector);
		// }

		IOUT("Creating output3.5");
		PRINTER(result_vector);
		IOUT("Creating output4");
		if (output_mode == "worst"){
			IOUT("\nOutput mode: worst");
			return *std::min_element(fids.begin(),fids.end());
		}

		else if (output_mode == "average") //DOES NOT WORK
		{
			PRINTER(fids);
			// double sum= std::accumulate(fids.begin(), fids.end(), 0);
			double sum = 0;
			for (auto x : fids)
			{
				sum += x;
			}
			DOUT("Sum fidelities :" + std::to_string(sum));
			double average = sum / fids.size();
			DOUT("Average fidelity:" + std::to_string(average));
			return average;			
		}

		// else if (output_mode == "gaussian") //DOES NOT WORK
		// {
		// 	IOUT("\nOutput mode: gaussian");
 		// 	double min = *std::min_element(fids.begin(),fids.end());
		// 	double sigma = (1.0 - min)/2;
		// 	IOUT("\nOutput mode: gaussian2");
		// 	double sum = 0;
		// 	for (auto x : fids)
		// 	{
		// 		IOUT("\nOutput mode: gaussian3");

		// 		sum += x*gaussian_pdf(x, min, sigma); //weight the fidelities
		// 	} 
		// 	IOUT("\nOutput mode: gaussian4");
		// 	return 2*sum; // *2 to normalize (we use half gaussian). divide by Nqubits?
		// }
		else
		{
			IOUT("\nOutput mode: error");			
			return 500;
		}
		
	} 
	
	
	// double bounded_fidelity(const ql::circuit& circ, std::vector<double> &fids)
	double bounded_fidelity(const ql::circuit& circ, std::vector<double> &fids)
	{ 
		//this function considers the primitive gates! each operand undergoing a 2-qubit operation is always considered to have the same latency
		//same end fidelity considered for the two operands of the same 2-qubit gate
		//TODO - URGENT!! Check if gate->cycle starts in zero;
		//TODO - URGENT!! do not consider the fidelity of non-used qubits (nqubits < qubits from architecture) (set to 2/-1?)
		//TODO - URGENT!! do not consider the fidelity of used but non initialized qubits (set to 2/-1?)

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

		IOUT("Entered loop");
		for (auto &gate : circ)
		{

			IOUT("Next gate\n");

			if (gate->name == "measure")
				continue;
			else if (gate->name == "prepz")
			{
				size_t qubit = gate->operands[0]; 
				fids[qubit] = 1.0;
				last_op_endtime[qubit] = gate->cycle + gate->duration / CYCLE_TIME;
				continue;
			}
			
			if (gate->duration > CYCLE_TIME*2 && gate->name!="prep_z" && gate->name!="measure" )
			{
				EOUT("Gate with duration larger than CYCLE_TIME*20 detected! Non primitive?: " << gate->name );
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
				IOUT("Idled time q_t:" + std::to_string(idled_time_t) + " gate cycle=" + std::to_string(gate->cycle) + ". last_time_t=" + std::to_string(last_time_t));

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

		//Now we should still add decoherence effect in case the last gate was a two-qubit gate (the other qubits still decohere in the meantime!)



		IOUT(" \n\n THE END \n\n ");
		IOUT("Fidelity after idlying: ");
		PRINTER(fids);
		//Concatenating data into a single value, to serve as metric
		return create_output(fids);
	}

}; //class end


	static double quick_fidelity(std::list< ql::gate * > gate_list )
	{
		ql::Metrics estimator(17);
		std::vector<double> previous_fids;
		ql::circuit circuit;
		std::copy(std::begin(gate_list), std::end(gate_list), std::back_inserter(circuit));
		double fidelity = estimator.bounded_fidelity(circuit, previous_fids);
		fidelity =- fidelity; //Symmetric value because lower score is considered better in mapper.h
		return fidelity;
	}

    static double quick_fidelity_circuit(ql::circuit circuit )
	{
		ql::Metrics estimator(17);
		std::vector<double> previous_fids;
		double fidelity = estimator.bounded_fidelity(circuit, previous_fids);
		fidelity =- fidelity; //Symmetric value because lower score is considered better in mapper.h
		return fidelity;
	}

    static double quick_fidelity(ql::circuit circuit )
	{
		ql::Metrics estimator(17);
		std::vector<double> previous_fids;
		double fidelity = estimator.bounded_fidelity(circuit, previous_fids);
		fidelity =- fidelity; //Symmetric value because lower score is considered better in mapper.h
		return fidelity;
	}


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
// 		std::map<std::string, double> error_map; //the string contains the error status of each Qubit. Ex: IIXYXXI for a qubit set with 7 qubits 
// 	public:
// 		std::vector<size_t> qubits; //Contains the qubits ordered as they would appear in a error string
// 		static double new_error_threshold;

// 		QubitSet( std::vector<size_t> qubits )
// 		{
// 			// error_map.reserve( qubits.size() );
// 			this->qubits = qubits;
// 			reset();
// 		}

// 		QubitSet( size_t qubit )
// 		{
// 			// error_map.reserve( qubits.size() );
// 			this->qubits.push_back(qubit);
// 			reset();
// 		}

// 		//Constructor for merging sets, doesn't use reset
// 		QubitSet( QubitSet* qubit_set1, QubitSet* qubit_set2 )
// 		{

// 			// for (auto& error : qubit_set2->error_map) //Wrong, we need to do the cross product
//   			// 	error_map.insert(std::move(error));
// 			for (auto & error1 : qubit_set1->error_map)
// 			{
// 				for (auto & error2 : qubit_set2->error_map )
// 				{
// 					double joint_probability = error1.second * error2.second;
// 					if ( joint_probability > 0.01)
// 						error_map[error1.first + error2.first] = joint_probability; 
// 				}
// 			}
// 			// error_map.push_back(std::move(qubit_set2->error_map));
			
// 			this->qubits = qubit_set1->qubits;
// 			std::move(qubit_set2->qubits.begin(), qubit_set2->qubits.end(), std::back_inserter(this->qubits));
		

// 		}

// 		void reset()
// 		{
// 			std::string initial_state(qubits.size(), 0);
// 			error_map[initial_state] = 1.0;
// 		}


// 		void remove_qubit(size_t qubit)
// 		{
// 			size_t q_index = get_qubit_index(qubit)
			
// 			for (auto & error : error_map)
// 			{
// 				//Since map is ordered by string, this can probably be optimized, by checking the next 3 keys and verifying if they are variations
// 				//^ This would probably be faster than map::find()
// 				std::string error_string = error.first; 
// 				unsigned char original = error.first.at(q_index);
// 				unsigned char variations[3];

// 				if (original == 0)
// 					variations = {1,2,3};
// 				else if (original == 1)
// 					variations = {0,2,3};
// 				else if (original == 2)
// 					variations = {0,1,3};
// 				else if (original == 3)
// 					variations = {0,1,2};

// 				for (auto variant : variations)
// 				{
// 					error_string.at(q_index) = variant;
// 					auto it = error_map.find(error_string);
					
// 					if ( it != error_map.end() )
// 					{
// 						//If variation is found, its probability is summed to original string, and it is deleted
// 						error.second += it->second; 
// 						error_map.erase(it);
// 					}

// 				}
// 				//The probabilities are now funneled onto this error.
// 				//Now we erase the qubit from this string.
// 				error.first.erase(q_index, 1);
// 			}
// 			//Finally, we remove the qubit from the qubit_list
// 			qubits.erase(qubits.begin() + q_index);
// 		}


// 	// void introduce_wait_cycles(size_t cycles, size_t qubit)
// 	// {
// 	// 	for (int i = 0 ; i<cycles ; i++)
// 	// 	{
// 	// 		single_qubit_gate(qubit, fid_dec);
// 	// 	}
// 	// }

// 		size_t get_qubit_index(size_t qubit)
// 		{

// 			std::vector<size_t>::iterator it = std::find(qubits.begin(), qubits.end(), 22);

// 			if (it == qubits.end())
// 			{
// 				DOUT("Qubit not found in expected QubitSet \n");
// 				throw ql::exception("Qubit not found in expected QubitSet", false);
// 			}

// 			return std::distance(qubits.begin(), it);
// 		}

// 		void single_qubit_gate(size_t qubit, double fid, const std::string &name)
// 		{
// 			//TODO: Transform previous errors first
// 			introduce_single_qubit_error(qubit, fid); 
// 		}

// 		void two_qubit_gate(size_t qubit, double fid, const std::string &name, int control_target)
// 		{
// 			//TODO: Transform previous errors first
// 			introduce_single_qubit_error(qubit, fid); 
// 		}

// 		void introduce_single_qubit_error(size_t qubit, double fid)
// 		{
// 			double axis_flip_prob = (1-fid)/3;
// 			std::map< std::string, double> new_error_map;
			
// 			for (auto & error : error_map)
// 			{

// 				char current_state = error.first.at( get_qubit_index(qubit) ); //Gets error status for qubit at a error string. Replace .at() for [] for optimization
				

// 				// new_error_map.insert(std::pair<std::string, double>(error, error.second * fid)); //Inserts the new states for a 'I' injected error
// 				// use pointers and the new char instead of the string?
				
// 				new_error_map.insert(std::pair<std::string, double>(error.first, error.second * fid));

// 				for (auto new_error = 1; new_error < 4; new_error++)
// 				{
// 					unsigned char new_state = new_state_calc(current_state, new_error);
// 					double new_state_prob = error.second * axis_flip_prob;

// 					std::string new_error_string(error.first);
// 					new_error_string.at( get_qubit_index(qubit) ) = new_state; //string with the new error syndrome, for each case
					
// 					if (new_state_prob > new_error_threshold )
// 					{
// 						auto ret = new_error_map.insert(std::pair<std::string, double>(new_error_string, new_state_prob)); //only adds if error syndrome not on the list
// 						if (ret.second == false) //If a key already existed, then
// 							error_map[new_error_string] += new_state_prob; 
// 					}
// 				}

// 			}
// 			error_map = new_error_map;
// 		}



// };

// class Depolarizing_model
// {
// 	private:
// 		std::vector< QubitSet * > qubit_partition;
// 		double gatefid1;
// 		double gatefid2;
// 		size_t Nqubits;

// 	public:
// 		Depolarizing_model(size_t Nqubits, double gatefid1, double gatefid2, double new_error_threshold): gatefid1(gatefid1), gatefid2(gatefid2)
// 		{
// 			qubit_partition.reserve(Nqubits);
// 			QubitSet::new_error_threshold = new_error_threshold; 

// 			for (size_t i = 0; i<Nqubits; i++)
// 			{
// 				std::vector<size_t> initial_qubit (1, (size_t)1);
// 				qubit_partition.push_back(new QubitSet(  initial_qubit ));
// 			}
// 			this->Nqubits = Nqubits;
// 		}

// 		void merge_sets(size_t qubit1, size_t qubit2)
// 		{
// 			QubitSet* qubit_set1 = qubit_partition.at(qubit1);
// 			QubitSet* qubit_set2 = qubit_partition.at(qubit2);

// 			QubitSet* new_set = new QubitSet(qubit_set1, qubit_set2);

// 			delete qubit_set1; //We destroy the old qubit sets
// 			delete qubit_set2;

// 			for (auto & qubit : new_set->qubits)
// 			{
// 				qubit_partition[qubit] = new_set; //Change the listing in the qubits partition
// 			}
// 		}

// 		void split_sets(size_t qubit1)
// 		{
// 			QubitSet* qubit_set1 = qubit_partition.at(qubit1);

			
// 			if (qubit_set1->qubits.size() == 1 && qubit_set1->qubits.at(0) != qubit_1)
// 				DOUT("Something seriously wrong happened: splitting qubit from the wrong, trivial set")
// 			else if (qubit_set1->qubits.size() == 1)
// 			{
// 				DOUT("This shouldn't happen: Spliting qubit from trivial set")
// 				return; //Just a safety, no sense moving a qubit to a new set when it is alone already...
// 			}

// 			QubitSet* new_set = new QubitSet(qubit1);

// 			qubit_partition[qubit_1] = new_set; //Change the listing in the qubits partition
// 			qubit_set1->remove_qubit(qubit1);

// 			return;
// 		}

// 		double run_circuit(ql::circuit &circ)
// { 
// 		//this function considers the primitive gates! each operand undergoing a 2-qubit operation is always considered to have the same latency
// 		//same end fidelity considered for the two operands of the same 2-qubit gate
// 		//TODO - URGENT!! Check if gate->cycle starts in zero;
// 		//TODO - URGENT!! do not consider the fidelity of non-used qubits (set to 2/-1?)

// 		// if (fids.size() == 0)
// 		// {
// 		// 	IOUT("EMPTY VECTOR - Initializing. Nqubits = " + std::to_string(Nqubits));
// 		// 	fids.resize(Nqubits, 1.0); //Initiallize a fidelity vector, if one is not provided
// 		// 	//TODO: non initialized qubits should have undefined fidelity. It shouldn't be taken into account.
// 		// }

// 		std::vector<size_t> last_op_endtime(Nqubits, 1); //First cycle has index 1

// 		PRINTER(last_op_endtime);
// 		IOUT("\n\n");

// 		for (auto &gate : circ)
// 		{

// 			if (gate->name == "measure")
// 				continue;
// 			else if (gate->name == "prep_z")
// 			{
// 				size_t qubit = gate->operands[0]; 
// 				// fids[qubit] = 1.0;
// 				split_qubits(qubit);
// 				last_op_endtime[qubit] = gate->cycle + gate->duration / CYCLE_TIME;
// 				continue;
// 			}
			
// 			if (gate->duration > CYCLE_TIME*2 && gate->name!="prep_z" && gate->name!="measure" )
// 			{
// 				EOUT("Gate with duration larger than CYCLE_TIME*20 detected! Non primitive?: " << output_mode);
//     			throw ql::exception("Check for non primitive gates at cycle "  + std::to_string(gate->cycle) + "!", false);
// 			}

			
// 			unsigned char type_op = gate->operands.size(); // type of operation (1-qubit/2-qubit)
// 			if (type_op == 1)
// 			{
// 				size_t qubit = gate->operands[0];
// 				size_t last_time = last_op_endtime[qubit];
// 				IOUT("Gate " + gate->name + "("+ std::to_string(gate->operands[0]) +") at cycle " + std::to_string(gate->cycle) + " with duration " + std::to_string(gate->duration));
// 				size_t idled_time = gate->cycle - last_time; //get idlying time to introduce decoherence. This assumes "cycle" starts at zero, otherwise gate->cycle-> (gate->cycle - 1)
				
// 				last_op_endtime[qubit] = gate->cycle  + gate->duration / CYCLE_TIME; //This assumes "cycle" starts at zero, otherwise gate->cycle-> (gate->cycle - 1)
				
// 				IOUT("Idled time:" + std::to_string(idled_time));


// 				// fids[qubit] *= std::exp(-((double)idled_time)/decoherence_time); // Update fidelity with idling-caused decoherence
// 				// fids[qubit] *= gatefid_1; //Update fidelity after gate
				
// 				//TODO: OPTIMIZE THE DECOHERENCE PART?
// 				for (auto i = 0; i < idled_time; i++)
// 				{
// 					qubit_partition[qubit]->introduce_single_qubit_error(qubit, std::exp(-(double)1.0/decoherence_time)); //Introduces wait gates
// 				} 
// 				qubit_partition[qubit]->single_qubit_gate(qubit, gatefid_1, gate->name); //Introduces gate fidelity
				
				
				
				
// 				IOUT("METRICS - one qubit gate - END");

// 			}
// 			else if (type_op == 2)
// 			{
// 				IOUT("METRICS - TWO qubit gate");
// 				size_t qubit_c = gate->operands[0];
// 				size_t qubit_t = gate->operands[1];
				
// 				size_t last_time_c = last_op_endtime[qubit_c];
// 				size_t last_time_t = last_op_endtime[qubit_t];
// 				size_t idled_time_c = gate->cycle - last_time_c;
// 				size_t idled_time_t = gate->cycle - last_time_t; //get idlying time to introduce decoherence. This assumes "cycle" starts at zero, otherwise gate->cycle-> (gate->cycle - 1)
// 				last_op_endtime[qubit_c] = gate->cycle  + gate->duration / CYCLE_TIME; //This assumes "cycle" starts at zero, otherwise gate->cycle-> (gate->cycle - 1)
// 				last_op_endtime[qubit_t] = gate->cycle  + gate->duration / CYCLE_TIME ; //This assumes "cycle" starts at zero, otherwise gate->cycle-> (gate->cycle - 1)
				
// 				IOUT("Gate " + gate->name + "("+ std::to_string(gate->operands[0]) + ", " + std::to_string(gate->operands[1]) +") at cycle " + std::to_string(gate->cycle) + " with duration " + std::to_string(gate->duration));
// 				IOUT("Idled time q_c:" + std::to_string(idled_time_c));
// 				IOUT("Idled time q_t:" + std::to_string(idled_time_t));
// 				IOUT("Decoherence time: " + std::to_string(decoherence_time));

// 				// fids[qubit_c] *= std::exp(-(double) idled_time_c/decoherence_time); // Update fidelity with idling-caused decoherence
// 				// fids[qubit_t] *= std::exp(-(double)idled_time_t/decoherence_time); // Update fidelity with idling-caused decoherence

// 				for (auto i = 0; i < idled_time_c; i++)
// 					qubit_partition[qubit_c]->introduce_single_qubit_error(qubit_c, std::exp(-(double)1.0/decoherence_time)); //Introduces wait gates
// 				for (auto i = 0; i < idled_time_t; i++)
// 					qubit_partition[qubit_t]->introduce_single_qubit_error(qubit_t, std::exp(-(double)1.0/decoherence_time)); //Introduces wait gates

// 				// IOUT("Error status after idlying: ");
// 				// PRINTER(fids);

// 				qubit_partition[qubit_c]->two_qubit_gate(qubit_c, gatefid_2, gate->name, 0); //Introduces gate fidelity
// 				qubit_partition[qubit_t]->two_qubit_gate(qubit_t, gatefid_2, gate->name, 1); //Introduces gate fidelity


// 				// fids[qubit_c] *=  fids[qubit_t] * gatefid_2; //Update fidelity after gate
// 				// fids[qubit_t] = fids[qubit_c];  					//Update fidelity after gate



// 				//TODO - Convert the code into a for loop with range 2, to get the compiler's for optimization (and possible paralellization?)
// 			}
// 				// PRINTER(fids);
// 				// PRINTER(last_op_endtime);
				
// 				IOUT("\n NEXT GATE");

// 		}
// 		size_t end_cycle = circ.back()->cycle + circ.back()->duration/CYCLE_TIME; 
// 		for (auto qubit=0; qubit < Nqubits; qubit++ )
// 		{
// 			size_t idled_time_final = end_cycle - last_op_endtime[i];
// 			for(auto wait=0; wait < idled_time_final; wait++)
// 				qubit_partition[qubit]->introduce_single_qubit_error(qubit, std::exp(-(double)1.0/decoherence_time)); //Introduces wait gates
				
// 		}

// 		IOUT(" \n\n THE END \n\n ");
// 		IOUT("Fidelity after idlying: ");
// 		PRINTER(fids);
// 		//Concatenating data into a single value, to serve as metric
// 		return create_output(fids);
// 	};

// 	void print_status()
// 	{
// 		for(auto q_set in qubit_partition)
// 		{
// 			DOUT("\n\n")
// 			DOUT("QUBIT_SET: ");
// 			std::string output;
// 			for(auto qubit : q_set->qubits)
// 				output += std::to_string(qubit) + ",";
// 			DOUT(output)
// 			output="";
// 			for(auto error : q_set->error_map)
// 				output += error.first + std::to_string(error.second) + "\n";
// 			DOUT(output)

// 		} 
// 	}


// };
















// } //metrics namespace end
} //ql namespace end
#endif // METRICS_H
