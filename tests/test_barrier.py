import os
import unittest
from openql import openql as ql
from utils import file_compare

rootDir = os.path.dirname(os.path.realpath(__file__))
curdir = os.path.dirname(__file__)
output_dir = os.path.join(curdir, 'test_output')


class Test_barrier(unittest.TestCase):

    def setUp(self):
        ql.set_option('output_dir', output_dir)
        ql.set_option('log_level', 'LOG_WARNING')
        ql.set_option('optimize', 'no')
        ql.set_option('scheduler', 'ALAP')
        ql.set_option('scheduler_post179', 'yes')
        ql.set_option("scheduler_commute", 'no')
        ql.set_option('use_default_gates', 'yes')
        ql.set_option('write_qasm_files', 'yes')
        

    # barrier on specified qubits
    def test_barrier(self):
        self.setUp()
        config_fn = os.path.join(curdir, 'hardware_config_cc_light.json')
        platform = ql.Platform('seven_qubits_chip', config_fn)
        sweep_points = [1, 2]
        num_qubits = platform.get_qubit_number()
        p = ql.Program('test_barrier', platform, num_qubits)
        p.set_sweep_points(sweep_points)

        k = ql.Kernel('aKernel', platform, num_qubits)

        k.gate("x", [0])
        k.gate("x", [1])
        k.gate("y", [0])

        # k.barrier([0, 1])
        # OR
        k.gate("barrier", [0, 1])

        k.gate("measure", [0])
        k.gate("measure", [1])

        p.add_kernel(k)
        p.compile()

        QISA_fn = os.path.join(output_dir, p.name+'.qisa')
        gold_fn = rootDir + '/golden/test_barrier.qisa'
        self.assertTrue(file_compare(QISA_fn, gold_fn))

    # barrier on specified qubits with 'wait' and duration = 0
    def test_wait_barrier(self):
        self.setUp()
        config_fn = os.path.join(curdir, 'hardware_config_cc_light.json')
        platform = ql.Platform('seven_qubits_chip', config_fn)
        sweep_points = [1, 2]
        num_qubits = platform.get_qubit_number()
        p = ql.Program('test_wait_barrier', platform, num_qubits)
        p.set_sweep_points(sweep_points)

        k = ql.Kernel('aKernel', platform, num_qubits)

        k.gate("x", [0])
        k.gate("x", [1])
        k.gate("y", [0])
        k.gate("wait", [0, 1], 0)  # this will serve as barrier
        k.gate("measure", [0])
        k.gate("measure", [1])

        p.add_kernel(k)
        ql.print_options()
        p.compile()

        QISA_fn = os.path.join(output_dir, p.name+'.qisa')
        gold_fn = rootDir + '/golden/test_wait_barrier.qisa'
        self.assertTrue(file_compare(QISA_fn, gold_fn))


    # barrier on all qubits with barrier
    def test_barrier_all_1(self):
        self.setUp()
        config_fn = os.path.join(curdir, 'hardware_config_cc_light.json')
        platform = ql.Platform('seven_qubits_chip', config_fn)
        sweep_points = [1, 2]
        num_qubits = platform.get_qubit_number()
        p = ql.Program('test_barrier_all_1', platform, num_qubits)
        p.set_sweep_points(sweep_points)

        k = ql.Kernel('aKernel', platform, num_qubits)

        k.gate('ry90', [0])
        k.gate('ry90', [2])
        k.gate('rx90', [2])

        k.gate('measure', [0])
        k.gate('measure', [1])
        k.gate('measure', [2])
        k.gate('measure', [4])

        # with barrier syntax
        k.barrier()

        k.gate('ry90', [4])
        k.gate('ry90', [1])

        k.gate('measure', [0])
        k.gate('measure', [1])
        k.gate('measure', [2])
        k.gate('measure', [4])

        k.barrier()

        k.gate('ry90', [4])
        k.gate('ry90', [1])

        p.add_kernel(k)
        p.compile()


        QASM_fn = os.path.join(output_dir, p.name+'_scheduledqasmwriter_out.qasm')
        gold_fn = rootDir + '/golden/test_barrier_all.qasm'
        self.assertTrue(file_compare(QASM_fn, gold_fn))


    # barrier on all qubits with generalized gate API using 'barrier'
    def test_barrier_all_2(self):
        self.setUp()
        config_fn = os.path.join(curdir, 'hardware_config_cc_light.json')
        platform = ql.Platform('seven_qubits_chip', config_fn)
        sweep_points = [1, 2]
        num_qubits = platform.get_qubit_number()
        p = ql.Program('test_barrier_all_2', platform, num_qubits)
        p.set_sweep_points(sweep_points)

        k = ql.Kernel('aKernel', platform, num_qubits)

        k.gate('ry90', [0])
        k.gate('ry90', [2])
        k.gate('rx90', [2])

        k.gate('measure', [0])
        k.gate('measure', [1])
        k.gate('measure', [2])
        k.gate('measure', [4])

        # with generalized gate syntax
        k.gate('barrier', [])

        k.gate('ry90', [4])
        k.gate('ry90', [1])

        k.gate('measure', [0])
        k.gate('measure', [1])
        k.gate('measure', [2])
        k.gate('measure', [4])

        k.barrier()

        k.gate('ry90', [4])
        k.gate('ry90', [1])

        p.add_kernel(k)
        p.compile()

        QASM_fn = os.path.join(output_dir, p.name+'_scheduledqasmwriter_out.qasm')
        gold_fn = rootDir + '/golden/test_barrier_all.qasm'
        self.assertTrue(file_compare(QASM_fn, gold_fn))


    # barrier on all qubits with generalized gate API using wait with duration 0
    def test_barrier_all_3(self):
        self.setUp()
        config_fn = os.path.join(curdir, 'hardware_config_cc_light.json')
        platform = ql.Platform('seven_qubits_chip', config_fn)
        sweep_points = [1, 2]
        num_qubits = platform.get_qubit_number()
        p = ql.Program('test_barrier_all_3', platform, num_qubits)
        p.set_sweep_points(sweep_points)

        k = ql.Kernel('aKernel', platform, num_qubits)

        k.gate('ry90', [0])
        k.gate('ry90', [2])
        k.gate('rx90', [2])

        k.gate('measure', [0])
        k.gate('measure', [1])
        k.gate('measure', [2])
        k.gate('measure', [4])

        # with generalized gate syntax using wait with duration 0
        k.gate('wait', [], 0)

        k.gate('ry90', [4])
        k.gate('ry90', [1])

        k.gate('measure', [0])
        k.gate('measure', [1])
        k.gate('measure', [2])
        k.gate('measure', [4])

        k.barrier()

        k.gate('ry90', [4])
        k.gate('ry90', [1])

        p.add_kernel(k)
        p.compile()

        QASM_fn = os.path.join(output_dir, p.name+'_scheduledqasmwriter_out.qasm')
        gold_fn = rootDir + '/golden/test_barrier_all.qasm'
        self.assertTrue(file_compare(QASM_fn, gold_fn))

if __name__ == '__main__':
    unittest.main()
