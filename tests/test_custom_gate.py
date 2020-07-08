import os
import unittest
from openql import openql as ql

curdir = os.path.dirname(__file__)
config_fn = os.path.join(curdir, 'hardware_config_cc_light.json')
platf = ql.Platform("starmon", config_fn)

output_dir = os.path.join(curdir, 'test_output')
ql.set_option('output_dir', output_dir)
ql.set_option('write_qasm_files', 'yes')


class Test_kernel(unittest.TestCase):

    def custom_gate_test(self):
        nqubits = 1

        # create a kernel
        k = ql.Kernel("aKernel", platf, nqubits)

        # load custom instruction definition
        k.load_custom_instructions("instructions.json")

        # print user-defined instructions (qasm/microcode)
        k.print_custom_instructions()

        # populate a kernel
        k.prepz(0)
        k.x(0)

        k.gate("rx180", [0])
        # or
        k.gate("rx180", 0)

        sweep_points = [1, 1.25, 1.75, 2.25, 2.75 ]

        # create a program
        p = ql.Program("aProgram", platf, nqubits)
        p.set_sweep_points(sweep_points, len(sweep_points))

        # add kernel to program
        p.add_kernel(k)

        # compile  opt  verbose
        p.compile(optimize=False, scheduler='ASAP', log_level='LOG_WARNING')

        # load qasm
        qasm_files = []
        qasm_files.append(os.path.join(output_dir, 'aProgram.qasm'))
        qasm_files.append(os.path.join(output_dir, 'aProgram_scheduled.qasm'))

        for qasm_file in qasm_files:
           qasm_reader = ql.QASM_Loader(qasm_file)
           errors = qasm_reader.load()
           self.assertTrue(errors == 0)



        # all the outputs are generated in 'output' dir

if __name__ == '__main__':
    unittest.main()
