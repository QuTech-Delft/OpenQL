import openql as ql
import os
import unittest

from config import cq_dir, output_dir, qasm_golden_dir
from utils import file_compare


class TestDiamond(unittest.TestCase):
    def test_diamond_api(self):
        ql.initialize()
        ql.set_option('output_dir', output_dir)
        ql.set_option('prescheduler', 'no')

        platform = ql.Platform('diamond_test', 'diamond')

        num_qubits = 3
        p = ql.Program('diamond_api', platform, num_qubits)
        k = ql.Kernel('kernel', platform, num_qubits)

        # Initialization
        k.gate('prep_z', [0])
        k.gate('prep_x', [0])
        k.gate('prep_y', [0])
        k.gate('initialize', [0])

        # Measurement
        k.gate('measure', [0])
        k.gate('measure_z', [0])
        k.gate('measure_x', [0])
        k.gate('measure_y', [0])

        # single qubit gates
        k.x(0)
        k.y(0)
        k.z(0)
        k.s(0)
        k.t(0)

        # two qubit gates 
        k.gate('cnot', [0, 1])  # only between color center - nuclear spin qubit
        k.gate('cz', [0, 1])

        # three qubit gate
        k.gate('toffoli', [0, 1, 2])

        # calibration
        k.gate('cal_measure', [0])
        k.gate('cal_pi', [0])
        k.gate('cal_halfpi', [0])
        k.gate('decouple', [0])

        # custom rotations
        k.gate('rx', [0], 0, 1.57)
        k.gate('ry', [0], 0, 1.57)
        k.gate('crk', [0, 1], 0, 1)
        k.gate('cr', [0, 1], 0, 3.14)

        # diamond protocols/sequences
        k.diamond_crc(0, 30, 5)
        k.diamond_rabi_check(0, 100, 2, 3)  # qubit, duration, measurement, t_max
        k.diamond_excite_mw(1, 100, 200, 0, 60, 0)  # envelope, duration, frequency, phase, amplitude, qubit
        k.diamond_qentangle(0, 15)  # qubit, nuclear qubit
        k.gate('nventangle', [0, 1])
        k.diamond_memswap(0, 1)  # qubit, nuclear qubit
        k.diamond_sweep_bias(0, 10, 0, 0, 10, 100, 0)  # qubit, value, dacreg, start, step, max, memaddress

        # timing
        k.gate('wait', [], 200)
        k.gate('qnop', [0])  # qubit, mandatory from openQL

        # calculate bias
        k.gate('calculate_current', [0])

        # calculate voltage
        k.gate('calculate_voltage', [0])

        # magnetic bias check
        k.diamond_sweep_bias(0, 10, 0, 0, 10, 100, 0)
        k.gate('calculate_voltage', [0])

        p.add_kernel(k)
        p.compile()

        gold_fn = os.path.join(qasm_golden_dir, p.name + '_scheduled.qasm')
        qasm_fn = os.path.join(output_dir, p.name + '_scheduled.qasm')
        self.assertTrue(file_compare(qasm_fn, gold_fn))

        gold_fn = os.path.join(qasm_golden_dir, p.name + '_diamond_codegen.dqasm')
        qasm_fn = os.path.join(output_dir, p.name + '_diamond_codegen.dqasm')
        self.assertTrue(file_compare(qasm_fn, gold_fn))

    def test_diamond_cqasm(self):
        ql.initialize()
        ql.set_option('output_dir', output_dir)
        ql.set_option('prescheduler', 'no')

        platform = ql.Platform('diamond_test', 'diamond')
        compiler = platform.get_compiler()
        compiler.prefix_pass(
            'io.cqasm.Read',
            '',
            {
                'cqasm_file': os.path.join(cq_dir, 'test_diamond.cq'),
                'measure_all_target': 'measure_z'
            }
        )
        compiler.compile_with_frontend(platform)

        gold_fn = os.path.join(qasm_golden_dir, 'diamond_cqasm_scheduled.qasm')
        qasm_fn = os.path.join(output_dir, 'diamond_cqasm_scheduled.qasm')
        self.assertTrue(file_compare(qasm_fn, gold_fn))

        gold_fn = os.path.join(qasm_golden_dir, 'diamond_cqasm_diamond_codegen.dqasm')
        qasm_fn = os.path.join(output_dir, 'diamond_cqasm_diamond_codegen.dqasm')
        self.assertTrue(file_compare(qasm_fn, gold_fn))


if __name__ == '__main__':
    unittest.main()
