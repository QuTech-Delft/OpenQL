import os
from utils import file_compare
import unittest
from openql import openql as ql

rootDir = os.path.dirname(os.path.realpath(__file__))

curdir = os.path.dirname(__file__)
config_fn = os.path.join(curdir, 'test_config_default.json')
platf = ql.Platform("starmon", config_fn)

output_dir = os.path.join(curdir, 'test_output')


class Test_unitary_decomp(unittest.TestCase):

    @classmethod
    def setUpClass(self):
        ql.set_option('output_dir', output_dir)
        ql.set_option('optimize', 'no')
        ql.set_option('scheduler', 'ALAP')
        ql.set_option('log_level', 'LOG_WARNING')

    # def test_1_qubit(self):
    #     nqubits = 1
    #     sweep_points = [2]

    #     k = ql.Kernel("aKernel", platf, nqubits)

    #     # populate kernel
    #     k.prepz(0)
    #     k.unitarydecomp(0, [[0.7071,0.7071],[-0.7071,0.7071]])

    #     p = ql.Program("1_qubit_unitary_decomp", platf, nqubits)
    #     p.set_sweep_points(sweep_points, len(sweep_points))

    #     p.add_kernel(k)  # add kernel to program
    #     p.compile()

    #     gold_fn = rootDir + '/golden/1_qubit_unitary_decomp.qasm'
    #     qasm_fn = os.path.join(output_dir, p.name+'.qasm')
    #     self.assertTrue( file_compare(qasm_fn, gold_fn) )

    def test_unitary_decompose_X(self):
        config_fn = os.path.join(curdir, 'test_cfg_none.json')
        platform = ql.Platform('platform_none', config_fn)
        num_qubits = 1
        p = ql.Program('test_unitary_pass', platform, num_qubits)
        k = ql.Kernel('akernel', platform, num_qubits)

        u = ql.Unitary('u1', [  complex(1.0, 0.0), complex(0.0, 0.0),
                                complex(0.0, 0.0), complex(0.0, 1.0)])
        u.decompose()
        k.gate(u, [0])

        p.add_kernel(k)
        p.compile()

        self.assertEqual(p.qasm(), "this is not equal")

if __name__ == '__main__':
    unittest.main()
