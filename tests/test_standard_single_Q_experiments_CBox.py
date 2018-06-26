import os
import numpy as np
import unittest
from openql import Kernel, Program
from openql import openql as ql
from CBox_Assembler import Assembler  # required to be in same folder

rootDir = os.path.dirname(os.path.realpath(__file__))

curdir = os.path.dirname(__file__)
config_fn = os.path.join(curdir, 'test_cfg_cbox.json')
platf = ql.Platform("starmon", config_fn)
output_dir = os.path.join(curdir, 'test_output')
ql.set_option('output_dir', output_dir)


class Test_single_qubit_seqs_CBox(unittest.TestCase):
    def test_allxy(self):
        nqubits = 1
        p = Program("AllXY", platf, nqubits)
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
        p.set_sweep_points(np.arange(len(allXY), dtype=float), len(allXY))

        for i, xy in enumerate(allXY):
            k = Kernel("allXY"+str(i), platform=platf, qubit_count=nqubits)
            k.prepz(0)
            k.gate(xy[0], [0])
            k.gate(xy[1], [0])
            k.measure(0)
            p.add_kernel(k)

        p.compile()

        # Test that the generated code is valid
        qumis_fn = os.path.join(output_dir, p.name_+'.asm')
        Assembler(qumis_fn).convert_to_instructions()

    def test_qasm_seq_echo(self):
        nqubits=1
        p = Program("Echo", platf, nqubits)
        times = np.linspace(0, 20e3, 61)  # in ns
        # To prevent superslow workaround
        times = np.linspace(0, 60, 61)  # in ns
        # this should be implicit
        p.set_sweep_points(times, len(times))
        for tau in times:
            # this is an invalid kernel name (contains '.')
            # and will produce and invalid qasm
            n = 'echo_tau_{}ns'.format(tau) 
            n = n.replace(".","_")
            k = Kernel(n, platform=platf, qubit_count=nqubits)
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
        qumis_fn = os.path.join(output_dir, p.name_+'.asm')
        Assembler(qumis_fn).convert_to_instructions()

    def test_qasm_seq_butterfly(self):
        nqubits=1
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
        qumis_fn = os.path.join(output_dir, p.name_+'.asm')
        Assembler(qumis_fn).convert_to_instructions()
