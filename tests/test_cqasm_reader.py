from openql import openql as ql
import unittest
import os
from utils import file_compare


curdir = os.path.dirname(__file__)
output_dir = os.path.join(curdir, 'test_output')


class TestcQasmReader(unittest.TestCase):

    @classmethod
    def setUp(self):
        ql.initialize()
        ql.set_option('output_dir', output_dir)
        # ql.set_option("write_qasm_files", "yes")

    def test_invalid_qasm(self):
        config_fn = os.path.join(curdir, 'hardware_config_cc_light.json')
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
        config_fn = os.path.join(curdir, 'hardware_config_cc_light.json')
        platform = ql.Platform('seven_qubits_chip', config_fn)
        number_qubits = platform.get_qubit_number()
        name = 'test_single_bit_kernel_operations'
        program = ql.Program(name, platform, number_qubits)
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
        self.assertTrue(file_compare(os.path.join(output_dir, name + '.qasm'), os.path.join(curdir, 'golden', name + '.qasm')))

    def test_sub_circuit_programs(self):
        config_fn = os.path.join(curdir, 'hardware_config_cc_light.json')
        platform = ql.Platform('seven_qubits_chip', config_fn)
        number_qubits = platform.get_qubit_number()
        name = 'test_sub_circuit_programs'
        program = ql.Program(name, platform, number_qubits)
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
        self.assertTrue(file_compare(os.path.join(output_dir, name + '.qasm'), os.path.join(curdir, 'golden', name + '.qasm')))

    def test_parallel_programs(self):
        config_fn = os.path.join(curdir, 'hardware_config_cc_light.json')
        platform = ql.Platform('seven_qubits_chip', config_fn)
        number_qubits = platform.get_qubit_number()
        name = 'test_parallel_programs'
        program = ql.Program(name, platform, number_qubits)
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
        self.assertTrue(file_compare(os.path.join(output_dir, name + '.qasm'), os.path.join(curdir, 'golden', name + '.qasm')))

    def test_multiple_programs(self):
        config_fn = os.path.join(curdir, 'hardware_config_cc_light.json')
        platform = ql.Platform('seven_qubits_chip', config_fn)
        number_qubits = platform.get_qubit_number()
        name = 'test_multiple_programs'
        program = ql.Program(name, platform, number_qubits)
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
        self.assertTrue(file_compare(os.path.join(output_dir, name + '.qasm'), os.path.join(curdir, 'golden', name + '.qasm')))

    def test_conditions(self):
        config_fn = os.path.join(curdir, 'hardware_config_cc_light.json')
        cqasm_config_fn = os.path.join(curdir, 'cqasm_config_cc_light.json')
        platform = ql.Platform('seven_qubits_chip', config_fn)
        number_qubits = platform.get_qubit_number()
        name = 'test_cqasm_conditions'
        program = ql.Program(name, platform, number_qubits)
        qasm_rdr = ql.cQasmReader(platform, program, cqasm_config_fn)
        qasm_str = "version 1.1\n"                  \
                   "var qa, qb: qubit\n"            \
                   "var ca, cb: bool\n"             \
                   "measure qa, ca\n"               \
                   "measure qb, cb\n"               \
                   "cond(true) x qa\n"              \
                   "cond(false) y qa\n"             \
                   "cond(ca) z qa\n"                \
                   "cond(!true) x qa\n"             \
                   "cond(!false) y qa\n"            \
                   "cond(!ca) z qa\n"               \
                   "cond(!!true) x qa\n"            \
                   "cond(!!false) y qa\n"           \
                   "cond(!!ca) z qa\n"              \
                   "cond(ca && cb) x qa\n"          \
                   "cond(ca && true) y qa\n"        \
                   "cond(ca && false) z qa\n"       \
                   "cond(true && cb) x qa\n"        \
                   "cond(false && cb) y qa\n"       \
                   "cond(ca || cb) z qa\n"          \
                   "cond(ca || true) x qa\n"        \
                   "cond(ca || false) y qa\n"       \
                   "cond(true || cb) z qa\n"        \
                   "cond(false || cb) x qa\n"       \
                   "cond(ca ^^ cb) y qa\n"          \
                   "cond(ca ^^ true) z qa\n"        \
                   "cond(ca ^^ false) x qa\n"       \
                   "cond(true ^^ cb) y qa\n"        \
                   "cond(false ^^ cb) z qa\n"       \
                   "cond(!(ca && cb)) x qa\n"       \
                   "cond(!(ca && true)) y qa\n"     \
                   "cond(!(ca && false)) z qa\n"    \
                   "cond(!(true && cb)) x qa\n"     \
                   "cond(!(false && cb)) y qa\n"    \
                   "cond(!(ca || cb)) z qa\n"       \
                   "cond(!(ca || true)) x qa\n"     \
                   "cond(!(ca || false)) y qa\n"    \
                   "cond(!(true || cb)) z qa\n"     \
                   "cond(!(false || cb)) x qa\n"    \
                   "cond(!(ca ^^ cb)) y qa\n"       \
                   "cond(!(ca ^^ true)) z qa\n"     \
                   "cond(!(ca ^^ false)) x qa\n"    \
                   "cond(!(true ^^ cb)) y qa\n"     \
                   "cond(!(false ^^ cb)) z qa\n"
        qasm_rdr.string2circuit(qasm_str)
        program.compile()
        self.assertTrue(file_compare(os.path.join(output_dir, name + '.qasm'), os.path.join(curdir, 'golden', name + '.qasm')))


if __name__ == '__main__':
    unittest.main()
