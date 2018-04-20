import os
import numpy as np
import unittest
from openql import Kernel, Program
from openql import openql as ql
from test_QISA_assembler_present import assemble

rootDir = os.path.dirname(os.path.realpath(__file__))

curdir = os.path.dirname(__file__)
# config_fn = os.path.join(curdir, 'test_cfg_cbox.json')
# platf = ql.Platform("starmon", config_fn)
# hardware_config_cc_light.json  OR  test_cfg_CCL.json
config_fn = os.path.join(curdir, 'hardware_config_cc_light.json') 
platf = ql.Platform('seven_qubits_chip', config_fn)

output_dir = os.path.join(curdir, 'test_output')

ql.set_option('output_dir', output_dir)
ql.set_option('optimize', 'no')
ql.set_option('scheduler', 'ALAP')
ql.set_option('log_level', 'LOG_WARNING')


class Test_single_qubit_seqs_CCL(unittest.TestCase):

    def test_bug(self):
        p = Program(pname="bug", nqubits=1, p=platf)

        k = Kernel("bugKernel", p=platf)
        k.gate('rx180', 0)
        k.gate('measure', 0)

        p.add_kernel(k)
        p.compile()

        # Test that the generated code is valid
        QISA_fn = os.path.join(output_dir, p.name+'.qisa')
        assemble(QISA_fn)

    def test_allxy(self):
        p = Program(pname="AllXY", nqubits=1, p=platf)
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
            k = Kernel("allXY"+str(i), p=platf)
            k.prepz(0)
            k.gate(xy[0], 0)
            k.gate(xy[1], 0)
            k.measure(0)
            p.add_kernel(k)

        p.compile()

        # Test that the generated code is valid
        QISA_fn = os.path.join(output_dir, p.name+'.qisa')
        assemble(QISA_fn)

    def test_qasm_seq_echo(self):
        p = Program(pname="Echo", nqubits=1, p=platf)
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
            k = Kernel(n, p=platf)
            k.prepz(0)
            k.rx90(0)
            # This is a dirty hack that repeats the I gate
            for j in range(int(tau/2)):
                k.gate('i', 0)
            k.rx180(0)
            for j in range(int(tau/2)):
                k.gate('i', 0)
            k.rx90(0)
            k.measure(0)
            p.add_kernel(k)
            
        p.compile()

        # Test that the generated code is valid
        QISA_fn = os.path.join(output_dir, p.name+'.qisa')
        assemble(QISA_fn)

    def test_qasm_seq_butterfly(self):
        p = Program(pname="Butterfly", nqubits=1, p=platf)

        k = Kernel('0', p=platf)
        k.prepz(0)
        k.measure(0)
        k.measure(0)
        # what does the measurement tell us it is?
        k.measure(0)
        # what is the post measuremnet state
        p.add_kernel(k)

        k = Kernel('1', p=platf)
        k.prepz(0)
        k.measure(0)
        k.x(0)
        k.measure(0)
        k.measure(0)
        p.add_kernel(k)
        p.compile()

        # Test that the generated code is valid
        QISA_fn = os.path.join(output_dir, p.name+'.qisa')
        assemble(QISA_fn)

if __name__ == '__main__':
    unittest.main()
        
