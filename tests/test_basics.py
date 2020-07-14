from openql import openql as ql
import unittest
import os
from test_QASM_assembler_present import assemble, assembler_present

curdir = os.path.dirname(__file__)
output_dir = os.path.join(curdir, 'test_output')

class Test_basic(unittest.TestCase):

    @classmethod
    def setUpClass(self):
        ql.set_option('output_dir', output_dir)
        ql.set_option('optimize', 'no')
        ql.set_option('log_level', 'LOG_WARNING')
        ql.set_option('use_default_gates', 'no')
        ql.set_option('write_qasm_files', 'yes')

        # TODO cleanup
        ql.set_option('scheduler', 'ALAP')
        ql.set_option('scheduler_post179', 'yes')



    @unittest.skipUnless(assembler_present, "libqasm not found")
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
        
        # load qasm
        qasm_files = []
        qasm_files.append(os.path.join(output_dir, 'basic_initialqasmwriter_out.qasm'))
        qasm_files.append(os.path.join(output_dir, 'basic_scheduledqasmwriter_out.qasm'))

        for qasm_file in qasm_files:
            print('assembling: {}'.format(qasm_file))
            assemble(qasm_file)

if __name__ == '__main__':
    unittest.main()
