# tests flux qubit parking
#
# assumes config files: test_mapper_s7.json
#

import openql as ql
import os
import unittest

from config import output_dir, qasm_golden_dir
from utils import file_compare


class TestMapper(unittest.TestCase):
    def setUp(self):
        ql.initialize()
        # uses defaults of options in mapper branch except for output_dir and for maptiebreak
        # this uses output_dir set above
        ql.set_option('output_dir', output_dir)
        # this makes behavior deterministic to cmp with golden and deviates from default
        ql.set_option('maptiebreak', 'first')

        ql.set_option('log_level', 'LOG_NOTHING')
        ql.set_option('optimize', 'no')
        ql.set_option('use_default_gates', 'no')
        ql.set_option('decompose_toffoli', 'no')
        ql.set_option('scheduler', 'ALAP')
        ql.set_option('scheduler_uniform', 'no')
        ql.set_option('scheduler_commute', 'yes')
        ql.set_option('prescheduler', 'yes')
        ql.set_option('cz_mode', 'auto')
        ql.set_option('print_dot_graphs', 'no')
        
        ql.set_option('mapper', 'no')
        ql.set_option('clifford_postmapper', 'yes')

        ql.set_option('generate_code', 'no')
        
        ql.set_option('write_qasm_files', 'no')
        ql.set_option('write_report_files', 'no')

    def test_flux_all(self):
        v = 'all'
        num_qubits = 7

        # create and set platform
        prog_name = "test_flux_" + v
        kernel_name = "kernel_" + v
        starmon = ql.Platform("starmon", "cc_light.s7")
        prog = ql.Program(prog_name, starmon, num_qubits, 0)
        k = ql.Kernel(kernel_name, starmon, num_qubits, 0)

        for j in range(7):
            k.gate("x", [j])
        k.gate("cz", [0, 2])
        k.gate("cz", [0, 3])
        k.gate("cz", [1, 3])
        k.gate("cz", [1, 4])
        k.gate("cz", [2, 0])
        k.gate("cz", [2, 5])
        k.gate("cz", [3, 0])
        k.gate("cz", [3, 1])
        k.gate("cz", [3, 5])
        k.gate("cz", [3, 6])
        k.gate("cz", [4, 1])
        k.gate("cz", [4, 6])
        k.gate("cz", [5, 2])
        k.gate("cz", [5, 3])
        k.gate("cz", [6, 3])
        k.gate("cz", [6, 4])
        for j in range(7):
            k.gate("x", [j])

        prog.add_kernel(k)
        prog.compile()

        gold_fn = os.path.join(qasm_golden_dir, prog_name + '_last.qasm')
        qasm_fn = os.path.join(output_dir, prog_name + '_last.qasm')
        self.assertTrue(file_compare(qasm_fn, gold_fn))


if __name__ == '__main__':
    # ql.set_option('log_level', 'LOG_DEBUG')
    unittest.main()
