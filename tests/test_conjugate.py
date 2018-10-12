import os
import unittest
from openql import openql as ql
import numpy as np

rootDir = os.path.dirname(os.path.realpath(__file__))
curdir = os.path.dirname(__file__)
output_dir = os.path.join(curdir, 'test_output')

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


class Test_conjugated_kernel(unittest.TestCase):

    @classmethod
    def setUpClass(self):
        ql.set_option('output_dir', output_dir)
        ql.set_option('optimize', 'no')
        ql.set_option('scheduler', 'ASAP')
        ql.set_option('log_level', 'LOG_WARNING')

    def test_conjugate(self):
        config_fn = os.path.join(curdir, 'test_cfg_none_simple.json')
        platform  = ql.Platform('platform_none', config_fn)
        num_qubits = 3
        p = ql.Program('test_conjugate', platform, num_qubits)

        k = ql.Kernel('kernel_orignal', platform, num_qubits)
        ck = ql.Kernel('kernel_conjugate', platform, num_qubits)

        k.gate("x", [0])
        k.gate("y", [0])
        k.gate("z", [0])
        k.gate("h", [0])
        k.gate("i", [0])
        k.gate("s", [0])
        k.gate("t", [0])
        k.gate("sdag", [0])
        k.gate("tdag", [0])

        k.gate('cnot', [0,1])
        k.gate('cphase', [1,2])
        k.gate('swap', [2,0])

        k.gate('toffoli', [0,1,2])

        # generate conjugate of k
        ck.conjugate(k)

        p.add_kernel(k)
        p.add_kernel(ck)

        p.compile()

        # gold_fn = rootDir + '/golden/test_conjugate.qasm'
        # qasm_fn = os.path.join(output_dir, p.name+'.qasm')
        # self.assertTrue( file_compare(qasm_fn, gold_fn) )

if __name__ == '__main__':
    unittest.main()
