from openql import openql as ql
import unittest
import os
from utils import file_compare

curdir = os.path.dirname(os.path.realpath(__file__))
output_dir = os.path.join(curdir, 'test_output')

class Test_basic(unittest.TestCase):

    @classmethod
    def setUp(self):
        ql.initialize()

        ql.set_option('output_dir', output_dir)
        ql.set_option('optimize', 'no')
        ql.set_option('log_level', 'LOG_WARNING')
        ql.set_option('use_default_gates', 'no')
        # ql.set_option('write_qasm_files', 'yes')

        # TODO cleanup
        ql.set_option('scheduler', 'ALAP')
        ql.set_option('scheduler_post179', 'yes')

    def test_compilation(self):

        print('output dir : {}'.format( ql.get_option('output_dir') ) )
        config_fn = os.path.join(curdir, 'hardware_config_qx.json')
        platf = ql.Platform("starmon", config_fn)
        sweep_points = [1]
        nqubits = 2
        p = ql.Program("basic", platf, nqubits, nqubits)
        p.set_sweep_points(sweep_points)

        # populate kernel
        k = ql.Kernel("first_kernel", platf, nqubits, nqubits)

        k.gate('prep_z', [0])
        k.gate('x', [0])
        k.gate('x90', [0])
        k.gate('measure', [0])
        p.add_kernel(k)

        k = ql.Kernel("second_kernel", platf, nqubits, nqubits)
        k.gate('prep_z', [0])
        k.gate('x90', [0])
        k.gate('cz', [0, 1])
        k.gate('x90', [0])
        k.gate("measure", [0])
        p.add_kernel(k)

        p.compile()

        for name in ('basic.qasm', 'basic_scheduled.qasm'):
            self.assertTrue(file_compare(os.path.join(output_dir, name), os.path.join(curdir, 'golden', name)))

if __name__ == '__main__':
    unittest.main()
