import openql as ql
import os
import unittest

from config import output_dir


platform = ql.Platform("starmon", "cc_light")
ql.set_option('output_dir', output_dir)
# ql.set_option('write_qasm_files', 'yes')


class TestKernel(unittest.TestCase):
    @classmethod
    def setUp(cls):
        ql.initialize()

    def custom_gate_test(self):
        num_qubits = 1

        # create a kernel
        k = ql.Kernel("aKernel", platform, num_qubits)

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

        # create a program
        p = ql.Program("custom_gate_test", platform, num_qubits)

        # add kernel to program
        p.add_kernel(k)

        # compile  opt  verbose
        p.compile(optimize=False, scheduler='ASAP', log_level='LOG_WARNING')

        # load qasm
        qasm_files = [os.path.join(output_dir, 'custom_gate_test.qasm'),
                      os.path.join(output_dir, 'custom_gate_scheduled_test.qasm')]

        for qasm_file in qasm_files:
            qasm_reader = ql.QASM_Loader(qasm_file)
            errors = qasm_reader.load()
            self.assertTrue(errors == 0)

        # all the outputs are generated in 'output' dir


if __name__ == '__main__':
    unittest.main()
