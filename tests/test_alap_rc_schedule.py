from openql import openql as ql
import os
from test_QISA_assembler_present import assemble, assembler_present
import unittest
from utils import file_compare


rootDir = os.path.dirname(os.path.realpath(__file__))
curdir = os.path.dirname(__file__)
output_dir = os.path.join(curdir, 'test_output')


class Test_Alap_Rc_Schedule(unittest.TestCase):
    _SCHEDULER = 'ALAP'
    config = os.path.join(rootDir, "hardware_config_cc_light.json")

    def setUp(self):
        ql.set_option('scheduler', self._SCHEDULER)
        ql.set_option('output_dir', output_dir)
        ql.set_option('log_level', "LOG_NOTHING")

    @unittest.skipUnless(assembler_present, "libqasm not found")
    def test_qwg(self):
        self.setUp()
        # parameters
        v = 'qwg'

        # create and set platform
        prog_name = "test_" + v + "_" + self._SCHEDULER
        kernel_name = "kernel_" + v + "_" + self._SCHEDULER

        starmon = ql.Platform("starmon", self.config)
        prog = ql.Program(prog_name, starmon, 7, 0)
        k = ql.Kernel(kernel_name, starmon, 7, 0)

        # no dependency, only a conflict in qwg resource
        k.gate("x", [0])
        k.gate("y", [1])

        prog.add_kernel(k)
        prog.compile()

        GOLD_fn = os.path.join(rootDir, 'golden', prog.name + '.qisa')
        QISA_fn = os.path.join(output_dir, prog.name+'.qisa')

        assemble(QISA_fn)
        self.assertTrue(file_compare(QISA_fn, GOLD_fn))

    @unittest.skipUnless(assembler_present, "libqasm not found")
    def test_qwg2(self):
        self.setUp()
        # parameters
        v = 'qwg2'
        scheduler = self._SCHEDULER

        # create and set platform
        prog_name = "test_" + v + "_" + scheduler
        kernel_name = "kernel_" + v + "_" + scheduler

        starmon = ql.Platform("starmon", self.config)
        prog = ql.Program(prog_name, starmon, 7, 0)
        k = ql.Kernel(kernel_name, starmon, 7, 0)

        for j in range(7):
            k.gate("x", [j])
        k.gate("x", [0])
        k.gate("y", [1])
        k.gate("y", [2])
        k.gate("x", [3])
        k.gate("y", [4])
        k.gate("x", [5])
        k.gate("y", [6])
        for j in range(7):
            k.gate("y", [j])

        prog.add_kernel(k)
        ql.set_option("scheduler", scheduler)
        prog.compile()

        GOLD_fn = os.path.join(rootDir, 'golden', prog.name + '.qisa')
        QISA_fn = os.path.join(output_dir, prog.name+'.qisa')

        assemble(QISA_fn)
        self.assertTrue(file_compare(QISA_fn, GOLD_fn))

    @unittest.skipUnless(assembler_present, "libqasm not found")
    def test_issue179(self):
        self.setUp()
        # parameters
        v = 'issue179'
        scheduler = self._SCHEDULER

        # create and set platform
        prog_name = "test_" + v + '_' + scheduler
        kernel_name = "kernel_" + v + '_' + scheduler

        starmon = ql.Platform("starmon", self.config)
        prog = ql.Program(prog_name, starmon, 7, 0)
        k = ql.Kernel(kernel_name, starmon, 7, 0)

        # independent gates but stacking qwg unit use
        # in s7, q2, q3 and q4 all use qwg1
        # the y q3 must be in an other cycle than the x's because x conflicts with y in qwg1
        # the x q2 and x q4 can be in parallel but the y q3 in between prohibits this
        # because the qwg1 resource in single dimensional:
        # after x q2 it is busy on x in cycle 0,
        # then it only looks at the y q3, which requires to go to cycle 1,
        # and then the x q4 only looks at the current cycle (cycle 1),
        # in which qwg1 is busy with the y, so for the x it is busy,
        # and the only option is to go for cycle 2
        k.gate("x", [2])
        k.gate("y", [3])
        k.gate("x", [4])

        prog.add_kernel(k)
        ql.set_option("scheduler", scheduler)
        prog.compile()

        GOLD_fn = os.path.join(rootDir, 'golden', prog.name + '.qisa')
        QISA_fn = os.path.join(output_dir, prog.name+'.qisa')

        assemble(QISA_fn)
        self.assertTrue(file_compare(QISA_fn, GOLD_fn))

    @unittest.skipUnless(assembler_present, "libqasm not found")
    def test_edge(self):
        self.setUp()
        # parameters
        v = 'edge'
        scheduler = self._SCHEDULER

        # create and set platform
        prog_name = "test_" + v + '_' + scheduler
        kernel_name = "kernel_" + v + '_' + scheduler

        starmon = ql.Platform("starmon", self.config)
        prog = ql.Program(prog_name, starmon, 7, 0)
        k = ql.Kernel(kernel_name, starmon, 7, 0)

        # no dependency, only a conflict in edge resource between the first two czs
        k.gate("cz", [1, 4])
        k.gate("cz", [0, 3])
        k.gate("cz", [2, 5])

        prog.add_kernel(k)
        ql.set_option("scheduler", scheduler)
        prog.compile()

        GOLD_fn = os.path.join(rootDir, 'golden', prog.name + '.qisa')
        QISA_fn = os.path.join(output_dir, prog.name+'.qisa')

        assemble(QISA_fn)
        self.assertTrue(file_compare(QISA_fn, GOLD_fn))

    @unittest.skipUnless(assembler_present, "libqasm not found")
    def test_detuned(self):
        self.setUp()
        # parameters
        v = 'detuned'
        scheduler = self._SCHEDULER

        # create and set platform
        prog_name = "test_" + v + '_' + scheduler
        kernel_name = "kernel_" + v + '_' + scheduler

        starmon = ql.Platform("starmon", self.config)
        prog = ql.Program(prog_name, starmon, 7, 0)
        k = ql.Kernel(kernel_name, starmon, 7, 0)

        # preferably cz's parallel, but not with x 3
        k.gate("cz", [0, 2])
        k.gate("cz", [1, 4])
        k.gate("x", [3])

        # likewise, while y 3, no cz on 0,2 or 1,4
        k.gate("y", [3])
        k.gate("cz", [0, 2])
        k.gate("cz", [1, 4])

        prog.add_kernel(k)
        ql.set_option("scheduler", scheduler)
        prog.compile()

        GOLD_fn = os.path.join(rootDir, 'golden', prog.name + '.qisa')
        QISA_fn = os.path.join(output_dir, prog.name+'.qisa')

        assemble(QISA_fn)
        self.assertTrue(file_compare(QISA_fn, GOLD_fn))

    @unittest.skipUnless(assembler_present, "libqasm not found")
    def test_detuned2(self):
        self.setUp()
        # parameters
        v = 'detuned2'
        scheduler = self._SCHEDULER

        # create and set platform
        prog_name = "test_" + v + '_' + scheduler
        kernel_name = "kernel_" + v + '_' + scheduler

        starmon = ql.Platform("starmon", self.config)
        prog = ql.Program(prog_name, starmon, 7, 0)
        k = ql.Kernel(kernel_name, starmon, 7, 0)

        # preferably cz's parallel, but not with x 3
        k.gate("cz", [0, 2])
        k.gate("cz", [1, 4])
        k.gate("x", [3])

        # likewise, while y 3, no cz on 0,2 or 1,4
        k.gate("y", [3])
        k.gate("cz", [2, 5])
        k.gate("cz", [4, 6])

        prog.add_kernel(k)
        ql.set_option("scheduler", scheduler)
        prog.compile()

        GOLD_fn = os.path.join(rootDir, 'golden', prog.name + '.qisa')
        QISA_fn = os.path.join(output_dir, prog.name+'.qisa')

        assemble(QISA_fn)
        self.assertTrue(file_compare(QISA_fn, GOLD_fn))

    @unittest.skipUnless(assembler_present, "libqasm not found")
    def test_adriaan(self):
        self.setUp()
        # parameters
        v = 'adriaan'
        scheduler = self._SCHEDULER

        # create and set platform
        prog_name = "test_" + v + '_' + scheduler
        kernel_name = "kernel_" + v + '_' + scheduler

        starmon = ql.Platform("starmon", self.config)
        prog = ql.Program(prog_name, starmon, 7, 0)
        k = ql.Kernel(kernel_name, starmon, 7, 0)

        k.gate("prepz", [0])
        k.gate("prepz", [2])
        for _ in range(10):
            k.gate("x", [0])

        for _ in range(6):
            k.gate("rx90", [2])
        k.gate("measure", [2])
        k.gate("measure", [0])

        prog.add_kernel(k)
        ql.set_option("scheduler", scheduler)
        prog.compile()

        GOLD_fn = os.path.join(rootDir, 'golden', prog.name + '.qisa')
        QISA_fn = os.path.join(output_dir, prog.name+'.qisa')

        assemble(QISA_fn)
        self.assertTrue(file_compare(QISA_fn, GOLD_fn))

    @unittest.skipUnless(assembler_present, "libqasm not found")
    def test_1(self):
        self.setUp()
        # parameters
        v = '1'
        scheduler = self._SCHEDULER

        # create and set platform
        prog_name = "test_" + v + '_' + scheduler
        kernel_name = "kernel_" + v + '_' + scheduler

        starmon = ql.Platform("starmon", self.config)
        prog = ql.Program(prog_name, starmon, 7, 0)
        k = ql.Kernel(kernel_name, starmon, 7, 0)

        for j in range(7):
            k.gate("x", [j])

        # a list of all cnots that are ok in trivial mapping
        k.gate("cnot", [0, 2])
        k.gate("cnot", [0, 3])
        k.gate("cnot", [1, 3])
        k.gate("cnot", [1, 4])
        k.gate("cnot", [2, 0])
        k.gate("cnot", [2, 5])
        k.gate("cnot", [3, 0])
        k.gate("cnot", [3, 1])
        k.gate("cnot", [3, 5])
        k.gate("cnot", [3, 6])
        k.gate("cnot", [4, 1])
        k.gate("cnot", [4, 6])
        k.gate("cnot", [5, 2])
        k.gate("cnot", [5, 3])
        k.gate("cnot", [6, 3])
        k.gate("cnot", [6, 4])

        prog.add_kernel(k)

        ql.set_option("scheduler", scheduler)
        prog.compile()

        GOLD_fn = os.path.join(rootDir, 'golden', prog.name + '.qisa')
        QISA_fn = os.path.join(output_dir, prog.name+'.qisa')

        assemble(QISA_fn)
        self.assertTrue(file_compare(QISA_fn, GOLD_fn))

    @unittest.skipUnless(assembler_present, "libqasm not found")
    def test_7(self):
        self.setUp()
        # parameters
        v = '7'
        scheduler = self._SCHEDULER

        # create and set platform
        prog_name = "test_" + v + '_' + scheduler
        kernel_name = "kernel_" + v + '_' + scheduler

        starmon = ql.Platform("starmon", self.config)
        prog = ql.Program(prog_name, starmon, 7, 0)
        k = ql.Kernel(kernel_name, starmon, 7, 0)

        k.gate("prepz", [0])
        k.gate("prepz", [1])
        k.gate("prepz", [2])
        k.gate("prepz", [3])
        k.gate("prepz", [4])
        k.gate("prepz", [5])
        k.gate("prepz", [6])
        # preps all end at same time, is the base of ASAP

        # rotations on q0, q2 and q5, mutually independent, also wrt resource use
        k.gate("h", [0])  # qubit 0 in qwg0 10 cycles rotations
        k.gate("t", [0])
        k.gate("h", [0])
        k.gate("t", [0])

        k.gate("h", [2])  # qubit 2 in qwg1 5 cycles rotations
        k.gate("t", [2])

        k.gate("h", [5])  # qubit 4 in qwg2 2 cycles rotations

        # measures all start at same time, is the horizon for ALAP
        k.gate("measure", [0])
        k.gate("measure", [1])
        k.gate("measure", [2])
        k.gate("measure", [3])
        k.gate("measure", [4])
        k.gate("measure", [5])
        k.gate("measure", [6])

        prog.add_kernel(k)

        ql.set_option("scheduler", scheduler)
        prog.compile()

        GOLD_fn = os.path.join(rootDir, 'golden', prog.name + '.qisa')
        QISA_fn = os.path.join(output_dir, prog.name+'.qisa')

        assemble(QISA_fn)
        self.assertTrue(file_compare(QISA_fn, GOLD_fn))


if __name__ == '__main__':
    # ql.set_option('log_level', 'LOG_DEBUG')
    unittest.main()
