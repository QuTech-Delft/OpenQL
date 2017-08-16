import os
import unittest
from openql import openql as ql

curdir = os.path.dirname(__file__)
config_fn = os.path.join(curdir, 'test_cfg_cbox.json')
platf = ql.Platform("starmon", config_fn)

class Test_program(unittest.TestCase):

    @classmethod
    def setUpClass(self):
        pass

    # @unittest.skip('NotImplemented')
    def test_program_name(self):
        name = "program1"
        nqubits=1
        p = ql.Program(name, nqubits, platf)
        # Program does not have a name attribute at this moment
        self.assertEqual(p.name, name)

    def test_add_kernel(self):
        # test that this does not raise any error
        k = ql.Kernel("kernel1", platf)
        nqubits=5
        p = ql.Program('program1', nqubits, platf)
        p.add_kernel(k)

        # there should be a check here to see if k was indeed added
        # p.kernel_list ==??

    def test_program_methods(self):
        # This tests for the existence of the right methods in the wrapping
        nqubits=5
        p = ql.Program('program1', nqubits, platf)
        program_methods = [
            'add_kernel',
            'compile',
            'microcode',
            'qasm',
            'schedule',
            'set_sweep_points']
        self.assertTrue(set(program_methods).issubset(dir(p)))

    def test_simple_program(self):
        ql.set_instruction_map_file("instructions.map")
        ql.init()
        k = ql.Kernel("kernel1", platf)
        k.prepz(0)
        k.prepz(1)
        k.x(0)
        k.y(0)
        k.cnot(0, 1)
        k.toffoli(0, 1, 2)
        k.rx90(0)
        k.clifford(2, 0)
        k.measure(2)
        sweep_points = [2]
        num_circuits = 1
        nqubits = 3
        p = ql.Program("rbProgram", nqubits, platf)
        p.set_sweep_points(sweep_points, num_circuits)
        p.add_kernel(k)
        p.compile()
        p.schedule()

        # N.B. This does not actually test if the output is correct
        print(p.qasm())
        print(p.microcode())

    def test_5qubit_program(self):
        ql.set_instruction_map_file("instructions.map")
        ql.init()
        nqubits=5
        p = ql.Program("aProgram", nqubits, platf)
        k = ql.Kernel("aKernel", platf)

        # populate kernel
        for i in range(5):
            k.prepz(i)
        for i in range(3):
            k.hadamard(i)
        for i in range(3):
            for j in range(3, 5):
                k.cnot(i, j)

        # sweep points is not specified the program does not work but don't
        # know what it does...
        p.set_sweep_points([10], 10)
        p.add_kernel(k)  # add kernel to program
        p.compile()     # compile program

        # N.B. This does not actually test if the output is correct
        print(p.qasm())
        print(p.microcode())

    @unittest.skip('Gate by name not implemented')
    def test_allxy_program(self):
        ql.set_instruction_map_file("instructions.map")
        ql.init()
        nqubits=7
        p = ql.Program('AllXY', nqubits,platf)
        k = ql.Kernel('AllXY_q0', platf)

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

        nr_sweep_pts = len(pulse_combinations)

        for pulse_comb in pulse_combinations:
            k.prepz(q)
            # Currently not possible to specify a gate using a string
            k.gate(pulse_comb[0], q)
            k.gate(pulse_comb[1], q)
            k.measure(q)

        p.set_sweep_points(sweep_points=nr_sweep_pts,
                           nr_circuits=nr_sweep_pts)

        p.compile()
        print(p.qasm)
        print(p.microcode)

if __name__ == '__main__':
    unittest.main()
