from openql import openql as ql
import unittest
import os

curdir = os.path.dirname(__file__)

class Test_basic(unittest.TestCase):

    def test_compilation(self):
        # set global options kernel
        output_dir = os.path.join(curdir, 'test_output')
        ql.set_option('output_dir', output_dir)
        ql.set_option('optimize', 'no')
        ql.set_option('scheduler', 'ALAP')
        ql.set_option('log_level', 'LOG_WARNING')

        print('output dir : {}'.format( ql.get_option('output_dir') ) )
        config_fn = os.path.join(curdir, 'test_cfg_cbox.json')
        platf = ql.Platform("starmon", config_fn)
        sweep_points = [1]
        nqubits = 2
        p = ql.Program("basic", platf, nqubits, nqubits)
        p.set_sweep_points(sweep_points, len(sweep_points))

        # populate kernel
        k = ql.Kernel("first_kernel", platf, nqubits, nqubits)
        # k.prepz(0)
        k.gate("prepz", [0])
        # k.x(0)
        k.gate('rx180', [0])
        k.gate("measure", [0])
        p.add_kernel(k)

        k = ql.Kernel("second_kernel", platf, nqubits, nqubits)
        # k.prepz(0)
        k.gate("prepz", [0])
        k.gate('rx90', [0])
        k.gate('rx180', [1])
        k.gate('cz', [0, 1])
        k.gate('rx90', [0])
        k.gate("measure", [0])
        p.add_kernel(k)

        p.compile()
        
        # load qasm
        qasm_files = []
        qasm_files.append(os.path.join(output_dir, 'basic.qasm'))
        qasm_files.append(os.path.join(output_dir, 'basic_scheduled.qasm'))

        for qasm_file in qasm_files:
           qasm_reader = ql.QASM_Loader(qasm_file)
           errors = qasm_reader.load()
           self.assertTrue(errors == 0)

if __name__ == '__main__':
    unittest.main()
