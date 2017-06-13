import os
import filecmp
import unittest
from openql import Kernel, Program

rootDir = os.path.dirname(os.path.realpath(__file__))

class Test_qubits(unittest.TestCase):

	def test_1_qubit(self):
		k = Kernel("aKernel")

		# populate kernel
		k.prepz(0);
		k.x(0);
		k.y(0);
		k.rx90(0);
		k.measure(0);

		nqubits = 1
		sweep_points = [2]
		num_circuits = 1
		p = Program("aProgram", nqubits)
		p.set_sweep_points(sweep_points, num_circuits)

		p.add_kernel(k) # add kernel to program
		p.compile()     # compile program

		gold = rootDir + '/golden/test_1_qubit.qasm'
		isSame = filecmp.cmp('aProgram.qasm', gold)
		self.assertTrue(isSame)

	def test_2_qubit(self):
		k = Kernel("aKernel")

		# populate kernel
		k.prepz(0);
		k.prepz(1);
		k.prepz(2);
		k.cnot(0,1);
		k.clifford(1,2);
		k.measure(2);

		nqubits = 3
		sweep_points = [2]
		num_circuits = 1
		p = Program("aProgram", nqubits)
		p.set_sweep_points(sweep_points, num_circuits)

		p.add_kernel(k) # add kernel to program
		p.compile()     # compile program

		gold = rootDir + '/golden/test_2_qubit.qasm'
		isSame = filecmp.cmp('aProgram.qasm', gold)
		self.assertTrue(isSame)

	def test_3_qubit(self):
		k = Kernel("aKernel")

		# populate kernel
		k.prepz(0);
		k.prepz(1);
		k.prepz(2);
		k.toffoli(0,1,2);
		k.measure(2);

		nqubits = 3
		sweep_points = [2]
		num_circuits = 1
		p = Program("aProgram", nqubits)
		p.set_sweep_points(sweep_points, num_circuits)

		p.add_kernel(k) # add kernel to program
		p.compile()     # compile program

		gold = rootDir + '/golden/test_3_qubit.qasm'
		isSame = filecmp.cmp('aProgram.qasm', gold)
		self.assertTrue(isSame)

if __name__ == '__main__':
    unittest.main()
