import openql as ql
import os
import unittest

from config import cq_dir, json_dir, output_dir, qasm_golden_dir
from utils import file_compare


class TestQI2IntegrationTest(unittest.TestCase):
    @classmethod
    def setUp(cls):
        ql.initialize()
        ql.set_option('output_dir', output_dir)

    def test_instruction_not_registered(self):
        platform = ql.Platform("qi2_integration_test", os.path.join(json_dir, "spin-4.json"))
        compiler = platform.get_compiler()
        compiler.prefix_pass("io.cqasm.Read", "input", {
            "cqasm_file": os.path.join(cq_dir, "test_instruction_not_registered.cq")
        })
        program = ql.Program("test_instruction_not_registered", platform)
        program.get_compiler().insert_pass_after("input", "dec.Instructions", "decomposition")
        program.get_compiler().set_option("initialqasmwriter.cqasm_version", "3.0")
        program.get_compiler().set_option("initialqasmwriter.with_metadata", "no")

        with self.assertRaisesRegex(RuntimeError, r"""ERROR
Error: failed to resolve x90
Error: failed to resolve cnot"""):
            program.compile()


    def test_x90_q12__cnot_q1_q0(self):
        platform = ql.Platform("qi2_integration_test", os.path.join(json_dir, "spin-4.json"))
        compiler = platform.get_compiler()
        compiler.prefix_pass("io.cqasm.Read", "input", {
            "cqasm_file": os.path.join(cq_dir, "test_x90_q12__cnot_q1_q0.cq")
        })
        program = ql.Program("test_x90_q12__cnot_q1_q0", platform)
        program.get_compiler().insert_pass_after("input", "dec.Instructions", "decomposition")
        program.get_compiler().set_option("initialqasmwriter.cqasm_version", "3.0")
        program.get_compiler().set_option("initialqasmwriter.with_metadata", "no")
        program.compile()

        output_file = os.path.join(output_dir, "test_x90_q12__cnot_q1_q0.qasm")
        golden_file = os.path.join(qasm_golden_dir, "test_x90_q12__cnot_q1_q0.qasm")
        self.assertTrue(file_compare(output_file, golden_file))


if __name__ == '__main__':
    unittest.main()
