import os
import unittest
from openql import openql as ql
import numpy as np

curdir = os.path.dirname(os.path.realpath(__file__))
output_dir = os.path.join(curdir, 'test_output')

class Test_uniform_scheduler(unittest.TestCase):

    @classmethod
    def setUp(self):
        ql.initialize()

    def test_uniform_scheduler_0(self):
        ql.set_option('output_dir', output_dir)
        ql.set_option('optimize', 'no')
        ql.set_option('scheduler', 'ALAP')
        ql.set_option('scheduler_uniform', 'yes')
        ql.set_option('log_level', 'LOG_WARNING')

        config_fn = os.path.join(curdir, 'test_cfg_none_s7.json')
        platform  = ql.Platform('starmon', config_fn)

        num_qubits = 7
        p = ql.Program('test_uniform_scheduler_0', platform, num_qubits, 0)
        k = ql.Kernel('kernel_0', platform, num_qubits, 0)

	# a simple first test
	# the x gates serve to separate the cnot gates wrt dependences
	# this creates big bundles with 7 x gates
	# and small bundles with just a cnot
	# after uniform scheduling, one or more x gates
	# should have been moved next to the cnot
	# those will move that do not have operands that overlap those of the cnot

        for j in range(7):
            k.gate("x", [j])

        k.gate("cnot", [0,2])

        for j in range(7):
            k.gate("x", [j])

        k.gate("cnot", [6,3])

        for j in range(7):
            k.gate("x", [j])

        k.gate("cnot", [1,4])

        p.add_kernel(k)
        p.compile()

    def test_uniform_scheduler_1(self):
        ql.set_option('output_dir', output_dir)
        ql.set_option('optimize', 'no')
        ql.set_option('scheduler', 'ALAP')
        ql.set_option('scheduler_uniform', 'yes')
        ql.set_option('log_level', 'LOG_WARNING')

        config_fn = os.path.join(curdir, 'test_cfg_none_s7.json')
        platform  = ql.Platform('starmon', config_fn)

        num_qubits = 7
        p = ql.Program('test_uniform_scheduler_1', platform, num_qubits, 0)
        k = ql.Kernel('kernel_1', platform, num_qubits, 0)

	# just as the previous one
	# but then more of the same

        for j in range(7):
            k.gate("x", [j])
        k.gate("cnot", [0,2])

        for j in range(7):
            k.gate("x", [j])
        k.gate("cnot", [6,3])

        for j in range(7):
            k.gate("x", [j])
        k.gate("cnot", [1,4])

        for j in range(7):
            k.gate("x", [j])
        k.gate("cnot", [2,5])

        for j in range(7):
            k.gate("x", [j])
        k.gate("cnot", [3,1])

        for j in range(7):
            k.gate("x", [j])
        k.gate("cnot", [4,6])

        for j in range(7):
            k.gate("x", [j])
        k.gate("cnot", [2,0])

        for j in range(7):
            k.gate("x", [j])
        k.gate("cnot", [3,6])

        for j in range(7):
            k.gate("x", [j])
        k.gate("cnot", [4,1])

        for j in range(7):
            k.gate("x", [j])
        k.gate("cnot", [5,2])

        for j in range(7):
            k.gate("x", [j])
        k.gate("cnot", [1,3])

        for j in range(7):
            k.gate("x", [j])
        k.gate("cnot", [6,4])

        for j in range(7):
            k.gate("x", [j])

        p.add_kernel(k)
        p.compile()

    def test_uniform_scheduler_2(self):
        ql.set_option('output_dir', output_dir)
        ql.set_option('optimize', 'no')
        ql.set_option('scheduler', 'ALAP')
        ql.set_option('scheduler_uniform', 'yes')
        ql.set_option('log_level', 'LOG_WARNING')

        config_fn = os.path.join(curdir, 'test_cfg_none_s7.json')
        platform  = ql.Platform('starmon', config_fn)

        num_qubits = 7
        p = ql.Program('test_uniform_scheduler_2', platform, num_qubits, 0)
        k = ql.Kernel('kernel_2', platform, num_qubits, 0)

	# again big bundles with x gates
	# alternated with non-trivial cnot bundles;
	# these cnots were chosen to be mutually independent
	# so will be going all 3 in one bundle
	# the single independent x will be moved with it
	# but because the cnots take 4 cycles,
	# also a lot of empty cycles are created
	# which cannot be filled except by that
	# stray independent x

        for j in range(7):
            k.gate("x", [j])
        k.gate("cnot", [0,2])
        k.gate("cnot", [6,3])
        k.gate("cnot", [1,4])

        for j in range(7):
            k.gate("x", [j])
        k.gate("cnot", [2,5])
        k.gate("cnot", [3,1])
        k.gate("cnot", [4,6])

        for j in range(7):
            k.gate("x", [j])
        k.gate("cnot", [2,0])
        k.gate("cnot", [3,6])
        k.gate("cnot", [4,1])

        for j in range(7):
            k.gate("x", [j])
        k.gate("cnot", [5,2])
        k.gate("cnot", [1,3])
        k.gate("cnot", [6,4])

        for j in range(7):
            k.gate("x", [j])

        p.add_kernel(k)
        p.compile()

    def test_uniform_scheduler_3(self):
        ql.set_option('output_dir', output_dir)
        ql.set_option('optimize', 'no')
        ql.set_option('scheduler', 'ALAP')
        ql.set_option('scheduler_uniform', 'yes')
        ql.set_option('log_level', 'LOG_WARNING')

        config_fn = os.path.join(curdir, 'test_cfg_none_s7.json')
        platform  = ql.Platform('starmon', config_fn)

        num_qubits = 7
        p = ql.Program('test_uniform_scheduler_3', platform, num_qubits, 0)
        k = ql.Kernel('kernel_3', platform, num_qubits, 0)

	# as with test 2 but now the cnots are not mutually independent anymore
	# this already creates smaller bundles but more of them

        for j in range(7):
            k.gate("x", [j])
        k.gate("cnot", [6,3])
        k.gate("cnot", [0,2])
        k.gate("cnot", [1,3])
        k.gate("cnot", [1,4])
        k.gate("cnot", [0,3])

        for j in range(7):
            k.gate("x", [j])
        k.gate("cnot", [2,5])
        k.gate("cnot", [3,1])
        k.gate("cnot", [2,0])
        k.gate("cnot", [3,6])

        for j in range(7):
            k.gate("x", [j])
        k.gate("cnot", [4,1])
        k.gate("cnot", [3,0])
        k.gate("cnot", [4,6])

        for j in range(7):
            k.gate("x", [j])
        k.gate("cnot", [3,5])
        k.gate("cnot", [5,2])
        k.gate("cnot", [6,4])
        k.gate("cnot", [5,3])

        for j in range(7):
            k.gate("x", [j])

        p.add_kernel(k)
        p.compile()

    def test_uniform_scheduler_4(self):
        ql.set_option('output_dir', output_dir)
        ql.set_option('optimize', 'no')
        ql.set_option('scheduler', 'ALAP')
        ql.set_option('scheduler_uniform', 'yes')
        ql.set_option('log_level', 'LOG_WARNING')

        config_fn = os.path.join(curdir, 'test_cfg_none_s7.json')
        platform  = ql.Platform('starmon', config_fn)

        num_qubits = 7
        p = ql.Program('test_uniform_scheduler_4', platform, num_qubits, 0)
        k = ql.Kernel('kernel_4', platform, num_qubits, 0)

	# as with test 3 but now without the big x bundles
	# just the cnots in lexicographic order
	# the worst you can imagine,
	# creating the smallest bundles

        k.gate("cnot", [0,2])
        k.gate("cnot", [0,3])
        k.gate("cnot", [1,3])
        k.gate("cnot", [1,4])
        k.gate("cnot", [2,0])
        k.gate("cnot", [2,5])
        k.gate("cnot", [3,0])
        k.gate("cnot", [3,1])
        k.gate("cnot", [3,5])
        k.gate("cnot", [3,6])
        k.gate("cnot", [4,1])
        k.gate("cnot", [4,6])
        k.gate("cnot", [5,2])
        k.gate("cnot", [5,3])
        k.gate("cnot", [6,3])
        k.gate("cnot", [6,4])

        p.add_kernel(k)
        p.compile()

if __name__ == '__main__':
    unittest.main()
