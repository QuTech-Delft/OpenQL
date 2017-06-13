import unittest
from openql import Kernel, Program

class Test_basic(unittest.TestCase):

	def test_compilation(self):
		k = Kernel("aKernel")

		# populate kernel
		for i in range(4):
		    k.prepz(i);

		for i in range(4):
		    k.hadamard(i);

		for i in range(4):
		    for j in range(3,5):
		        k.cnot(i,j);

		nqubits = 5
		sweep_points = [2]
		num_circuits = 1
		p = Program("aProgram", nqubits)
		p.set_sweep_points(sweep_points, num_circuits)

		p.add_kernel(k) # add kernel to program
		p.compile()     # compile program

	def test_scheduling(self):
		k = Kernel("aKernel")

		# populate kernel
		for i in range(4):
		    k.prepz(i);

		for i in range(4):
		    k.hadamard(i);

		for i in range(4):
		    for j in range(3,5):
		        k.cnot(i,j);

		nqubits = 5
		sweep_points = [2]
		num_circuits = 1
		p = Program("aProgram", nqubits)
		p.set_sweep_points(sweep_points, num_circuits)

		p.add_kernel(k) # add kernel to program
		p.compile()     # compile program
		p.schedule()


if __name__ == '__main__':
    unittest.main()
