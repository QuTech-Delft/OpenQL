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
        ql.initialize()
        ql.set_option('output_dir', output_dir)     # this uses output_dir set above

        ql.set_option('log_level', 'LOG_NOTHING')
        ql.set_option('write_qasm_files', 'no')
        ql.set_option('write_report_files', 'no')

    def test_condex_basic(self):
        # parameters
        v = 'basic'
        config = os.path.join(curdir, "test_mapper_s7.json")
        num_qubits = 7
        num_bregs = num_qubits

        # create and set platform
        prog_name = "test_condex_" + v
        kernel_name = "kernel_" + v
        starmon = ql.Platform("starmon", config)
        prog = ql.Program(prog_name, starmon, num_qubits, 0, num_bregs)
        k = ql.Kernel(kernel_name, starmon, num_qubits, 0, num_bregs)

        ql.set_option('use_default_gates', 'yes')

        # measure q0 implicitly assigns b0, then x conditionally using condgate on b0 on q0
        k.gate("measure", [0])
        k.condgate("x", [0], 'COND_UNARY', [0])

        # measure q1 implicitly assigns b1, then x conditionally using preset_condition on b1 on q1
        k.gate("measure", [1])
        k.gate_preset_condition('COND_UNARY', [1])
        k.gate("x", [1])
        k.gate_clear_condition()
        k.gate("x", [1])

        # measure q2 implicitly assigns b2, then x conditionally using explicit gate on b2 on q2
        k.gate("measure", [2])
        k.gate("x", [2], 0, 0.0, [], 'COND_UNARY', [2])

        # measure q3 and explicitly assign b1, then x conditionally on b1 on q3
        k.gate("measure", [3], 0, 0.0, [1])
        k.condgate("x", [3], 'COND_UNARY', [1])

        # measure q4 and then x conditionally on NOT b4 on q4
        k.gate("measure", [4])
        k.condgate("x", [4], 'COND_NOT', [4])

        # measure q5 and then x conditionally NEVER and ALWAYS on q5
        k.gate("measure", [5])
        k.condgate("x", [5], 'COND_ALWAYS', [])
        k.condgate("x", [5], 'COND_NEVER', [])

        # measure q0 and q1 and then x conditionally on b0 OP b1 on q0
        k.gate("measure", [0])
        k.gate("measure", [1])
        k.condgate("x", [0], 'COND_AND', [0,1])
        k.condgate("x", [0], 'COND_NAND', [0,1])
        k.condgate("x", [0], 'COND_OR', [0,1])
        k.condgate("x", [0], 'COND_NOR', [0,1])
        k.condgate("x", [0], 'COND_XOR', [0,1])
        k.condgate("x", [0], 'COND_NXOR', [0,1])

        prog.add_kernel(k)
        prog.compile()

        gold_fn = curdir + '/golden/' + prog_name +'_scheduled.qasm'
        qasm_fn = os.path.join(output_dir, prog_name+'_scheduled.qasm')
        self.assertTrue( file_compare(qasm_fn, gold_fn) )

        ql.set_option('use_default_gates', 'no')

    def test_condex_measure(self):
        # parameters
        v = 'measure'
        config = os.path.join(curdir, "test_mapper_s7.json")
        num_qubits = 7
        num_bregs = num_qubits

        # create and set platform
        prog_name = "test_condex_" + v
        kernel_name = "kernel_" + v
        starmon = ql.Platform("starmon", config)
        prog = ql.Program(prog_name, starmon, num_qubits, 0, num_bregs)
        k = ql.Kernel(kernel_name, starmon, num_qubits, 0, num_bregs)

        # conditional measure
        k.gate("measure", [0], 0, 0.0, [0])
        k.gate("measure", [1], 0, 0.0, [1], 'COND_UNARY', [0])
        k.gate("measure", [2], 0, 0.0, [2], 'COND_NOT', [0])
        k.condgate("x", [3], 'COND_UNARY', [1])
        k.condgate("y", [5], 'COND_UNARY', [2])

        prog.add_kernel(k)
        prog.compile()

        gold_fn = curdir + '/golden/' + prog_name +'_last.qasm'
        qasm_fn = os.path.join(output_dir, prog_name+'_last.qasm')
        self.assertTrue( file_compare(qasm_fn, gold_fn) )

    def test_condex_cnot(self):
        # check whether condex works with conditional cnot gate which is to be decomposed by mapper
        # parameters
        v = 'cnot'
        config = os.path.join(curdir, "test_mapper_s7.json")
        num_qubits = 7
        num_bregs = num_qubits

        # create and set platform
        prog_name = "test_condex_" + v
        kernel_name = "kernel_" + v
        starmon = ql.Platform("starmon", config)
        prog = ql.Program(prog_name, starmon, num_qubits, 0, num_bregs)
        k = ql.Kernel(kernel_name, starmon, num_qubits, 0, num_bregs)

        ql.set_option('mapper', 'minextend')
        ql.set_option('mapassumezeroinitstate', 'no')
        ql.set_option('mapinitone2one', 'yes')
        ql.set_option('mapusemoves', 'yes')
        ql.set_option('maplookahead', 'noroutingfirst')
        ql.set_option('mappathselect', 'all')
        ql.set_option('maptiebreak', 'first')

        k.gate("cnot", [0,1])

        k.gate("measure", [2])
        k.gate("cnot", [0,2], 0, 0.0, [], 'COND_UNARY', [2])

        k.gate("measure", [2])
        k.condgate("cnot", [0,5], 'COND_UNARY', [2])

        k.gate("measure", [2])
        k.gate_preset_condition('COND_UNARY', [2])
        k.gate("cnot", [0,6])
        k.gate_clear_condition()

        prog.add_kernel(k)
        prog.compile()

        gold_fn = curdir + '/golden/' + prog_name +'_last.qasm'
        qasm_fn = os.path.join(output_dir, prog_name+'_last.qasm')
        self.assertTrue( file_compare(qasm_fn, gold_fn) )

        ql.set_option('mapper', 'no')

    def test_condex_toffoli_pass(self):
        # check whether condex works with conditional toffoli gate which is to be decomposed by toffoli decomposition
        # parameters
        v = 'toffoli_pass'
        config = os.path.join(curdir, "test_mapper_s7.json")
        num_qubits = 7
        num_bregs = num_qubits

        # create and set platform
        prog_name = "test_condex_" + v
        kernel_name = "kernel_" + v
        starmon = ql.Platform("starmon", config)
        prog = ql.Program(prog_name, starmon, num_qubits, 0, num_bregs)
        k = ql.Kernel(kernel_name, starmon, num_qubits, 0, num_bregs)

        ql.set_option('decompose_toffoli', 'AM')
        ql.set_option('use_default_gates', 'yes')

        ql.set_option('mapper', 'minextend')
        ql.set_option('mapassumezeroinitstate', 'no')
        ql.set_option('mapinitone2one', 'yes')
        ql.set_option('mapusemoves', 'yes')
        ql.set_option('maplookahead', 'noroutingfirst')
        ql.set_option('mappathselect', 'all')
        ql.set_option('maptiebreak', 'first')

        k.gate("toffoli", [0,1,5])

        k.gate("measure", [0])
        k.gate("toffoli", [0,1,5], 0, 0.0, [], 'COND_UNARY', [0])

        k.gate("measure", [0])
        k.condgate("toffoli", [0,1,5], 'COND_UNARY', [0])

        k.gate("measure", [0])
        k.gate_preset_condition('COND_UNARY', [0])
        k.toffoli(0,1,5)
        k.gate_clear_condition()

        prog.add_kernel(k)
        prog.compile()

        gold_fn = curdir + '/golden/' + prog_name +'_last.qasm'
        qasm_fn = os.path.join(output_dir, prog_name+'_last.qasm')
        self.assertTrue( file_compare(qasm_fn, gold_fn) )

        ql.set_option('mapper', 'no')
        ql.set_option('decompose_toffoli', 'no')
        ql.set_option('use_default_gates', 'no')

    def test_condex_toffoli_composgate(self):
        # check whether condex works with conditional toffoli gate which is to be decomposed by toffoli decomposition
        # parameters
        v = 'toffoli_composgate'
        config = os.path.join(curdir, "test_mapper_s7.json")
        num_qubits = 7
        num_bregs = num_qubits

        # create and set platform
        prog_name = "test_condex_" + v
        kernel_name = "kernel_" + v
        starmon = ql.Platform("starmon", config)
        prog = ql.Program(prog_name, starmon, num_qubits, 0, num_bregs)
        k = ql.Kernel(kernel_name, starmon, num_qubits, 0, num_bregs)

        ql.set_option('mapper', 'minextend')
        ql.set_option('mapassumezeroinitstate', 'no')
        ql.set_option('mapinitone2one', 'yes')
        ql.set_option('mapusemoves', 'yes')
        ql.set_option('maplookahead', 'noroutingfirst')
        ql.set_option('mappathselect', 'all')
        ql.set_option('maptiebreak', 'first')

        k.gate("toffoli_decomp", [0,1,5])

        k.gate("measure", [0])
        k.gate("toffoli_decomp", [0,1,5], 0, 0.0, [], 'COND_UNARY', [0])

        k.gate("measure", [0])
        k.condgate("toffoli_decomp", [0,1,5], 'COND_UNARY', [0])

        k.gate("measure", [0])
        k.gate_preset_condition('COND_UNARY', [0])
        k.gate("toffoli_decomp", [0,1,5])
        k.gate_clear_condition()

        prog.add_kernel(k)
        prog.compile()

        gold_fn = curdir + '/golden/' + prog_name +'_last.qasm'
        qasm_fn = os.path.join(output_dir, prog_name+'_last.qasm')
        self.assertTrue( file_compare(qasm_fn, gold_fn) )

        ql.set_option('mapper', 'no')

if __name__ == '__main__':
    # ql.set_option('log_level', 'LOG_DEBUG')
    unittest.main()
