# tests for multi core
#
# assumes config files: test_multi_core_4x4_full.json
#

import openql as ql
import os
import unittest

from config import json_dir, output_dir, qasm_golden_dir
from utils import file_compare


class TestMultiCore(unittest.TestCase):
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
        ql.set_option('generate_code', 'no')

        ql.set_option('decompose_toffoli', 'no')
        ql.set_option('scheduler', 'ALAP')
        ql.set_option('scheduler_uniform', 'no')
        ql.set_option('scheduler_commute', 'yes')
        ql.set_option('scheduler_commute_rotations', 'yes')
        ql.set_option('prescheduler', 'yes')
        ql.set_option('cz_mode', 'manual')
        ql.set_option('print_dot_graphs', 'no')
        
        ql.set_option('clifford_premapper', 'yes')
        ql.set_option('clifford_postmapper', 'no')
        ql.set_option('mapper', 'minextend')
        ql.set_option('mapassumezeroinitstate', 'yes')
        ql.set_option('mapusemoves', 'yes')
        ql.set_option('mapreverseswap', 'yes')
        ql.set_option('mappathselect', 'all')
        ql.set_option('maplookahead', 'noroutingfirst')
        ql.set_option('maprecNN2q', 'no')
        ql.set_option('mapselectmaxlevel', '0')
        ql.set_option('mapselectmaxwidth', 'min')
        
        # ql.set_option('write_qasm_files', 'yes')
        # ql.set_option('write_report_files', 'yes')

    def test_mc_locals(self):
        v = 'locals'
        config_fn = os.path.join(json_dir, "test_multi_core_4x4_full.json")
        num_qubits = 16

        # create and set platform
        prog_name = "test_mc_" + v
        kernel_name = "kernel_" + v
        starmon = ql.Platform("mc4x4full", config_fn)
        prog = ql.Program(prog_name, starmon, num_qubits, 0)
        k = ql.Kernel(kernel_name, starmon, num_qubits, 0)

        for i in range(4):
            k.gate("x", [4*i])
            k.gate("x", [4*i+1])
        for i in range(4):
            k.gate("cnot", [4*i, 4*i+1])

        prog.add_kernel(k)
        prog.compile()

        gold_fn = os.path.join(qasm_golden_dir, prog_name + '_last.qasm')
        qasm_fn = os.path.join(output_dir, prog.name + '_last.qasm')
        self.assertTrue(file_compare(qasm_fn, gold_fn))

    def test_mc_non_comms(self):
        v = 'non_comms'
        config_fn = os.path.join(json_dir, "test_multi_core_4x4_full.json")
        num_qubits = 16

        # create and set platform
        prog_name = "test_mc_" + v
        kernel_name = "kernel_" + v
        starmon = ql.Platform("mc4x4full", config_fn)
        prog = ql.Program(prog_name, starmon, num_qubits, 0)
        k = ql.Kernel(kernel_name, starmon, num_qubits, 0)

        i = 0
        j = 1
        k.gate("x", [4*i+3])
        k.gate("x", [4*j+3])
        k.gate("cnot", [4*i+3, 4*j+3])

        prog.add_kernel(k)
        prog.compile()

        gold_fn = os.path.join(qasm_golden_dir, prog_name + '_last.qasm')
        qasm_fn = os.path.join(output_dir, prog.name + '_last.qasm')
        self.assertTrue(file_compare(qasm_fn, gold_fn))

    def test_mc_comms(self):
        v = 'comms'
        config_fn = os.path.join(json_dir, "test_multi_core_4x4_full.json")
        num_qubits = 16

        # create and set platform
        prog_name = "test_mc_" + v
        kernel_name = "kernel_" + v
        starmon = ql.Platform("mc4x4full", config_fn)
        prog = ql.Program(prog_name, starmon, num_qubits, 0)
        k = ql.Kernel(kernel_name, starmon, num_qubits, 0)

        i = 0
        j = 1
        k.gate("x", [4*i])
        k.gate("x", [4*j])
        k.gate("cnot", [4*i, 4*j])

        prog.add_kernel(k)
        prog.compile()

        gold_fn = os.path.join(qasm_golden_dir, prog_name + '_last.qasm')
        qasm_fn = os.path.join(output_dir, prog.name + '_last.qasm')
        self.assertTrue(file_compare(qasm_fn, gold_fn))

    def test_mc_all(self):
        v = 'all'
        config_fn = os.path.join(json_dir, "test_multi_core_4x4_full.json")
        num_qubits = 16

        # create and set platform
        prog_name = "test_mc_" + v
        kernel_name = "kernel_" + v
        starmon = ql.Platform("mc4x4full", config_fn)
        prog = ql.Program(prog_name, starmon, num_qubits, 0)
        k = ql.Kernel(kernel_name, starmon, num_qubits, 0)

        for i in range(4):
            k.gate("x", [4*i])
            k.gate("x", [4*i+1])
        for i in range(4):
            k.gate("cnot", [4*i, 4*i+1])
        for i in range(4):
            for j in range(4):
                if i != j:
                    k.gate("cnot", [4*i, 4*j])

        prog.add_kernel(k)
        prog.compile()

        gold_fn = os.path.join(qasm_golden_dir, prog_name + '_last.qasm')
        qasm_fn = os.path.join(output_dir, prog.name + '_last.qasm')
        self.assertTrue(file_compare(qasm_fn, gold_fn))

    def test_mc_all_saturate(self):
        v = 'all_saturate'
        config_fn = os.path.join(json_dir, "test_multi_core_4x4_full.json")
        num_qubits = 16

        # create and set platform
        prog_name = "test_mc_" + v
        kernel_name = "kernel_" + v
        starmon = ql.Platform("mc4x4full", config_fn)
        prog = ql.Program(prog_name, starmon, num_qubits, 0)
        k = ql.Kernel(kernel_name, starmon, num_qubits, 0)

        for i in range(4):
            k.gate("x", [4*i])
            k.gate("x", [4*i+1])
        for i in range(4):
            k.gate("cnot", [4*i, 4*i+1])
        for i in range(4):
            for j in range(4):
                if i != j:
                    k.gate("cnot", [4*i, 4*j])
                    k.gate("cnot", [4*i+1, 4*j+1])
                    k.gate("cnot", [4*i+2, 4*j+2])
                    k.gate("cnot", [4*i+3, 4*j+3])

        prog.add_kernel(k)
        prog.compile()

        gold_fn = os.path.join(qasm_golden_dir, prog_name + '_last.qasm')
        qasm_fn = os.path.join(output_dir, prog.name + '_last.qasm')
        self.assertTrue(file_compare(qasm_fn, gold_fn))


if __name__ == '__main__':
    # ql.set_option('log_level', 'LOG_DEBUG')
    unittest.main()