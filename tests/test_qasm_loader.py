import os
import unittest
from openql import openql as ql

curdir = os.path.dirname(__file__)
config_fn = os.path.join(curdir, 'test_cfg_cbox.json')
platf = ql.Platform("starmon", config_fn)

output_dir = os.path.join(curdir, 'test_output')
ql.set_output_dir(output_dir)


class Test_QASM_Loader(unittest.TestCase):

    @classmethod
    def setUpClass(self):
        pass

    def test_sequential_program(self):
        num_circuits = 2
        nqubits = 2
        sweep_points = [1,2]
        p = ql.Program("sequential_program", nqubits, platf)
        p.set_sweep_points(sweep_points, len(sweep_points))
        # populate kernel using default gates
        k = ql.Kernel("kernel_1", platf)
        k.prepz(0)
        k.prepz(0)
        k.ry90(0)
        k.rx180(0)
        k.ry90(0)
        k.measure(0)
        
        # add the kernel to the program
        p.add_kernel(k)

        # populate a second kernel using both custom and default gates
        k = ql.Kernel("kernel_2", platf)
        k.gate("prepz", [0]) 
        k.gate('rx180', [0])
        k.rx180(0)
        k.gate('cz', [0, 1])
        k.gate("measure", [0])

        # add the kernel to the program
        p.add_kernel(k)

        # compile the program
        p.compile(optimize=False, scheduler='ASAP', log_level='LOG_WARNING')
        
        # load qasm
        qasm_files = []
        qasm_files.append(os.path.join(output_dir, 'sequential_program.qasm'))
        qasm_files.append(os.path.join(output_dir, 'sequential_program_scheduled.qasm'))

        for qasm_file in qasm_files:
           qasm_reader = ql.QASM_Loader(qasm_file)
           errors = qasm_reader.load()
           self.assertTrue(errors == 0)

    def test_parallel_program(self):
        num_circuits = 2
        nqubits = 2
        sweep_points = [1,2]
        p = ql.Program("parallel_program", nqubits, platf)
        p.set_sweep_points(sweep_points, len(sweep_points))
        # populate kernel using default gates
        k = ql.Kernel("kernel_1", platf)
        k.prepz(0)
        k.prepz(1)
        k.ry90(0)
        k.rx180(0)
        k.ry90(1)
        k.rx180(1)
        k.measure(0)
        
        # add the kernel to the program
        p.add_kernel(k)

        # populate a second kernel using both custom and default gates
        k = ql.Kernel("kernel_2", platf)
        k.gate("prepz", [0]) 
        k.gate('rx180', [0])
        k.rx180(0)
        k.ry90(0)
        k.rx180(1)
        k.gate('cz', [0, 1])
        k.gate("measure", [0])

        # add the kernel to the program
        p.add_kernel(k)

        # compile the program
        p.compile(optimize=False, scheduler='ASAP', log_level='LOG_WARNING')
        
        # load qasm
        qasm_files = []
        qasm_files.append(os.path.join(output_dir, 'parallel_program.qasm'))
        qasm_files.append(os.path.join(output_dir, 'parallel_program_scheduled.qasm'))

        for qasm_file in qasm_files:
           qasm_reader = ql.QASM_Loader(qasm_file)
           errors = qasm_reader.load()
           self.assertTrue(errors == 0)

if __name__ == '__main__':
    unittest.main()
