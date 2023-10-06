import openql as ql
import os
import unittest

from config import json_dir, output_dir


class TestConfiguration(unittest.TestCase):
    @classmethod
    def setUp(cls):
        ql.initialize()
        ql.set_option('output_dir', output_dir)
        ql.set_option('optimize', 'no')
        ql.set_option('scheduler', 'ALAP')
        ql.set_option('log_level', 'LOG_WARNING')
        # ql.set_option('write_qasm_files', 'yes')

    @unittest.skip("QI2 integration test: temporarily skipped this test")
    def test_case_insensitivity(self):
        config_fn = os.path.join(json_dir, 'test_cfg_CCL_long_duration.json')
        platform = ql.Platform('seven_qubits_chip', config_fn)
        p = ql.Program("test_case_insensitivity", platform, platform.get_qubit_number())

        k = ql.Kernel('aKernel', platform, platform.get_qubit_number())
        k.gate('Rx180', [0])  # in the configuration its name is rx180 q0
        k.gate('rX180', [2])  # in the configuration its name is rx180 q2
        k.gate('CZ', [2, 0])  # in the configuration its name is cz q2, q0 

        # add the kernel to the program
        p.add_kernel(k)

        # compile the program
        p.compile()
        
        # # load qasm
        # qasm_files = []
        # qasm_files.append(os.path.join(output_dir, 'aProgram.qasm'))
        # qasm_files.append(os.path.join(output_dir, 'aProgram_scheduled.qasm'))

        # for qasm_file in qasm_files:
        #    qasm_reader = ql.QASM_Loader(qasm_file)
        #    errors = qasm_reader.load()
        #    self.assertTrue(errors == 0)

    def test_missing_instr(self):
        config_fn = os.path.join(json_dir, 'test_cfg_CCL_long_duration.json')
        platform = ql.Platform('seven_qubits_chip', config_fn)
        p = ql.Program("test_missing_instr", platform, platform.get_qubit_number())

        k = ql.Kernel('aKernel', platform, platform.get_qubit_number())
        k.gate('rx180', [0])  # available

        try:
            k.gate('rx181', [2])  # not available, will raise exception
        except:
            pass
        else:
            print("Exception not raised for missing instruction !!!")
            raise

        # add the kernel to the program
        p.add_kernel(k)

        # compile the program
        p.compile()
        
        # load qasm
        # qasm_files = []
        # qasm_files.append(os.path.join(output_dir, 'aProgram.qasm'))
        # qasm_files.append(os.path.join(output_dir, 'aProgram_scheduled.qasm'))

        # for qasm_file in qasm_files:
        #    qasm_reader = ql.QASM_Loader(qasm_file)
        #    errors = qasm_reader.load()
        #    self.assertTrue(errors == 0)


if __name__ == '__main__':
    unittest.main()
