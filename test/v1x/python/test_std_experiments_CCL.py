import numpy as np
import openql as ql
import os
import unittest

from config import output_dir, qasm_golden_dir
from utils import file_compare


platform = ql.Platform('seven_qubits_chip', 'cc_light')

ql.set_option('output_dir', output_dir)
ql.set_option('optimize', 'no')
ql.set_option('scheduler', 'ALAP')
ql.set_option('log_level', 'LOG_WARNING')


@unittest.skip
class TestSingleQubitSeqsCcl(unittest.TestCase):
    @classmethod
    def setUp(cls):
        ql.initialize()

    def test_bug(self):
        p = ql.Program("bug", platform, 1)

        k = ql.Kernel("bugKernel", platform=platform, qubit_count=1)
        k.gate('rx180', [0])
        k.gate('measure', [0])

        p.add_kernel(k)
        p.compile()

        # Test that the generated code is valid
        qisa_fn = os.path.join(output_dir, p.name + '.qisa')
        assemble(qisa_fn)

    def test_all_xy(self):
        p = ql.Program("AllXY", platform, 1)
        # uppercase lowercase problems

        all_xy = [
            ['i', 'i'], ['rx180', 'rx180'], ['ry180', 'ry180'],
            ['rx180', 'ry180'], ['ry180', 'rx180'],
            ['rx90', 'i'], ['ry90', 'i'], ['rx90', 'ry90'],
            ['ry90', 'rx90'], ['rx90', 'ry180'], ['ry90', 'rx180'],
            ['rx180', 'ry90'], ['ry180', 'rx90'], ['rx90', 'rx180'],
            ['rx180', 'rx90'], ['ry90', 'ry180'], ['ry180', 'ry90'],
            ['rx180', 'i'], ['ry180', 'i'], ['rx90', 'rx90'],
            ['ry90', 'ry90']
        ]

        for i, xy in enumerate(all_xy):
            k = ql.Kernel("allXY"+str(i), platform, 1)
            k.prepz(0)
            k.gate(xy[0], [0])
            k.gate(xy[1], [0])
            k.measure(0)
            p.add_kernel(k)

        p.compile()

        # Test that the generated code is valid
        gold_fn = os.path.join(qasm_golden_dir, p.name + '.qisa')
        qisa_fn = os.path.join(output_dir, p.name + '.qisa')
        self.assertTrue(file_compare(qisa_fn, gold_fn))

    def test_qasm_seq_echo(self):
        num_qubits = 1
        p = ql.Program("Echo", platform, num_qubits)
        # times = np.linspace(0, 20e3, 61)  # in ns
        # To prevent superslow workaround
        times = np.linspace(0, 60, 61)  # in ns
        # this should be implicit
        for tau in times:
            # this is an invalid kernel name (contains '.')
            # and will produce and invalid qasm
            n = 'echo_tau_{}ns'.format(tau)
            n = n.replace(".", "_")
            k = ql.Kernel(n, platform, num_qubits)
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
        gold_fn = os.path.join(qasm_golden_dir, p.name + '.qisa')
        qisa_fn = os.path.join(output_dir, p.name + '.qisa')
        self.assertTrue(file_compare(qisa_fn, gold_fn))

    def test_qasm_seq_butterfly(self):
        num_qubits = 1
        p = ql.Program("Butterfly", platform, num_qubits)

        k = ql.Kernel('0', platform, num_qubits)
        k.prepz(0)
        k.measure(0)
        k.measure(0)
        # what does the measurement tell us it is?
        k.measure(0)
        # what is the post measuremnet state
        p.add_kernel(k)

        k = ql.Kernel('1', platform, num_qubits)
        k.prepz(0)
        k.measure(0)
        k.x(0)
        k.measure(0)
        k.measure(0)
        p.add_kernel(k)
        p.compile()

        # Test that the generated code is valid
        gold_fn = os.path.join(qasm_golden_dir, p.name + '.qisa')
        qisa_fn = os.path.join(output_dir, p.name + '.qisa')
        self.assertTrue(file_compare(qisa_fn, gold_fn))

if __name__ == '__main__':
    unittest.main()

