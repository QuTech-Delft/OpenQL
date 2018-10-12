import os
import unittest
from openql import openql as ql
from test_QISA_assembler_present import assemble

rootDir = os.path.dirname(os.path.realpath(__file__))

curdir = os.path.dirname(__file__)
output_dir = os.path.join(curdir, 'test_output')

def file_compare(fn1, fn2):
    isSame = False
    with open(fn1, 'r') as f1:
        with open(fn2, 'r') as f2:
            a = f1.read()
            b = f2.read()
            f1.close()
            f2.close()
            isSame = (a==b)
    return isSame


class Test_basic(unittest.TestCase):

    @classmethod
    def setUpClass(self):
        ql.set_option('output_dir', output_dir)
        ql.set_option('optimize', 'no')
        ql.set_option('scheduler', 'ASAP')
        ql.set_option('log_level', 'LOG_WARNING')

    # single qubit mask generation test
    # @unittest.expectedFailure
    # @unittest.skip
    def test_smis(self):

        # You can specify a config location, here we use a default config
        config_fn = os.path.join(curdir, 'hardware_config_cc_light.json')
        platform = ql.Platform('seven_qubits_chip', config_fn)
        sweep_points = [1, 2]
        num_qubits = platform.get_qubit_number()
        p = ql.Program('aProgram', platform, num_qubits)
        p.set_sweep_points(sweep_points, len(sweep_points))

        # populate kernel using default gates
        k = ql.Kernel('aKernel', platform, num_qubits)
        k.prepz(0)
        k.prepz(1)
        k.prepz(2)
        k.prepz(3)
        k.gate('h', [0])
        k.gate('h', [1])
        k.x(2)
        k.x(3)
        k.measure(0)
        k.measure(1)
        k.measure(2)
        k.measure(3)

        # add the kernel to the program
        p.add_kernel(k)

        # compile the program
        p.compile()

        # load qasm
        qasm_files = []
        qasm_files.append(os.path.join(output_dir, 'aProgram.qasm'))
        qasm_files.append(os.path.join(output_dir, 'aProgram_scheduled.qasm'))

        # TODO use new cqasm v1.0 interface
        # for qasm_file in qasm_files:
        #    qasm_reader = ql.QASM_Loader(qasm_file)
        #    errors = qasm_reader.load()
        #    self.assertTrue(errors == 0)

        QISA_fn = os.path.join(output_dir, p.name+'.qisa')
        assemble(QISA_fn)

    # single qubit mask generation test with custom gates
    # @unittest.skip
    def test_smis_with_custom_gates(self):

        # You can specify a config location, here we use a default config
        config_fn = os.path.join(curdir, 'hardware_config_cc_light.json')
        platform = ql.Platform('seven_qubits_chip', config_fn)
        sweep_points = [1, 2]
        num_qubits = 7
        p = ql.Program('aProgram', platform, num_qubits)
        p.set_sweep_points(sweep_points, len(sweep_points))

        # populate the second kernel using both custom and default gates
        k = ql.Kernel('aKernel', platform, num_qubits)
        k.gate('prepz', [0])
        k.gate('x', [0])
        k.y(1)
        k.z(5)
        k.gate('rx90', [0])
        k.gate('cnot', [0, 3])
        k.gate('measure', [0])

        # add the kernel to the program
        p.add_kernel(k)

        # compile the program
        p.compile()

        # load qasm
        qasm_files = []
        qasm_files.append(os.path.join(output_dir, 'aProgram.qasm'))
        qasm_files.append(os.path.join(output_dir, 'aProgram_scheduled.qasm'))

        # TODO use new cqasm v1.0 interface
        # for qasm_file in qasm_files:
        #    qasm_reader = ql.QASM_Loader(qasm_file)
        #    errors = qasm_reader.load()
        #    self.assertTrue(errors == 0)

        QISA_fn = os.path.join(output_dir, p.name+'.qisa')
        assemble(QISA_fn)

    # single qubit mask generation multi-kernel test (custom with non-custom
    # gates)
    # @unittest.skip
    def test_smis_multi_kernel(self):

        # You can specify a config location, here we use a default config
        config_fn = os.path.join(curdir, 'hardware_config_cc_light.json')
        platform = ql.Platform('seven_qubits_chip', config_fn)
        sweep_points = [1, 2]
        num_qubits = 7
        p = ql.Program('aProgram', platform, num_qubits)
        p.set_sweep_points(sweep_points, len(sweep_points))

        # populate the first kernel using default gates
        k = ql.Kernel('aKernel01', platform, num_qubits)
        k.prepz(0)
        k.x(2)
        k.y(3)
        k.z(5)
        k.rx90(0)
        # k.cphase(0,1)  # when uncommented this should raise 'operation not supported' exception.
        # k.cphase(0,3)
        k.cnot(0, 3)
        k.measure(0)

        # add the kernel to the program
        p.add_kernel(k)

        # populate the second kernel using both custom and default gates
        k = ql.Kernel('aKernel02', platform, num_qubits)
        k.gate('prepz', [0])  # this line is equivalent to the previous
        k.gate('x', [0])
        k.y(1)
        k.z(5)
        k.gate('rx90', [0])
        # k.gate('cz', [0, 3])
        k.gate('cnot', [0, 3])
        k.gate('measure', [0])

        # add the kernel to the program
        p.add_kernel(k)

        # compile the program
        p.compile()

        # load qasm
        qasm_files = []
        qasm_files.append(os.path.join(output_dir, 'aProgram.qasm'))
        qasm_files.append(os.path.join(output_dir, 'aProgram_scheduled.qasm'))

        # TODO use new cqasm v1.0 interface
        # for qasm_file in qasm_files:
        #    qasm_reader = ql.QASM_Loader(qasm_file)
        #    errors = qasm_reader.load()
        #    self.assertTrue(errors == 0)

        QISA_fn = os.path.join(output_dir, p.name+'.qisa')
        assemble(QISA_fn)

    def test_smis_all_bundled(self):

        # You can specify a config location, here we use a default config
        config_fn = os.path.join(curdir, 'hardware_config_cc_light.json')
        platform = ql.Platform('seven_qubits_chip', config_fn)
        sweep_points = [1, 2]
        num_qubits = platform.get_qubit_number()
        p = ql.Program('test_smis_all_bundled', platform, num_qubits)
        p.set_sweep_points(sweep_points, len(sweep_points))

        k = ql.Kernel('aKernel', platform, num_qubits)

        for i in range(7):
            k.prepz(i)

        for i in range(7):
            k.gate('x', [i])

        # add the kernel to the program
        p.add_kernel(k)

        # compile the program
        p.compile()

        QISA_fn = os.path.join(output_dir, p.name+'.qisa')
        GOLD_fn = rootDir + '/golden/test_smis_all_bundled.qisa'

        assemble(QISA_fn)

        self.assertTrue( file_compare(QISA_fn, GOLD_fn) )


    # two qubit mask generation test
    # @unittest.skip
    def test_smit(self):

        # You can specify a config location, here we use a default config
        config_fn = os.path.join(curdir, 'hardware_config_cc_light.json')
        platform = ql.Platform('seven_qubits_chip', config_fn)
        sweep_points = [1, 2]
        num_qubits = 7
        p = ql.Program('aProgram', platform, num_qubits)
        p.set_sweep_points(sweep_points, len(sweep_points))

        # populate kernel using default gates
        k = ql.Kernel('aKernel', platform, num_qubits)
        k.prepz(0)
        k.prepz(1)
        k.prepz(2)
        k.prepz(3)
        k.gate('h', [0])
        k.gate('h', [1])
        k.x(2)
        k.x(3)
        k.cnot(2, 0)
        k.cnot(2, 0)
        k.cnot(1, 4)
        k.measure(0)
        k.measure(1)
        k.measure(2)
        k.measure(3)

        # add the kernel to the program
        p.add_kernel(k)

        # compile the program
        p.compile()

        # load qasm
        qasm_files = []
        qasm_files.append(os.path.join(output_dir, 'aProgram.qasm'))
        qasm_files.append(os.path.join(output_dir, 'aProgram_scheduled.qasm'))

        # TODO use new cqasm v1.0 interface
        # for qasm_file in qasm_files:
        #    qasm_reader = ql.QASM_Loader(qasm_file)
        #    errors = qasm_reader.load()
        #    self.assertTrue(errors == 0)

        QISA_fn = os.path.join(output_dir, p.name+'.qisa')
        assemble(QISA_fn)

    def test_smit_all_bundled(self):

        # You can specify a config location, here we use a default config
        config_fn = os.path.join(curdir, 'hardware_config_cc_light.json')
        platform = ql.Platform('seven_qubits_chip', config_fn)
        sweep_points = [1, 2]
        num_qubits = platform.get_qubit_number()
        p = ql.Program('test_smit_all_bundled', platform, num_qubits)
        p.set_sweep_points(sweep_points, len(sweep_points))

        # populate kernel using default gates
        k = ql.Kernel('aKernel', platform, num_qubits)

        for i in range(7):
            k.prepz(i)

        k.gate('cz', [2, 0])
        k.gate('cz', [3, 5])
        k.gate('cz', [1, 4])
        k.gate('cz', [4, 6])
        k.gate('cz', [2, 5])
        k.gate('cz', [3, 0])

        # add the kernel to the program
        p.add_kernel(k)

        # compile the program
        p.compile()

        QISA_fn = os.path.join(output_dir, p.name+'.qisa')
        GOLD_fn = rootDir + '/golden/test_smit_all_bundled.qisa'

        assemble(QISA_fn)

        self.assertTrue( file_compare(QISA_fn, GOLD_fn) )

class Test_advance(unittest.TestCase):
	# @unittest.skip
    def test_qubit_busy(self):

        config_fn = os.path.join(curdir, 'hardware_config_cc_light.json')
        platform  = ql.Platform('seven_qubits_chip', config_fn)
        sweep_points = [1,2]
        num_qubits = 7
        p = ql.Program('aProgram', platform, num_qubits)
        p.set_sweep_points(sweep_points, len(sweep_points))

        # populate kernel using default gates
        k = ql.Kernel('aKernel', platform, num_qubits)

        k.prepz(0)
        k.prepz(0)

        # add the kernel to the program
        p.add_kernel(k)

        # compile the program
        p.compile()

        # load qasm
        qasm_files = []
        qasm_files.append(os.path.join(output_dir, 'aProgram.qasm'))
        qasm_files.append(os.path.join(output_dir, 'aProgram_scheduled.qasm'))

        # TODO use new cqasm v1.0 interface
        # for qasm_file in qasm_files:
        #    qasm_reader = ql.QASM_Loader(qasm_file)
        #    errors = qasm_reader.load()
        #    self.assertTrue(errors == 0)

        QISA_fn = os.path.join(output_dir, p.name+'.qisa')
        assemble(QISA_fn)

	# @unittest.skip
    def test_qwg_available_01(self):

        config_fn = os.path.join(curdir, 'hardware_config_cc_light.json')
        platform  = ql.Platform('seven_qubits_chip', config_fn)
        sweep_points = [1,2]
        num_qubits = 7
        p = ql.Program('aProgram', platform, num_qubits)
        p.set_sweep_points(sweep_points, len(sweep_points))

        # populate kernel using default gates
        k = ql.Kernel('aKernel', platform, num_qubits)

        k.gate('h', [0])
        k.x(2)

        # add the kernel to the program
        p.add_kernel(k)

        # compile the program
        p.compile()

        # load qasm
        qasm_files = []
        qasm_files.append(os.path.join(output_dir, 'aProgram.qasm'))
        qasm_files.append(os.path.join(output_dir, 'aProgram_scheduled.qasm'))

        # TODO use new cqasm v1.0 interface
        # for qasm_file in qasm_files:
        #    qasm_reader = ql.QASM_Loader(qasm_file)
        #    errors = qasm_reader.load()
        #    self.assertTrue(errors == 0)

        QISA_fn = os.path.join(output_dir, p.name+'.qisa')
        assemble(QISA_fn)

	# @unittest.skip
    def test_qwg_available_02(self):

        config_fn = os.path.join(curdir, 'hardware_config_cc_light.json')
        platform  = ql.Platform('seven_qubits_chip', config_fn)
        sweep_points = [1,2]
        num_qubits = 7
        p = ql.Program('aProgram', platform, num_qubits)
        p.set_sweep_points(sweep_points, len(sweep_points))

        # populate kernel using default gates
        k = ql.Kernel('aKernel', platform, num_qubits)

        k.x(0)
        k.x(1)

        # add the kernel to the program
        p.add_kernel(k)

        # compile the program
        p.compile()

        # load qasm
        qasm_files = []
        qasm_files.append(os.path.join(output_dir, 'aProgram.qasm'))
        qasm_files.append(os.path.join(output_dir, 'aProgram_scheduled.qasm'))

        # TODO use new cqasm v1.0 interface
        # for qasm_file in qasm_files:
        #    qasm_reader = ql.QASM_Loader(qasm_file)
        #    errors = qasm_reader.load()
        #    self.assertTrue(errors == 0)

        QISA_fn = os.path.join(output_dir, p.name+'.qisa')
        assemble(QISA_fn)

	# @unittest.skip
    def test_qwg_busy(self):

        config_fn = os.path.join(curdir, 'hardware_config_cc_light.json')
        platform  = ql.Platform('seven_qubits_chip', config_fn)
        sweep_points = [1,2]
        num_qubits = 7
        p = ql.Program('aProgram', platform, num_qubits)
        p.set_sweep_points(sweep_points, len(sweep_points))

        # populate kernel using default gates
        k = ql.Kernel('aKernel', platform, num_qubits)

        k.gate('h', [0])
        k.x(1)

        # add the kernel to the program
        p.add_kernel(k)

        # compile the program
        p.compile()

        # load qasm
        qasm_files = []
        qasm_files.append(os.path.join(output_dir, 'aProgram.qasm'))
        qasm_files.append(os.path.join(output_dir, 'aProgram_scheduled.qasm'))

        # TODO use new cqasm v1.0 interface
        # for qasm_file in qasm_files:
        #    qasm_reader = ql.QASM_Loader(qasm_file)
        #    errors = qasm_reader.load()
        #    self.assertTrue(errors == 0)

	# @unittest.skip
    def test_measure_available01(self):

        config_fn = os.path.join(curdir, 'hardware_config_cc_light.json')
        platform  = ql.Platform('seven_qubits_chip', config_fn)
        sweep_points = [1,2]
        num_qubits = 7
        p = ql.Program('aProgram', platform, num_qubits)
        p.set_sweep_points(sweep_points, len(sweep_points))

        k = ql.Kernel('aKernel', platform, num_qubits)

        # following should be packed together as measure resource is available
        # this is because measurement is starting on different meas unit
        k.measure(0)
        k.measure(1)

        # add the kernel to the program
        p.add_kernel(k)

        # compile the program
        p.compile()

        QISA_fn = os.path.join(output_dir, p.name+'.qisa')
        assemble(QISA_fn)

        # load qasm
        qasm_files = []
        qasm_files.append(os.path.join(output_dir, 'aProgram.qasm'))
        qasm_files.append(os.path.join(output_dir, 'aProgram_scheduled.qasm'))

        # TODO use new cqasm v1.0 interface
        # for qasm_file in qasm_files:
        #    qasm_reader = ql.QASM_Loader(qasm_file)
        #    errors = qasm_reader.load()
        #    self.assertTrue(errors == 0)

	# @unittest.skip
    def test_measure_available02(self):

        config_fn = os.path.join(curdir, 'hardware_config_cc_light.json')
        platform  = ql.Platform('seven_qubits_chip', config_fn)
        sweep_points = [1,2]
        num_qubits = 7
        p = ql.Program('aProgram', platform, num_qubits)
        p.set_sweep_points(sweep_points, len(sweep_points))

        # populate kernel using default gates
        k = ql.Kernel('aKernel', platform, num_qubits)

        # following should be packed together as measure resource is available
        # this is because measurement is starting in same cycle on same meas unit
        k.measure(0)
        k.measure(2)

        # add the kernel to the program
        p.add_kernel(k)

        # compile the program
        p.compile()

        # load qasm
        qasm_files = []
        qasm_files.append(os.path.join(output_dir, 'aProgram.qasm'))
        qasm_files.append(os.path.join(output_dir, 'aProgram_scheduled.qasm'))

        # TODO use new cqasm v1.0 interface
        # for qasm_file in qasm_files:
        #    qasm_reader = ql.QASM_Loader(qasm_file)
        #    errors = qasm_reader.load()
        #    self.assertTrue(errors == 0)

	# @unittest.skip
    def test_measure_busy(self):

        config_fn = os.path.join(curdir, 'hardware_config_cc_light.json')
        platform  = ql.Platform('seven_qubits_chip', config_fn)
        sweep_points = [1,2]
        num_qubits = 7
        p = ql.Program('aProgram', platform, num_qubits)
        p.set_sweep_points(sweep_points, len(sweep_points))

        # populate kernel using default gates
        k = ql.Kernel('aKernel', platform, num_qubits)

        # following should not be packed together as measurement is starting on same
        # meas unit which is busy
        k.measure(0)
        k.wait([2], 20)
        k.measure(2)

        # add the kernel to the program
        p.add_kernel(k)

        # compile the program
        p.compile()

        QISA_fn = os.path.join(output_dir, p.name+'.qisa')
        assemble(QISA_fn)

        # load qasm
        qasm_files = []
        qasm_files.append(os.path.join(output_dir, 'aProgram.qasm'))
        qasm_files.append(os.path.join(output_dir, 'aProgram_scheduled.qasm'))

        # TODO use new cqasm v1.0 interface
        # for qasm_file in qasm_files:
        #    qasm_reader = ql.QASM_Loader(qasm_file)
        #    errors = qasm_reader.load()
        #    self.assertTrue(errors == 0)

	# @unittest.skip
    def test_edge_available(self):

        config_fn = os.path.join(curdir, 'hardware_config_cc_light.json')
        platform  = ql.Platform('seven_qubits_chip', config_fn)
        sweep_points = [1,2]
        num_qubits = 7
        p = ql.Program('aProgram', platform, num_qubits)
        p.set_sweep_points(sweep_points, len(sweep_points))

        # populate kernel using default gates
        k = ql.Kernel('aKernel', platform, num_qubits)

        k.gate('cz', [0, 2])
        k.gate('cz', [4, 6])

        # add the kernel to the program
        p.add_kernel(k)

        # compile the program
        p.compile()

        # load qasm
        qasm_files = []
        qasm_files.append(os.path.join(output_dir, 'aProgram.qasm'))
        qasm_files.append(os.path.join(output_dir, 'aProgram_scheduled.qasm'))

        # TODO use new cqasm v1.0 interface
        # for qasm_file in qasm_files:
        #    qasm_reader = ql.QASM_Loader(qasm_file)
        #    errors = qasm_reader.load()
        #    self.assertTrue(errors == 0)

	# @unittest.skip
    def test_edge_busy(self):

        config_fn = os.path.join(curdir, 'hardware_config_cc_light.json')
        platform  = ql.Platform('seven_qubits_chip', config_fn)
        sweep_points = [1,2]
        num_qubits = 7
        p = ql.Program('aProgram', platform, num_qubits)
        p.set_sweep_points(sweep_points, len(sweep_points))

        # populate kernel using default gates
        k = ql.Kernel('aKernel', platform, num_qubits)

        k.gate('cz', [0, 2])
        k.gate('cz', [1, 3])

        # add the kernel to the program
        p.add_kernel(k)

        # compile the program
        p.compile()

        QISA_fn = os.path.join(output_dir, p.name+'.qisa')
        assemble(QISA_fn)

        # load qasm
        qasm_files = []
        qasm_files.append(os.path.join(output_dir, 'aProgram.qasm'))
        qasm_files.append(os.path.join(output_dir, 'aProgram_scheduled.qasm'))

        # TODO use new cqasm v1.0 interface
        # for qasm_file in qasm_files:
        #    qasm_reader = ql.QASM_Loader(qasm_file)
        #    errors = qasm_reader.load()
        #    self.assertTrue(errors == 0)

	# @unittest.skip
    def test_edge_illegal(self):

        config_fn = os.path.join(curdir, 'hardware_config_cc_light.json')
        platform  = ql.Platform('seven_qubits_chip', config_fn)
        sweep_points = [1,2]
        num_qubits = 7
        p = ql.Program('aProgram', platform, num_qubits)
        p.set_sweep_points(sweep_points, len(sweep_points))

        # populate kernel using default gates
        k = ql.Kernel('aKernel', platform, num_qubits)

        k.cphase(0, 2)
        k.cphase(0, 1)

        # add the kernel to the program
        p.add_kernel(k)

        # compile the program
        try:
            p.compile()
            raise
        except:
            pass

        # load qasm
        qasm_files = []
        qasm_files.append(os.path.join(output_dir, 'aProgram.qasm'))
        qasm_files.append(os.path.join(output_dir, 'aProgram_scheduled.qasm'))

        # TODO use new cqasm v1.0 interface
        # for qasm_file in qasm_files:
        #    qasm_reader = ql.QASM_Loader(qasm_file)
        #    errors = qasm_reader.load()
        #    self.assertTrue(errors == 0)

    # fast feedback test
    # @unittest.expectedFailure
    # @unittest.skip
    def test_fast_feedback(self):

        # You can specify a config location, here we use a default config
        config_fn = os.path.join(curdir, 'hardware_config_cc_light.json')
        platform = ql.Platform('seven_qubits_chip', config_fn)
        sweep_points = [1, 2]
        num_qubits = platform.get_qubit_number()
        p = ql.Program('aProgram', platform, num_qubits)
        p.set_sweep_points(sweep_points, len(sweep_points))

        # populate kernel using default gates
        k = ql.Kernel('aKernel', platform, num_qubits)
        k.prepz(0)
        k.gate('cprepz', [1])
        k.measure(0)
        k.measure(1)

        # add the kernel to the program
        p.add_kernel(k)

        # compile the program
        p.compile()

        QISA_fn = os.path.join(output_dir, p.name+'.qisa')
        assemble(QISA_fn)

        # load qasm
        # qasm_files = []
        # qasm_files.append(os.path.join(output_dir, 'aProgram.qasm'))

        # TODO use new cqasm v1.0 interface
        # for qasm_file in qasm_files:
        #    qasm_reader = ql.QASM_Loader(qasm_file)
        #    errors = qasm_reader.load()
        #    self.assertTrue(errors == 0)


    def test_ccl_buffers(self):

        tests = [
                # mw - mw buffer (same qubit and of course same awg channel is involved)
                            (0, [ ("x",[0]), ("y",[0]) ]),
                # mw - mw buffer (different qubit but same awg channel is involved)
                            (1, [ ("x",[0]), ("y",[1]) ]),
                # mw - mw buffer (different qubit and different awg channel)
                            (2, [ ("x",[0]), ("y",[2]) ]),
                # mw - flux
                            (3, [ ("x",[2]), ("cnot", [0,2]) ]),
                # mw - readout
                            (4, [ ("x",[0]), ("measure", [0]) ]),
                # flux - flux
                            (5, [ ("cnot", [0,2]), ("cnot", [0,2]) ]),
                # flux - mw
                            (6, [ ("cnot", [0,2]), ("x", [2]) ]),
                # flux - readout
                            (7, [("cnot", [0,2]), ("measure", [2])]),
                # readout - readout
                            (8, [("measure", [0]), ("measure", [0])]),
                # readout - mw
                            (9, [("measure", [0]), ("x", [0])]),
                # readout - flux
                            (10, [("measure", [0]), ("cnot", [0,2]) ])
                ]

        for testNo, testKernel in tests:
            print('Running test_ccl_buffers No: {}'.format(testNo))
            config_fn = os.path.join(curdir, 'test_cfg_cc_light_buffers_latencies.json')
            platform  = ql.Platform('seven_qubits_chip', config_fn)
            sweep_points = [1,2]
            num_qubits = 7
            p = ql.Program('test_ccl_buffers'+str(testNo), platform, num_qubits)
            p.set_sweep_points(sweep_points, len(sweep_points))
            k = ql.Kernel('aKernel', platform, num_qubits)

            for gate, qubits in testKernel:
                k.gate(gate, qubits)

            p.add_kernel(k)
            p.compile()

            # load qasm
            qasm_files = []
            qasm_files.append(os.path.join(output_dir, p.name+'.qasm'))
            qasm_files.append(os.path.join(output_dir, p.name+'_scheduled.qasm'))

            # TODO use new cqasm v1.0 interface
            # for qasm_file in qasm_files:
            #    qasm_reader = ql.QASM_Loader(qasm_file)
            #    errors = qasm_reader.load()
            #    self.assertTrue(errors == 0)

            QISA_fn = os.path.join(output_dir, p.name+'.qisa')
            gold_fn = rootDir + '/golden/test_ccl_buffers_'+str(testNo)+'.qisa'        

            assemble(QISA_fn)

            self.assertTrue( file_compare(QISA_fn, gold_fn) )


    def test_ccl_latencies(self):

        tests = [
                # 'y q3' has a latency of 0 ns
                            (0, [("x", [0]), ("y", [3])]),
                # 'y q4' has a latency of +20 ns
                            (1, [("x", [0]),("y", [4])]),
                # 'y q5' has a latency of -20 ns
                            (2, [("x", [0]),("y", [5])]),
                # qubit dependence and 'y q3' has a latency of 0 ns
                            (3, [("x", [3]),("y", [3])]),
                # qubit dependence and 'y q4' has a latency of +20 ns
                            (4, [("x", [4]),("y", [4])]),
                # qubit dependence and 'y q5' has a latency of -20 ns
                            (5, [("x", [5]),("y", [5])])
                ]

        for testNo, testKernel in tests:
            print('Running test_ccl_latencies No: {}'.format(testNo))
            config_fn = os.path.join(curdir, 'test_cfg_cc_light_buffers_latencies.json')
            platform  = ql.Platform('seven_qubits_chip', config_fn)
            sweep_points = [1,2]
            num_qubits = 7
            p = ql.Program('test_ccl_latencies'+str(testNo), platform, num_qubits)
            p.set_sweep_points(sweep_points, len(sweep_points))
            k = ql.Kernel('aKernel', platform, num_qubits)

            for gate, qubits in testKernel:
                k.gate(gate, qubits)

            p.add_kernel(k)
            p.compile()

            # load qasm
            qasm_files = []
            qasm_files.append(os.path.join(output_dir, p.name+'.qasm'))
            qasm_files.append(os.path.join(output_dir, p.name+'_scheduled.qasm'))

            # TODO use new cqasm v1.0 interface
            # for qasm_file in qasm_files:
            #    qasm_reader = ql.QASM_Loader(qasm_file)
            #    errors = qasm_reader.load()
            #    self.assertTrue(errors == 0)

            QISA_fn = os.path.join(output_dir, p.name+'.qisa')
            gold_fn = rootDir + '/golden/test_ccl_latencies_'+str(testNo)+'.qisa'        

            assemble(QISA_fn)

            self.assertTrue( file_compare(QISA_fn, gold_fn) )


if __name__ == '__main__':
    unittest.main()
