import os
import unittest
from openql import openql as ql
from utils import file_compare

rootDir = os.path.dirname(os.path.realpath(__file__))
curdir = os.path.dirname(__file__)
output_dir = os.path.join(curdir, 'test_output')


class Test_conjugated_kernel(unittest.TestCase):

    @classmethod
    def setUpClass(self):
        ql.set_option('output_dir', output_dir)
        ql.set_option('optimize', 'no')
        ql.set_option('scheduler', 'ASAP')
        ql.set_option('log_level', 'LOG_WARNING')

    def test_unitary_basic(self):
        config_fn = os.path.join(curdir, 'test_cfg_none.json')
        platform = ql.Platform('platform_none', config_fn)
        num_qubits = 3
        p = ql.Program('test_unitary_pass', platform, num_qubits)
        k = ql.Kernel('akernel', platform, num_qubits)

        u = ql.Unitary('u1', [  complex(1.0, 0.0), complex(0.0, 0.0),
                                complex(0.0, 0.0), complex(0.0, 1.0)])
        u.decompose()

        k.gate("i", [0])
        k.gate("s", [0])
        k.gate(u, [2])

        p.add_kernel(k)
        p.compile()


    def test_unitary_undecomposed(self):
        config_fn = os.path.join(curdir, 'test_cfg_none.json')
        platform = ql.Platform('platform_none', config_fn)
        num_qubits = 3
        p = ql.Program('test_unitary_pass', platform, num_qubits)
        k = ql.Kernel('akernel', platform, num_qubits)

        u = ql.Unitary('u1', [  complex(1.0, 0.0), complex(0.0, 0.0),
                                complex(0.0, 0.0), complex(0.0, 1.0)])
        k.gate("s", [0])

        # adding un-decomposed u to kernel should raise error
        with self.assertRaises(Exception) as cm:
            k.gate(u, [2])

        self.assertEqual(str(cm.exception), 'Unitary \'u1\' not decomposed. Cannot be added to kernel!')


    # TODO: add more tests here

    

if __name__ == '__main__':
    unittest.main()
