# tests for cqasm skip n
#
# assumes config files: test_mapper_s7.json
#

import openql as ql
import os
import unittest

from config import output_dir, qasm_golden_dir
from utils import file_compare


class TestSkip(unittest.TestCase):
    def setUp(self):
        ql.initialize()

        ql.set_option('output_dir', output_dir)     # this uses output_dir set above

        ql.set_option('log_level', 'LOG_NOTHING')
        # ql.set_option('write_qasm_files', 'yes')
        # ql.set_option('write_report_files', 'no')
        ql.set_option('unique_output', 'no')

        ql.set_option('optimize', 'no')
        ql.set_option('use_default_gates', 'no')
        ql.set_option('decompose_toffoli', 'no')
        ql.set_option('scheduler', 'ALAP')
        ql.set_option('scheduler_uniform', 'no')
        ql.set_option('scheduler_commute', 'yes')
        ql.set_option('prescheduler', 'yes')
        ql.set_option('cz_mode', 'manual')
        ql.set_option('print_dot_graphs', 'no')
        ql.set_option('mapper', 'no')

    @unittest.skip("QI2 integration test: temporarily skipped this test")
    def test_skip_yes(self):
        # just check whether skip works for trivial case
        # parameters
        ql.set_option('issue_skip_319', 'yes')
        v = 'yes'
        config_fn = "cc_light.s7"
        num_qubits = 7

        # create and set platform
        prog_name = "test_skip_" + v
        kernel_name = "kernel_" + v
        starmon = ql.Platform("starmon", config_fn)
        prog = ql.Program(prog_name, starmon, num_qubits, 0)

        qasm_rdr = ql.cQasmReader(starmon, prog)
        qasm_str = """version 1.0
            qubits 7
            .{kernel_name}
            cnot q[2],q[5]
            skip 2
            {{ x q[0] | y q[1] }}
            """.format(kernel_name=kernel_name)

        qasm_rdr.string2circuit(qasm_str)

        prog.compile()

        gold_fn = os.path.join(qasm_golden_dir, prog.name + '_scheduled.qasm')
        qasm_fn = os.path.join(output_dir, prog.name + '_scheduled.qasm')

        self.assertTrue(file_compare(qasm_fn, gold_fn))

        # ql.set_option('write_qasm_files', 'no')
        # ql.set_option('write_report_files', 'no')
        ql.set_option('unique_output', 'no')
        ql.set_option('mapper', 'no')

        ql.set_option('issue_skip_319', 'no')


if __name__ == '__main__':
    # ql.set_option('log_level', 'LOG_DEBUG')
    unittest.main()
