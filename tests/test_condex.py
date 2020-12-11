# tests for conditional execution
#
# tests python interface
#
# assumes config files: test_mapper_s7.json
#

from openql import openql as ql
import os
import unittest
from utils import file_compare


curdir = os.path.dirname(os.path.realpath(__file__))
output_dir = os.path.join(curdir, 'test_output')

class Test_condex(unittest.TestCase):

    def setUp(self):
        ql.set_option('output_dir', output_dir)     # this uses output_dir set above

        ql.set_option('log_level', 'LOG_NOTHING')
        ql.set_option('use_default_gates', 'no')
        
        ql.set_option('write_qasm_files', 'no')
        ql.set_option('write_report_files', 'no')

    def test_condex_mcx(self):
        # just check whether condex works for trivial case
        # parameters
        v = 'mcx'
        config = os.path.join(curdir, "test_mapper_s7.json")
        num_qubits = 7
        num_bregs = num_qubits

        # create and set platform
        prog_name = "test_condex_" + v
        kernel_name = "kernel_" + v
        starmon = ql.Platform("starmon", config)
        prog = ql.Program(prog_name, starmon, num_qubits, 0, num_bregs)
        k = ql.Kernel(kernel_name, starmon, num_qubits, 0, num_bregs)

        k.gate("measure", [0])
        k.condgate("x", [0], 'COND', [0])

        prog.add_kernel(k)
        prog.compile()

        gold_fn = curdir + '/golden/' + prog_name +'_last.qasm'
        qasm_fn = os.path.join(output_dir, prog_name+'_last.qasm')
        self.assertTrue( file_compare(qasm_fn, gold_fn) )

    def test_condex_mpccx(self):
        # check whether condex works with presetting condition and then creating an non-conditional gate
        # parameters
        v = 'mpccx'
        config = os.path.join(curdir, "test_mapper_s7.json")
        num_qubits = 7
        num_bregs = num_qubits

        # create and set platform
        prog_name = "test_condex_" + v
        kernel_name = "kernel_" + v
        starmon = ql.Platform("starmon", config)
        prog = ql.Program(prog_name, starmon, num_qubits, 0, num_bregs)
        k = ql.Kernel(kernel_name, starmon, num_qubits, 0, num_bregs)

        k.gate("measure", [0])
        k.gate_preset_condition('COND', [0])
        k.gate("x", [0])
        k.gate_clear_condition()
        k.gate("x", [0])

        prog.add_kernel(k)
        prog.compile()

        gold_fn = curdir + '/golden/' + prog_name +'_last.qasm'
        qasm_fn = os.path.join(output_dir, prog_name+'_last.qasm')
        self.assertTrue( file_compare(qasm_fn, gold_fn) )

    def test_condex_mccx(self):
        # check whether condex works with conditional gate created with base gate interface
        # parameters
        v = 'mccx'
        config = os.path.join(curdir, "test_mapper_s7.json")
        num_qubits = 7
        num_bregs = num_qubits

        # create and set platform
        prog_name = "test_condex_" + v
        kernel_name = "kernel_" + v
        starmon = ql.Platform("starmon", config)
        prog = ql.Program(prog_name, starmon, num_qubits, 0, num_bregs)
        k = ql.Kernel(kernel_name, starmon, num_qubits, 0, num_bregs)

        k.gate("measure", [0])
        k.gate("x", [0], 0, 0.0, [], 'COND', [0])

        prog.add_kernel(k)
        prog.compile()

        gold_fn = curdir + '/golden/' + prog_name +'_last.qasm'
        qasm_fn = os.path.join(output_dir, prog_name+'_last.qasm')
        self.assertTrue( file_compare(qasm_fn, gold_fn) )

    def test_condex_mbcx(self):
        # check whether condex works for measure storing in particular bit
        # parameters
        v = 'mbcx'
        config = os.path.join(curdir, "test_mapper_s7.json")
        num_qubits = 7
        num_bregs = num_qubits

        # create and set platform
        prog_name = "test_condex_" + v
        kernel_name = "kernel_" + v
        starmon = ql.Platform("starmon", config)
        prog = ql.Program(prog_name, starmon, num_qubits, 0, num_bregs)
        k = ql.Kernel(kernel_name, starmon, num_qubits, 0, num_bregs)

        k.gate("measure", [0], 0, 0.0, [1])
        k.condgate("x", [0], 'COND', [1])

        prog.add_kernel(k)
        prog.compile()

        gold_fn = curdir + '/golden/' + prog_name +'_last.qasm'
        qasm_fn = os.path.join(output_dir, prog_name+'_last.qasm')
        self.assertTrue( file_compare(qasm_fn, gold_fn) )

    def test_condex_mcnx(self):
        # check whether condex works for COND_NOT
        # parameters
        v = 'mcnx'
        config = os.path.join(curdir, "test_mapper_s7.json")
        num_qubits = 7
        num_bregs = num_qubits

        # create and set platform
        prog_name = "test_condex_" + v
        kernel_name = "kernel_" + v
        starmon = ql.Platform("starmon", config)
        prog = ql.Program(prog_name, starmon, num_qubits, 0, num_bregs)
        k = ql.Kernel(kernel_name, starmon, num_qubits, 0, num_bregs)

        k.gate("measure", [0])
        k.condgate("x", [0], 'COND_NOT', [0])

        prog.add_kernel(k)
        prog.compile()

        gold_fn = curdir + '/golden/' + prog_name +'_last.qasm'
        qasm_fn = os.path.join(output_dir, prog_name+'_last.qasm')
        self.assertTrue( file_compare(qasm_fn, gold_fn) )

    def test_condex_m2c2x(self):
        # check whether condex works for COND_AND on two measured bits
        # parameters
        v = 'm2c2x'
        config = os.path.join(curdir, "test_mapper_s7.json")
        num_qubits = 7
        num_bregs = num_qubits

        # create and set platform
        prog_name = "test_condex_" + v
        kernel_name = "kernel_" + v
        starmon = ql.Platform("starmon", config)
        prog = ql.Program(prog_name, starmon, num_qubits, 0, num_bregs)
        k = ql.Kernel(kernel_name, starmon, num_qubits, 0, num_bregs)

        k.gate("measure", [0])
        k.gate("measure", [1])
        k.condgate("x", [0], 'COND_AND', [0,1])

        prog.add_kernel(k)
        prog.compile()

        gold_fn = curdir + '/golden/' + prog_name +'_last.qasm'
        qasm_fn = os.path.join(output_dir, prog_name+'_last.qasm')
        self.assertTrue( file_compare(qasm_fn, gold_fn) )

if __name__ == '__main__':
    # ql.set_option('log_level', 'LOG_DEBUG')
    unittest.main()
