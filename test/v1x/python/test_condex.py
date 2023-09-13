# tests for conditional execution
#
# tests python interface
#
# assumes config files: test_mapper_s7.json
#

import openql as ql
import os
import unittest

from config import output_dir, qasm_golden_dir
from utils import file_compare


class TestCondex(unittest.TestCase):
    def setUp(self):
        ql.initialize()
        ql.set_option('output_dir', output_dir)  # this uses output_dir set above

        ql.set_option('log_level', 'LOG_NOTHING')
        ql.set_option('write_qasm_files', 'no')
        ql.set_option('write_report_files', 'no')

    def test_condex_basic(self):
        # Parameters
        v = 'basic'
        num_qubits = 7
        num_bregs = num_qubits

        # Create and set platform
        prog_name = "test_condex_" + v
        kernel_name = "kernel_" + v
        starmon = ql.Platform("starmon", "cc_light.s7")
        prog = ql.Program(prog_name, starmon, num_qubits, 0, num_bregs)
        k = ql.Kernel(kernel_name, starmon, num_qubits, 0, num_bregs)

        ql.set_option('use_default_gates', 'yes')

        # Measure q0 implicitly assigns b0, then x conditionally using cond gate on b0 on q0
        k.gate("measure", [0])
        k.condgate("x", [0], 'COND_UNARY', [0])

        # Measure q1 implicitly assigns b1, then x conditionally using preset_condition on b1 on q1
        k.gate("measure", [1])
        k.gate_preset_condition('COND_UNARY', [1])
        k.gate("x", [1])
        k.gate_clear_condition()
        k.gate("x", [1])

        # Measure q2 implicitly assigns b2, then x conditionally using explicit gate on b2 on q2
        k.gate("measure", [2])
        k.gate("x", [2], 0, 0.0, [], 'COND_UNARY', [2])

        # Measure q3 and explicitly assign b1, then x conditionally on b1 on q3
        k.gate("measure", [3], 0, 0.0, [1])
        k.condgate("x", [3], 'COND_UNARY', [1])

        # Measure q4 and then x conditionally on NOT b4 on q4
        k.gate("measure", [4])
        k.condgate("x", [4], 'COND_NOT', [4])

        # Measure q5 and then x conditionally NEVER and ALWAYS on q5
        k.gate("measure", [5])
        k.condgate("x", [5], 'COND_ALWAYS', [])
        k.condgate("x", [5], 'COND_NEVER', [])

        # Measure q0 and q1 and then x conditionally on b0 OP b1 on q0
        k.gate("measure", [0])
        k.gate("measure", [1])
        k.condgate("x", [0], 'COND_AND', [0, 1])
        k.condgate("x", [0], 'COND_NAND', [0, 1])
        k.condgate("x", [0], 'COND_OR', [0, 1])
        k.condgate("x", [0], 'COND_NOR', [0, 1])
        k.condgate("x", [0], 'COND_XOR', [0, 1])
        k.condgate("x", [0], 'COND_NXOR', [0, 1])

        prog.add_kernel(k)
        prog.compile()

        gold_fn = os.path.join(qasm_golden_dir, prog_name + '_scheduled.qasm')
        qasm_fn = os.path.join(output_dir, prog_name + '_scheduled.qasm')
        self.assertTrue(file_compare(qasm_fn, gold_fn))

        ql.set_option('use_default_gates', 'no')

    def test_condex_measure(self):
        # Parameters
        v = 'measure'
        num_qubits = 7
        num_bregs = num_qubits

        # Create and set platform
        prog_name = "test_condex_" + v
        kernel_name = "kernel_" + v
        starmon = ql.Platform("starmon", "cc_light.s7")
        prog = ql.Program(prog_name, starmon, num_qubits, 0, num_bregs)
        k = ql.Kernel(kernel_name, starmon, num_qubits, 0, num_bregs)

        # Conditional measure
        k.gate("measure", [0], 0, 0.0, [0])
        k.gate("measure", [1], 0, 0.0, [1], 'COND_UNARY', [0])
        k.gate("measure", [2], 0, 0.0, [2], 'COND_NOT', [0])
        k.condgate("x", [3], 'COND_UNARY', [1])
        k.condgate("y", [5], 'COND_UNARY', [2])

        prog.add_kernel(k)
        prog.compile()

        gold_fn = os.path.join(qasm_golden_dir, prog_name + '_last.qasm')
        qasm_fn = os.path.join(output_dir, prog_name + '_last.qasm')
        self.assertTrue(file_compare(qasm_fn, gold_fn))

    def test_condex_cnot(self):
        # Check whether condex works with conditional CNOT gate
        # which is to be decomposed by mapper parameters
        v = 'cnot'
        num_qubits = 7
        num_bregs = num_qubits

        # create and set platform
        prog_name = "test_condex_" + v
        kernel_name = "kernel_" + v
        starmon = ql.Platform("starmon", "cc_light.s7")
        prog = ql.Program(prog_name, starmon, num_qubits, 0, num_bregs)
        k = ql.Kernel(kernel_name, starmon, num_qubits, 0, num_bregs)

        ql.set_option('mapper', 'minextend')
        ql.set_option('mapassumezeroinitstate', 'no')
        ql.set_option('mapusemoves', 'yes')
        ql.set_option('maplookahead', 'noroutingfirst')
        ql.set_option('mappathselect', 'all')
        ql.set_option('maptiebreak', 'first')

        k.gate("cnot", [0, 1])

        k.gate("measure", [2])
        k.gate("cnot", [0, 2], 0, 0.0, [], 'COND_UNARY', [2])

        k.gate("measure", [2])
        k.condgate("cnot", [0, 5], 'COND_UNARY', [2])

        k.gate("measure", [2])
        k.gate_preset_condition('COND_UNARY', [2])
        k.gate("cnot", [0, 6])
        k.gate_clear_condition()

        prog.add_kernel(k)
        prog.compile()

        gold_fn = os.path.join(qasm_golden_dir, prog_name + '_last.qasm')
        qasm_fn = os.path.join(output_dir, prog_name + '_last.qasm')
        self.assertTrue(file_compare(qasm_fn, gold_fn))

        ql.set_option('mapper', 'no')

    def test_condex_toffoli_compos_gate(self):
        # Check whether condex works with conditional Toffoli gate
        # which is to be decomposed by toffoli decomposition parameters
        v = 'toffoli_compos_gate'
        num_qubits = 7
        num_bregs = num_qubits

        # create and set platform
        prog_name = "test_condex_" + v
        kernel_name = "kernel_" + v
        starmon = ql.Platform("starmon", "cc_light.s7")
        prog = ql.Program(prog_name, starmon, num_qubits, 0, num_bregs)
        k = ql.Kernel(kernel_name, starmon, num_qubits, 0, num_bregs)

        ql.set_option('mapper', 'minextend')
        ql.set_option('mapassumezeroinitstate', 'no')
        ql.set_option('mapusemoves', 'yes')
        ql.set_option('maplookahead', 'noroutingfirst')
        ql.set_option('mappathselect', 'all')
        ql.set_option('maptiebreak', 'first')

        k.gate("toffoli_decomp", [0, 1, 5])

        k.gate("measure", [0])
        k.gate("toffoli_decomp", [0, 1, 5], 0, 0.0, [], 'COND_UNARY', [0])

        k.gate("measure", [0])
        k.condgate("toffoli_decomp", [0, 1, 5], 'COND_UNARY', [0])

        k.gate("measure", [0])
        k.gate_preset_condition('COND_UNARY', [0])
        k.gate("toffoli_decomp", [0, 1, 5])
        k.gate_clear_condition()

        prog.add_kernel(k)
        prog.compile()

        gold_fn = os.path.join(qasm_golden_dir, prog_name + '_last.qasm')
        qasm_fn = os.path.join(output_dir, prog_name + '_last.qasm')
        self.assertTrue(file_compare(qasm_fn, gold_fn))

        ql.set_option('mapper', 'no')


if __name__ == '__main__':
    # ql.set_option('log_level', 'LOG_DEBUG')
    unittest.main()
