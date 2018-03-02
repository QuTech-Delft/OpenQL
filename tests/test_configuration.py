import numpy as np
import os
import unittest
from openql import openql as ql

curdir = os.path.dirname(__file__)
output_dir = os.path.join(curdir, 'test_output')
ql.set_output_dir(output_dir)

class Test_Configuration(unittest.TestCase):
    def test_case_insensitivity(self):
        config_fn = os.path.join(curdir, 'test_cfg_CCL_long_duration.json')
        platform  = ql.Platform('seven_qubits_chip', config_fn)
        p = ql.Program(pname="aProgram", nqubits=platform.get_qubit_number(), p=platform)
        sweep_points = [1,2]
        p.set_sweep_points(sweep_points, len(sweep_points))

        k = ql.Kernel('aKernel', platform)
        k.gate('rx180', 0)  # in the configuartion its name is rX180 q0
        k.gate('rX180', 2)  # in the configuartion its name is rx180 q2
        k.gate('cZ', 2, 0)  # in the configuartion its name is CZ q2, q0 
                            # (Note space in q2, q0)

        # add the kernel to the program
        p.add_kernel(k)

        # compile the program
        p.compile(optimize=False, scheduler='ALAP', log_level='LOG_WARNING')
        
        # load qasm
        qasm_files = []
        qasm_files.append(os.path.join(output_dir, 'aProgram.qasm'))
        qasm_files.append(os.path.join(output_dir, 'aProgram_scheduled.qasm'))

        for qasm_file in qasm_files:
           qasm_reader = ql.QASM_Loader(qasm_file)
           errors = qasm_reader.load()
           self.assertTrue(errors == 0)



    def test_missing_instr(self):
        config_fn = os.path.join(curdir, 'test_cfg_CCL_long_duration.json')
        platform  = ql.Platform('seven_qubits_chip', config_fn)
        p = ql.Program(pname="aProgram", nqubits=platform.get_qubit_number(), p=platform)
        sweep_points = [1,2]
        p.set_sweep_points(sweep_points, len(sweep_points))

        k = ql.Kernel('aKernel', platform)
        k.gate('rx180', 0)  # available

        try:
            k.gate('rx181', 2)  # not available, will raise exception
        except:
            pass
        else:
            print("Exception not raised for missing instruction !!!")
            raise

        # add the kernel to the program
        p.add_kernel(k)

        # compile the program
        p.compile(optimize=False, scheduler='ALAP', log_level='LOG_WARNING')
        
        # load qasm
        qasm_files = []
        qasm_files.append(os.path.join(output_dir, 'aProgram.qasm'))
        qasm_files.append(os.path.join(output_dir, 'aProgram_scheduled.qasm'))

        for qasm_file in qasm_files:
           qasm_reader = ql.QASM_Loader(qasm_file)
           errors = qasm_reader.load()
           self.assertTrue(errors == 0)



    def test_missing_cc_light_instr(self):
        config_fn = os.path.join(curdir, 'test_cfg_CCL_long_duration.json')
        platform  = ql.Platform('seven_qubits_chip', config_fn)
        p = ql.Program(pname="aProgram", nqubits=platform.get_qubit_number(), p=platform)
        sweep_points = [1,2]
        p.set_sweep_points(sweep_points, len(sweep_points))

        k = ql.Kernel('aKernel', platform)

        k.gate('rx180', 0)  # fine

        # cc_light_instr field is empty for this instruction
        # so it will raise error
        k.gate('rx180', 6)

        # add the kernel to the program
        p.add_kernel(k)

        try:
            # compile the program
            p.compile(optimize=False, scheduler='ALAP', log_level='LOG_WARNING')
        except:
            # print("exception raised")
            pass
        else:
            print("Exception not raised for missing cc_light_instr !!!")
            raise

if __name__ == '__main__':
    unittest.main()
