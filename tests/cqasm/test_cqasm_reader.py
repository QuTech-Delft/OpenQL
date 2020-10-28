import os
import unittest

from openql import openql as ql

rootDir = os.path.dirname(os.path.realpath(__file__))
curdir = os.path.dirname(__file__)
output_dir = os.path.join(curdir, 'test_output')


class TestcQasmReader(unittest.TestCase):

    @classmethod
    def setUpClass(self):
        ql.set_option('output_dir', output_dir)
        ql.set_option("write_qasm_files", "yes")

    def test_invalid_qasm(self):
        config_fn = os.path.join(curdir, '../hardware_config_cc_light.json')
        platform = ql.Platform('seven_qubits_chip', config_fn)
        number_qubits = platform.get_qubit_number()
        program = ql.Program('test_invalid_qasm', platform, number_qubits)
        qasm_str = "version 1.0\n"  \
                   "qubits 1\n"     \
                   "prop_z q[0]\n"    \
                   "measure_z q[0]\n"
        qasm_rdr = ql.cQasmReader(platform, program)
        with self.assertRaisesRegex(RuntimeError, r"Error at <unknown>:3:1..12: failed to resolve prop_z"):
            qasm_rdr.string2circuit(qasm_str)

    def test_single_bit_kernel_operations(self):
        config_fn = os.path.join(curdir, '../hardware_config_cc_light.json')
        platform = ql.Platform('seven_qubits_chip', config_fn)
        number_qubits = platform.get_qubit_number()
        program = ql.Program('test_single_bit_kernel_operations', platform, number_qubits)
        qasm_rdr = ql.cQasmReader(platform, program)
        qasm_str = "version 1.0\n"  \
                   "qubits 5\n"     \
                   "error_model depolarizing_channel, 0.001\n"  \
                   "prep_z q[0:3]\n"    \
                   "prep_z q[4]\n"  \
                   "i q[0:3]\n"     \
                   "h q[4]\n"       \
                   "x q[1:3]\n"     \
                   "z q[0:1]\n"     \
                   "s q[1]\n"       \
                   "sdag q[2:3]\n"  \
                   "tdag q[1:3]\n"  \
                   "x90 q[3]\n"     \
                   "mx90 q[1]\n"    \
                   "y90 q[0]\n"     \
                   "my90 q[2:3]\n"  \
                   "measure_z q[2:3]\n" \
                   "measure_z q[0, 1]\n"    \
                   "measure_z q[4]\n"
        qasm_rdr.string2circuit(qasm_str)
        program.compile()

    def test_sub_circuit_programs(self):
        config_fn = os.path.join(curdir, '../hardware_config_cc_light.json')
        platform = ql.Platform('seven_qubits_chip', config_fn)
        number_qubits = platform.get_qubit_number()
        program = ql.Program('test_sub_circuit_programs', platform, number_qubits)
        qasm_rdr = ql.cQasmReader(platform, program)
        qasm_str = "version 1.0\n"  \
                   "qubits 6\n"     \
                   ".init\n"        \
                   "  prep_z q[0]\n"    \
                   "  prep_z q[1]\n"    \
                   ".do_somework(3)\n"   \
                   "  x q[0]\n" \
                   "  h q[1]\n" \
                   ".do_measurement\n"   \
                   "  measure_all\n"
        qasm_rdr.string2circuit(qasm_str)
        program.compile()

    def test_parallel_programs(self):
        config_fn = os.path.join(curdir, '../hardware_config_cc_light.json')
        platform = ql.Platform('seven_qubits_chip', config_fn)
        number_qubits = platform.get_qubit_number()
        program = ql.Program('test_parallel_programs', platform, number_qubits)
        qasm_rdr = ql.cQasmReader(platform, program)
        qasm_str = "version 1.0\n"  \
                   "qubits 6\n"     \
                   ".init\n"        \
                   "  { prep_z q[0] | prep_z q[1] }\n"  \
                   ".do_somework(3)\n"   \
                   "  { x q[0] | h q[1] }\n"    \
                   ".do_measurement\n"   \
                   "  { measure_z q[0] | measure_z q[1] }\n"
        qasm_rdr.string2circuit(qasm_str)
        program.compile()

    def test_multiple_programs(self):
        config_fn = os.path.join(curdir, '../hardware_config_cc_light.json')
        platform = ql.Platform('seven_qubits_chip', config_fn)
        number_qubits = platform.get_qubit_number()
        program = ql.Program('test_multiple_programs', platform, number_qubits)
        qasm_rdr = ql.cQasmReader(platform, program)
        qasm_str = "version 1.0\n"  \
                   "qubits 6\n"     \
                   "prep_z q[0]\n"    \
                   "prep_z q[1]\n"    \
                   "x q[0]\n" \
                   "h q[1]\n"
        qasm_rdr.string2circuit(qasm_str)
        qasm_str = "version 1.0\n"  \
                   "qubits 6\n"     \
                   "prep_z q[2]\n"    \
                   "prep_z q[3]\n"    \
                   "x q[2]\n" \
                   "h q[3]\n" \
                   "measure_all\n"
        qasm_rdr.string2circuit(qasm_str)
        program.compile()


if __name__ == '__main__':
    unittest.main()
