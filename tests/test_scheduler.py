import os
from utils import file_compare
import unittest
from openql import openql as ql

rootDir = os.path.dirname(os.path.realpath(__file__))
curdir = os.path.dirname(__file__)
config_fn = os.path.join(curdir, 'test_179.json')
platf = ql.Platform("starmon", config_fn)

output_dir = os.path.join(curdir, 'test_output')

class Test_scheduler(unittest.TestCase):

    def setUp(self):
        ql.set_option('output_dir', output_dir)
        ql.set_option('optimize', 'no')
        ql.set_option("scheduler_uniform", "no")
        ql.set_option('log_level', 'LOG_WARNING')
        ql.set_option('scheduler', 'ASAP')

    def tearDown(self):
        ql.set_option('scheduler', 'ALAP')

    def test_independent(self):
        nqubits = 4

        k = ql.Kernel("aKernel", platf, nqubits)

        for i in range(4):
            k.prepz(i)

        k.cz(0, 1)
        k.cz(2, 3)

        k.measure(0)
        k.measure(1)

        sweep_points = [2]

        p = ql.Program("independent", platf, nqubits)
        p.set_sweep_points(sweep_points, len(sweep_points))
        p.add_kernel(k)
        p.compile()

        gold_fn = rootDir + '/golden/test_independence.qasm'
        qasm_fn = os.path.join(output_dir, p.name+'_scheduled.qasm')
        self.assertTrue( file_compare(qasm_fn, gold_fn) )




if __name__ == '__main__':
    unittest.main()
