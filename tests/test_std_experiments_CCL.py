import os
import numpy as np
import unittest
from openql import Kernel, Program
from openql import openql as ql

curdir = os.path.dirname(os.path.realpath(__file__))
config_fn = os.path.join(curdir, 'hardware_config_cc_light.json') 
platf = ql.Platform('seven_qubits_chip', config_fn)

output_dir = os.path.join(curdir, 'test_output')

ql.set_option('output_dir', output_dir)
ql.set_option('optimize', 'no')
ql.set_option('scheduler', 'ALAP')
ql.set_option('log_level', 'LOG_WARNING')

@unittest.skip
class Test_single_qubit_seqs_CCL(unittest.TestCase):

    @classmethod
    def setUp(self):
        ql.initialize()

    def test_bug(self):
        p = Program("bug", platf, 1)

        k = Kernel("bugKernel", platform=platf, qubit_count=1)
        k.gate('rx180', [0])
        k.gate('measure', [0])

        p.add_kernel(k)
        p.compile()

        # Test that the generated code is valid
        QISA_fn = os.path.join(output_dir, p.name+'.qisa')
        assemble(QISA_fn)

    def test_allxy(self):
        p = Program("AllXY", platf, 1)
        # uppercase lowercase problems

        allXY = [['i', 'i'], ['rx180', 'rx180'], ['ry180', 'ry180'],
                 ['rx180', 'ry180'], ['ry180', 'rx180'],
                 ['rx90', 'i'], ['ry90', 'i'], ['rx90', 'ry90'],
                 ['ry90', 'rx90'], ['rx90', 'ry180'], ['ry90', 'rx180'],
                 ['rx180', 'ry90'], ['ry180', 'rx90'], ['rx90', 'rx180'],
                 ['rx180', 'rx90'], ['ry90', 'ry180'], ['ry180', 'ry90'],
                 ['rx180', 'i'], ['ry180', 'i'], ['rx90', 'rx90'],
                 ['ry90', 'ry90']]
         # this should be implicit
        p.set_sweep_points(np.arange(len(allXY), dtype=float))

        for i, xy in enumerate(allXY):
            k = Kernel("allXY"+str(i), platf, 1)
            k.prepz(0)
            k.gate(xy[0], [0])
            k.gate(xy[1], [0])
            k.measure(0)
            p.add_kernel(k)

        p.compile()

        # Test that the generated code is valid
        GOLD_fn = os.path.join(curdir, 'golden', p.name + '.qisa')
        QISA_fn = os.path.join(output_dir, p.name+'.qisa')
        self.assertTrue(file_compare(QISA_fn, GOLD_fn))

    def test_qasm_seq_echo(self):
        nqubits = 1
        p = Program("Echo", platf, nqubits)
        times = np.linspace(0, 20e3, 61)  # in ns
        # To prevent superslow workaround
        times = np.linspace(0, 60, 61)  # in ns
        # this should be implicit
        p.set_sweep_points(times)
        for tau in times:
            # this is an invalid kernel name (contains '.')
            # and will produce and invalid qasm
            n = 'echo_tau_{}ns'.format(tau)
            n = n.replace(".","_")
            k = Kernel(n, platf, nqubits)
            k.prepz(0)
            k.rx90(0)
            # This is a dirty hack that repeats the I gate
            for j in range(int(tau/2)):
                k.gate('i', [0])
            k.rx180(0)
            for j in range(int(tau/2)):
                k.gate('i', [0])
            k.rx90(0)
            k.measure(0)
            p.add_kernel(k)

        p.compile()

        # Test that the generated code is valid
        GOLD_fn = os.path.join(curdir, 'golden', p.name + '.qisa')
        QISA_fn = os.path.join(output_dir, p.name+'.qisa')
        self.assertTrue(file_compare(QISA_fn, GOLD_fn))

    def test_qasm_seq_butterfly(self):
        nqubits = 1
        p = Program("Butterfly", platf, nqubits)

        k = Kernel('0', platf, nqubits)
        k.prepz(0)
        k.measure(0)
        k.measure(0)
        # what does the measurement tell us it is?
        k.measure(0)
        # what is the post measuremnet state
        p.add_kernel(k)

        k = Kernel('1', platf, nqubits)
        k.prepz(0)
        k.measure(0)
        k.x(0)
        k.measure(0)
        k.measure(0)
        p.add_kernel(k)
        p.compile()

        # Test that the generated code is valid
        GOLD_fn = os.path.join(curdir, 'golden', p.name + '.qisa')
        QISA_fn = os.path.join(output_dir, p.name+'.qisa')
        self.assertTrue(file_compare(QISA_fn, GOLD_fn))

if __name__ == '__main__':
    unittest.main()

