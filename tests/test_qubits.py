import os
import filecmp
import unittest
from openql import openql as ql

rootDir = os.path.dirname(os.path.realpath(__file__))

curdir = os.path.dirname(__file__)
config_fn = os.path.join(curdir, 'test_config_default.json')
platf = ql.Platform("starmon", config_fn)

output_dir = os.path.join(curdir, 'test_output')

ql.set_option('output_dir', output_dir)
ql.set_option('optimize', 'no')
ql.set_option('scheduler', 'ALAP')
ql.set_option('log_level', 'LOG_WARNING')

def file_compare(fn1, fn2):
    isSame = False
    with open(fn1, 'r') as f1:
        with open(fn2, 'r') as f2:
            a = f1.read()
            b = f2.read()
            f1.close()
            f2.close()
            isSame = (a==b)
    return isSame

class Test_qubits(unittest.TestCase):

    def test_1_qubit(self):
        k = ql.Kernel("aKernel", platf)

        # populate kernel
        k.prepz(0)
        k.x(0)
        k.y(0)
        k.rx90(0)
        k.measure(0)

        nqubits = 1
        sweep_points = [2]
        num_circuits = 1
        p = ql.Program("1_qubit_program", nqubits, platf)
        p.set_sweep_points(sweep_points, len(sweep_points))

        p.add_kernel(k)  # add kernel to program
        p.compile()

        gold_fn = rootDir + '/golden/test_1_qubit.qasm'
        qasm_fn = os.path.join(output_dir, p.name+'.qasm')

        self.assertTrue( file_compare(qasm_fn, gold_fn) )

    def test_2_qubit(self):

        k = ql.Kernel("aKernel", platf)

        # populate kernel
        k.prepz(0)
        k.prepz(1)
        k.prepz(2)
        k.cz(0, 1)
        k.clifford(1, 2)
        k.measure(2)

        nqubits = 3
        sweep_points = [2]
        num_circuits = 1
        p = ql.Program("2_qubit_program", nqubits, platf)
        p.set_sweep_points(sweep_points, len(sweep_points))

        p.add_kernel(k)  # add kernel to program
        p.compile()

        gold_fn = rootDir + '/golden/test_2_qubit.qasm'
        qasm_fn = os.path.join(output_dir, p.name+'.qasm')

        self.assertTrue( file_compare(qasm_fn, gold_fn) )

    def test_3_qubit(self):
        k = ql.Kernel("aKernel", platf)

        # populate kernel
        k.prepz(0)
        k.prepz(1)
        k.prepz(2)
        k.toffoli(0, 1, 2)
        k.measure(2)

        nqubits = 3
        sweep_points = [2]
        num_circuits = 1
        p = ql.Program("3_qubit_program", nqubits, platf)
        p.set_sweep_points(sweep_points, len(sweep_points))

        p.add_kernel(k)  # add kernel to program

        ql.set_option('decompose_toffoli', 'no')
        p.compile()

        gold_fn = rootDir + '/golden/test_3_qubit.qasm'
        qasm_fn = os.path.join(output_dir, p.name+'.qasm')

        self.assertTrue( file_compare(qasm_fn, gold_fn) )

if __name__ == '__main__':
    unittest.main()
