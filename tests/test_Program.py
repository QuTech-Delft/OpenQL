import os
import unittest
from openql import openql as ql

curdir = os.path.dirname(os.path.realpath(__file__))
config_fn = os.path.join(curdir, 'test_config_default.json')
platf = ql.Platform("starmon", config_fn)

output_dir = os.path.join(curdir, 'test_output')

class Test_program(unittest.TestCase):

    @classmethod
    def setUp(self):
        ql.initialize()
        ql.set_option('output_dir', output_dir)
        ql.set_option('optimize', 'no')
        ql.set_option('scheduler', 'ALAP')
        ql.set_option('log_level', 'LOG_WARNING')
        ql.set_option('use_default_gates', 'no')

    def test_program_name(self):
        name = "program1"
        nqubits=1
        p = ql.Program(name, platf, nqubits)
        self.assertEqual(p.name, name)

    def test_program_qubit_count(self):
        name = "program1"
        nqubits=3
        p = ql.Program(name, platf, nqubits)
        self.assertEqual(p.qubit_count, nqubits)

    def test_program_creg_count(self):
        name = "program1"
        nqubits=2
        ncreg = 3
        p = ql.Program(name, platf, nqubits, ncreg)
        self.assertEqual(p.creg_count, ncreg)

    def test_add_kernel(self):
        # test that this does not raise any error
        nqubits=5
        k = ql.Kernel("kernel1", platf, nqubits)
        p = ql.Program('program1', platf, nqubits)
        p.add_kernel(k)

        # there should be a check here to see if k was indeed added
        # p.kernel_list ==??

    def test_program_methods(self):
        # This tests for the existence of the right methods in the wrapping
        nqubits=5
        p = ql.Program('program1', platf, nqubits)
        program_methods = [
            'add_kernel',
            'add_program',
            'add_if',
            'add_if_else',
            'add_do_while',
            'add_for',
            'print_interaction_matrix',
            'write_interaction_matrix',
            'compile']
        self.assertTrue(set(program_methods).issubset(dir(p)))

    def test_simple_program(self):
        nqubits = 2
        k = ql.Kernel("kernel1", platf, nqubits)
        k.prepz(0)
        k.prepz(1)
        k.x(0)
        k.gate('cnot', [0, 1])
        k.gate("rx90", [1])
        k.clifford(1, 0)
        k.measure(0)

        p = ql.Program("rb_program", platf, nqubits)
        p.add_kernel(k)
        # print( p.qasm() )
        p.compile()


    def test_5qubit_program(self):
        nqubits=5
        p = ql.Program("a_program", platf, nqubits)
        k = ql.Kernel("a_kernel", platf, nqubits)

        # populate kernel
        for i in range(2):
            k.gate('prepz', [i])
        for i in range(2):
            k.gate('h', [i])

        k.gate('cnot', [0, 1])
        k.gate('cz', [0, 1])

        p.add_kernel(k)  # add kernel to program
        p.compile()


    # @unittest.skip('Gate by name not implemented')
    def test_allxy_program(self):
        nqubits=7
        p = ql.Program('AllXY', platf, nqubits)
        k = ql.Kernel('AllXY_q0', platf, nqubits)

        q = 0  # target qubit

        pulse_combinations = [
            ['I', 'I'], ['rx180', 'rx180'], ['ry180', 'ry180'],
            ['rx180', 'ry180'], ['ry180', 'rx180'],
            ['rx90', 'I'], ['ry90', 'I'], ['rx90', 'ry90'],
            ['ry90', 'rx90'], ['rx90', 'ry180'], ['ry90', 'rx180'],
            ['rx180', 'ry90'], ['ry180', 'rx90'], ['rx90', 'rx180'],
            ['rx180', 'rx90'], ['ry90', 'ry180'], ['ry180', 'ry90'],
            ['rx180', 'I'], ['ry180', 'I'], ['rx90', 'rx90'],
            ['ry90', 'ry90']]


        for pulse_comb in pulse_combinations:
            k.prepz(q)
            # Currently not possible to specify a gate using a string
            k.gate(pulse_comb[0], [q])
            k.gate(pulse_comb[1], [q])
            k.measure(q)

        p.add_kernel(k)

        p.compile()


if __name__ == '__main__':
    unittest.main()
