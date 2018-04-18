import os
import unittest
from openql import openql as ql
import numpy as np

rootDir = os.path.dirname(os.path.realpath(__file__))

curdir = os.path.dirname(__file__)
output_dir = os.path.join(curdir, 'test_output')
ql.set_output_dir(output_dir)

class Test_controlled_kernel(unittest.TestCase):
    # controlled-T requires an ancilla. At the moment qubit 0 is implicitly used
    def test_controlled_single_qubit_gates(self):
        config_fn = os.path.join(curdir, 'test_cfg_none_simple.json')
        platform  = ql.Platform('platform_none', config_fn)
        num_qubits = 4
        p = ql.Program('Test_controlled_single_qubit_gates', num_qubits, platform)

        k = ql.Kernel('kernel1', platform)
        ck = ql.Kernel('controlled_kernel1', platform)

        k.gate("x", [1])
        k.gate("y", [1])
        k.gate("z", [1])
        k.gate("h", [1])
        k.gate("i", [1])
        k.gate("s", [1])
        k.gate("t", [1])

        # generate controlled version of k. qubit 2 is used as control qubit
        ck.controlled(k, [2])

        p.add_kernel(k)
        p.add_kernel(ck)

        p.compile(optimize=False, scheduler='ASAP', log_level='LOG_INFO')

    def test_controlled_rotations(self):
        config_fn = os.path.join(curdir, 'test_cfg_none_simple.json')
        platform  = ql.Platform('platform_none', config_fn)
        num_qubits = 4
        p = ql.Program('Test_controlled_rotations', num_qubits, platform)

        k = ql.Kernel('kernel1', platform)
        ck = ql.Kernel('controlled_kernel1', platform)

        k.gate("rx", [1], angle=(np.pi)/4 )
        k.gate("ry", [1], angle=(np.pi)/4 )
        k.gate("rz", [1], angle=(np.pi)/4 )

        # generate controlled version of k. qubit 2 is used as control qubit        
        ck.controlled(k, [2])

        p.add_kernel(k)
        p.add_kernel(ck)

        p.compile(optimize=False, scheduler='ASAP', log_level='LOG_INFO')

    def test_controlled_two_qubit_gates(self):
        config_fn = os.path.join(curdir, 'test_cfg_none_simple.json')
        platform  = ql.Platform('platform_none', config_fn)
        num_qubits = 4
        p = ql.Program('Test_controlled_two_qubit_gates', num_qubits, platform)

        k = ql.Kernel('kernel1', platform)
        ck = ql.Kernel('controlled_kernel1', platform)

        k.gate("swap", [1, 2])

        # generate controlled version of k. qubit 3 is used as control qubit
        ck.controlled(k, [3])

        p.add_kernel(k)
        p.add_kernel(ck)

        p.compile(optimize=False, scheduler='ASAP', log_level='LOG_INFO')

if __name__ == '__main__':
    unittest.main()
