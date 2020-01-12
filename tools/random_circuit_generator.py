#pragma once
#include "circuit.h"
#include "gate.h"
#include <vector>
#include <random>



# class random_circuit_generator:
# 	std::default_random_engine generator
# 	std::uniform_real_distribution<double> uniform
# 	std::normal_distribution<> gate_load_dist

# 	def generate_random_gate()
# 		return #TODO

# 	random_circuit_generator( Nqubits,  depth, double gate_load, double two_qubit_fraction,  std_dev) : uniform(0.0, 1.0), gate_load_dist(gate_load, std_dev)


# 	ql::circuit generate_random_circuit( Nqubits,  depth, double gate_load, double two_qubit_fraction,  std_dev)
# 		ql::circuit c
# 		std::vector<bool> busy(Nqubits, false)
		
# 		for( cycle = 0 cycle < depth cycle++)
# 			 sampled = Nqubits+1
# 			while (sampled > Nqubits and sampled < 0)
# 				sampled = gate_load_dist(generator)

# 		for( qubit = 0 qubit < Nqubits qubit++)
	
# 				bool has_gate = uniform(generator)
# 				if (has_gate)
# 					stuff //TODO

#%%			
from openql import openql as ql
import random

probs = {}
#%%
def random_circuit(qubits, gate_load, depth, two_qubit_fraction, seed = None):
	"""
	Args:
		qubits: Number of circuit qubits.
		gate_load: The fraction of busy cycles overall.
		gate_domain: The set of gates to choose from, with a specified arity.
		two_qubit_fraction: Fraction of the gate_load that corresponds to two_qubit gates. (Note that a two qubit gate introduces 4 times the load of a single qubit gate, since they require double the qubits for double the amount of time.)

	Raises:
		ValueError:
			* gate_load is not in (0, 1).
			* gate_domain is empty.
			* qubits is an int less than 1 or an empty sequence.
		
	Returns:
		The randomly generated Circuit.
	"""

	random.seed(a=seed)

	if not 0 < gate_load < 1:
		raise ValueError('gate_load must be in (0, 1).')
	# if gate_domain is None:
	#     gate_domain = DEFAULT_GATE_DOMAIN
	# if not gate_domain:
	#     raise ValueError('gate_domain must be non-empty')
	# max_arity = max(gate_domain.values())

	if isinstance(qubits, int):
		if qubits < 1:
			raise ValueError('qubits must be a >=1 integer.')
	else:
		raise ValueError('qubits must be a >=1 integer.')

	#TODO save kernel with proper name?
	platform  = ql.Platform('test_platform', "test_files/test_mapper17.json")
	k = ql.Kernel('test_kernel', platform, qubits)

	gates_1qb = ['x','x45','x90','xm45','xm90','y','y45','y90','ym45','ym90']
	gates_2qb = ['cz']
	# probs = {}


	# prob_1qb_gate = gate_load *
	# prob_2qb_gate = gate_load * 
	# prob_idle = 1 - gate_load 

	cycle_free = list(range(qubits))
	next_cycle_free = list(range(qubits))
	for cycle in list(range(depth):)

		cycle_free = next_cycle_free
		next_cycle_free = list(range(qubits))

		free_qubits = len(cycle_free)
		dice = random.random()
		if free_qubits >= 2: #it might be faster to just do a shuffle on the free qubit list and then just pop the qubits (choosing them and removing from the free_list)
			if dice < two_qubit_fraction*gate_load:
				operands = random.sample(cycle_free, 2)
				k.gate('cz', [operands[0], operands[1]])
				cycle_free.remove(operands[0])
				cycle_free.remove(operands[1])
				next_cycle_free.remove(operands[0])
				next_cycle_free.remove(operands[1])
			elif two_qubit_fraction*gate_load <= dice < gate_load:
				gate = random.choice(gates_1qb)
				operand = random.choice(cycle_free)
				k.gate(gate, [operand])
				cycle_free.remove(operand)
			else: #idling case: no gate applied, we just mark the qubit as busy
				operand = random.choice(cycle_free)
				cycle_free.remove(operand)
		else:
			if dice <= gate_load:
				gate = random.choice(gates_1qb)
				operand = random.choice(cycle_free)
				k.gate(gate, [operand])
				cycle_free.remove(operand)
			else: #idling case: no gate applied, we just mark the qubit as busy
				operand = random.choice(cycle_free)
				cycle_free.remove(operand)

	return k


#%%
circ = random_circuit(4,0.5, 5, 0)

#%%	
save_random_circuit(qubits, gate_load)