# tests for cqasm skip n
#
# assumes config files: test_mapper_s7.json
#

from openql import openql as ql
import os
from test_QISA_assembler_present import assemble
import unittest
from utils import file_compare


rootDir = os.path.dirname(os.path.realpath(__file__))
curdir = os.path.dirname(__file__)
output_dir = os.path.join(curdir, 'test_output')

class Test_skip(unittest.TestCase):

    def setUp(self):
        ql.set_option('output_dir', output_dir)     # this uses output_dir set above

        ql.set_option('log_level', 'LOG_NOTHING')
        ql.set_option('write_qasm_files', 'yes')
        ql.set_option('write_report_files', 'no')
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

    def test_skip_yes(self):
        self.setUp()
        # just check whether skip works for trivial case
        # parameters
        ql.set_option('issue_skip_319', 'yes')
        v = 'yes'
        config = os.path.join(rootDir, "test_mapper_s7.json")
        num_qubits = 7

        # create and set platform
        prog_name = "test_skip_" + v
        kernel_name = "kernel_" + v
        starmon = ql.Platform("starmon", config)
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

        GOLD_fn = os.path.join(rootDir, 'golden', prog.name + '_scheduled.qasm')
        QASM_fn = os.path.join(output_dir, prog.name+'_scheduled.qasm')

        assemble(QASM_fn)
        self.assertTrue(file_compare(QASM_fn, GOLD_fn))

        ql.set_option('write_qasm_files', 'no')
        ql.set_option('write_report_files', 'no')
        ql.set_option('unique_output', 'no')
        ql.set_option('mapper', 'no')

        ql.set_option('issue_skip_319', 'no')

if __name__ == '__main__':
    # ql.set_option('log_level', 'LOG_DEBUG')
    unittest.main()
