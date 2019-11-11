import numpy as np
import os
from quantumsim.sparsedm import SparseDM
import argparse
from quantumsim.circuit import Circuit
from quantumsim.circuit import uniform_noisy_sampler
import quantumsim.sparsedm as sparsedm
import importlib
import glob
import sys



def simulation(circuit):
	# CIRCUIT DECLARATION
	bell_state = SparseDM(circuit.get_qubit_names())
	circuit.apply_to(bell_state)
	measurement_result = bell_state.peak_multiple_measurements(circuit.get_qubit_names())
	return measurement_result


def return_correct_result(circuit):

	measurement_result = simulation(circuit)
	# for measurement, value in measurement_result:
	# 	if round(value, 5) > 0:
	# 		print(measurement, round(value, 5))
	correct_result = max(measurement_result,key=lambda x: x[1])
	
	return correct_result

def compare(circuit_original, circuit_mapped, mapping):
	result_original = return_correct_result(circuit_original)
	result_mapped = return_correct_result(circuit_mapped)

	# measurement_result_rounded = [ (stateAndcount[0], round(stateAndcount[1],5)) for stateAndcount in measurement_result]
	if result_original[1]<0.999 or result_mapped[1]<0.999:
		print("Warning: Non deterministic circuit? P(most likely original)= ", result_original[1], " | P(most likely mapped)= ", result_mapped[1] )

	#TODO SWAP according to mapping
	result_original_updatedmapping_dict = {}
	for qubit, measurement in result_original[0].items():
		result_original_updatedmapping_dict[str(mapping[int(qubit)])] = measurement

	if result_mapped[0] == result_original_updatedmapping_dict:
		return True
	else:
		print(circuit_original.title)
		print("====Circuit original: ")
		print(result_original[0])
		print("====Mapping: ")
		print(mapping)
		print("====Circuit original, mapping corrected: ")
		print(result_original_updatedmapping_dict)
		print("====Circuit mapped: ")
		print(result_mapped[0])
		print("\n\n")
		return False

def get_mapping(mapper_out_report_file):

	with open(mapper_out_report_file, "r") as fopen:
		lines = fopen.readlines()

	for line in lines:
		if "virt2real map after mapper" in line:
			mapping_line = line
			print ("Mapping line= ", mapping_line)

	mapping = mapping_line.split(": ")[1]
	mapping = mapping.replace('[','').replace(']', '')
	mapping = mapping.split(', ')
	mapping = list(map(int, mapping))

	return mapping

def get_circuit_perfect(quantumsim_filepath):
	quantumsim_file = importlib.import_module(quantumsim_filepath)
	return quantumsim_file.circuit_generated(False) #noise_flag = False


def compiler_validation(curdir, indir):
	os.chdir(os.path.join(curdir, indir))
	python_files = glob.glob("*mapped.py")
	os.chdir(curdir)
	# python_files = glob.glob(os.path.join(input_dir, '*mapped.py'))
	# python_files = list(map(os.path.basename, python_files))

	python_files = [ file.replace('.py', '') for file in python_files]
	print(python_files)
	unmapped_mapped_mapping_path_tuples = [(file.replace('mapped', ''), file, file.split('quantumsim')[0] + 'mapper_out.report') for file in python_files]
	
	indir_abs = os.path.join(curdir, indir)
	name_unmapped_mapped_mapping_data_tuples = [(element[1].split('_mapped')[0], get_circuit_perfect(element[0]), get_circuit_perfect(element[1]), get_mapping(os.path.join(indir_abs, element[2])))	for element in unmapped_mapped_mapping_path_tuples]


	validation_dict = {}
	for element in name_unmapped_mapped_mapping_data_tuples:
		# print(element[0])
		is_correct = compare(*element[1:])
		validation_dict[element[0]] = is_correct

	for circuit, validity in validation_dict.items():
		print("{} ({})".format(circuit, validity))

	return validation_dict


if __name__ == "__main__":
	# parser = argparse.ArgumentParser(description='Validate OpenQL compiler')
	# parser.add_argument('indir', help='Input dir for circuits')
	# args = parser.parse_args()
	indir = './test_files/test_output'
	# indir = args.indir
	curdir = os.getcwd()
	os.chdir(curdir)
	sys.path.append(os.path.join(curdir, indir))
	# os.chdir(os.path.join(curdir, indir))

	compiler_validation(curdir, indir)
	

	



		

	

    # def qx_simulation(self, qasm_f_path):

    #     qx = qxelarator.QX()

    #     qx.set(qasm_f_path)

    #     qx.execute()                            # execute

    #     # Measure
    #     c_buff = []
    #     for q in range(self.N_qubits):
    #         c_buff.append(qx.get_measurement_outcome(q))

    #     measurement = np.array(c_buff[::-1], dtype=float)
    #     print(qx.get_state())
    #     q_state = self.output_quantum_state(qx.get_state())

    #     return q_state, measurement