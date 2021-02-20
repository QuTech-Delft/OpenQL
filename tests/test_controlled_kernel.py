import os
import unittest
from openql import openql as ql
import numpy as np
from utils import file_compare

curdir = os.path.dirname(os.path.realpath(__file__))
output_dir = os.path.join(curdir, 'test_output')


class Test_controlled_kernel(unittest.TestCase):

    @classmethod
    def setUp(self):
        ql.initialize()
        ql.set_option('output_dir', output_dir)
        ql.set_option('optimize', 'no')
        ql.set_option('scheduler', 'ASAP')
        ql.set_option('log_level', 'LOG_WARNING')

    def test_controlled_single_qubit_gates(self):
        config_fn = os.path.join(curdir, 'test_cfg_none_simple.json')
        platform  = ql.Platform('platform_none', config_fn)
        num_qubits = 3
        p = ql.Program('test_controlled_single_qubit_gates', platform, num_qubits)

        k = ql.Kernel('kernel1', platform, num_qubits)
        ck = ql.Kernel('controlled_kernel1', platform, num_qubits)

        k.gate("x", [0])
        k.gate("y", [0])
        k.gate("z", [0])
        k.gate("h", [0])
        k.gate("i", [0])
        k.gate("s", [0])
        k.gate("t", [0])

        # generate controlled version of k.
        # qubit 1 is used as control qubit
        # qubit 2 is used as ancilla qubit
        ck.controlled(k, [1], [2])

        p.add_kernel(k)
        p.add_kernel(ck)

        p.compile()

        gold_fn = curdir + '/golden/' + p.name +'_scheduled.qasm'
        qasm_fn = os.path.join(output_dir, p.name+'_scheduled.qasm')
        self.assertTrue(file_compare(qasm_fn, gold_fn))

    def test_controlled_rotations(self):
        config_fn = os.path.join(curdir, 'test_cfg_none_simple.json')
        platform  = ql.Platform('platform_none', config_fn)
        num_qubits = 3
        p = ql.Program('test_controlled_rotations', platform, num_qubits)

        k = ql.Kernel('kernel1', platform, num_qubits)
        ck = ql.Kernel('controlled_kernel1', platform, num_qubits)

        k.gate("rx", [0], 0, (np.pi)/4 ) # duration = 0 uses default value of duration
        k.gate("ry", [0], 0, (np.pi)/4 ) # duration = 0 uses default value of duration
        k.gate("rz", [0], 0, (np.pi)/4 ) # duration = 0 uses default value of duration

        # generate controlled version of k.
        # qubit 1 is used as control qubit
        # qubit 2 is used as ancilla qubit
        ck.controlled(k, [1], [2])

        p.add_kernel(k)
        p.add_kernel(ck)

        p.compile()

        gold_fn = curdir + '/golden/' + p.name +'_scheduled.qasm'
        qasm_fn = os.path.join(output_dir, p.name+'_scheduled.qasm')
        self.assertTrue(file_compare(qasm_fn, gold_fn))

    def test_controlled_two_qubit_gates(self):
        config_fn = os.path.join(curdir, 'test_cfg_none_simple.json')
        platform  = ql.Platform('platform_none', config_fn)
        num_qubits = 4
        p = ql.Program('test_controlled_two_qubit_gates', platform, num_qubits)

        k = ql.Kernel('kernel1', platform, num_qubits)
        ck = ql.Kernel('controlled_kernel1', platform, num_qubits)

        k.gate("swap", [0, 1])

        # generate controlled version of k.
        # qubit 2 is used as control qubit
        # qubit 3 is used as ancilla qubit
        ck.controlled(k, [2], [3])

        p.add_kernel(k)
        p.add_kernel(ck)

        p.compile()

        gold_fn = curdir + '/golden/' + p.name +'_scheduled.qasm'
        qasm_fn = os.path.join(output_dir, p.name+'_scheduled.qasm')
        self.assertTrue(file_compare(qasm_fn, gold_fn))

    def test_multi_controlled(self):
        config_fn = os.path.join(curdir, 'test_cfg_none_simple.json')
        platform  = ql.Platform('platform_none', config_fn)
        num_qubits = 12
        p = ql.Program('test_multi_controlled', platform, num_qubits)

        k = ql.Kernel('kernel1', platform, num_qubits)
        ck = ql.Kernel('controlled_kernel1', platform, num_qubits)

        k.gate('x', [0])
        k.gate('cnot', [0, 1])

        # disable toffoli decomposition to visualize the network
        ql.set_option('decompose_toffoli', 'no')

        # generate controlled version of k.
        # qubits 2, 3, 4, 5, 6 are used as control qubits
        # qubits 7, 8, 9, 10, 11 are used as ancilla qubits
        ck.controlled(k, [2, 3, 4, 5, 6], [7, 8, 9, 10, 11])

        p.add_kernel(k)
        p.add_kernel(ck)

        p.compile()

        gold_fn = curdir + '/golden/' + p.name +'_scheduled.qasm'
        qasm_fn = os.path.join(output_dir, p.name+'_scheduled.qasm')
        self.assertTrue(file_compare(qasm_fn, gold_fn))

    def test_decompose_toffoli(self):
        config_fn = os.path.join(curdir, 'test_cfg_none_simple.json')
        platform  = ql.Platform('platform_none', config_fn)
        num_qubits = 4

        p = ql.Program('test_decompose_toffoli', platform, num_qubits)
        k = ql.Kernel('kernel1', platform, num_qubits)

        k.hadamard(2)
        k.toffoli(0, 1, 2)
        k.hadamard(2)

        p.add_kernel(k)
        ql.set_option('decompose_toffoli', 'NC')
        p.compile()

        gold_fn = curdir + '/golden/' + p.name +'_scheduled.qasm'
        qasm_fn = os.path.join(output_dir, p.name+'_scheduled.qasm')
        self.assertTrue(file_compare(qasm_fn, gold_fn))

if __name__ == '__main__':
    unittest.main()
