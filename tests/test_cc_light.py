import os
import unittest
from openql import openql as ql
from utils import file_compare

curdir = os.path.dirname(os.path.realpath(__file__))
output_dir = os.path.join(curdir, 'test_output')

class Test_basic(unittest.TestCase):

    @classmethod
    def setUp(self):
        ql.initialize()

        ql.set_option('output_dir', output_dir)
        ql.set_option('optimize', 'no')

        # TODO cleanup
        ql.set_option('scheduler', 'ASAP');
        ql.set_option('scheduler_post179', 'yes');

        ql.set_option('log_level', 'LOG_WARNING')

    # single qubit mask generation test
    def test_smis(self):
        # You can specify a config location, here we use a default config
        config_fn = os.path.join(curdir, 'hardware_config_cc_light.json')
        platform = ql.Platform('seven_qubits_chip', config_fn)
        sweep_points = [1, 2]
        num_qubits = platform.get_qubit_number()
        p = ql.Program('test_smis', platform, num_qubits)
        p.set_sweep_points(sweep_points)

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

        GOLD_fn = os.path.join(curdir, 'golden', p.name + '.qisa')
        QISA_fn = os.path.join(output_dir, p.name+'.qisa')

        self.assertTrue(file_compare(QISA_fn, GOLD_fn))

    # single qubit mask generation test with custom gates
    def test_smis_with_custom_gates(self):

        # You can specify a config location, here we use a default config
        config_fn = os.path.join(curdir, 'hardware_config_cc_light.json')
        platform = ql.Platform('seven_qubits_chip', config_fn)
        sweep_points = [1, 2]
        num_qubits = 7
        p = ql.Program('test_smis_with_custom_gates', platform, num_qubits)
        p.set_sweep_points(sweep_points)

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

        GOLD_fn = os.path.join(curdir, 'golden', p.name + '.qisa')
        QISA_fn = os.path.join(output_dir, p.name+'.qisa')

        self.assertTrue(file_compare(QISA_fn, GOLD_fn))


    # single qubit mask generation multi-kernel test (custom with non-custom
    # gates)
    def test_smis_multi_kernel(self):

        # You can specify a config location, here we use a default config
        config_fn = os.path.join(curdir, 'hardware_config_cc_light.json')
        platform = ql.Platform('seven_qubits_chip', config_fn)
        sweep_points = [1, 2]
        num_qubits = 7
        p = ql.Program('test_smis_multi_kernel', platform, num_qubits)
        p.set_sweep_points(sweep_points)

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

        GOLD_fn = os.path.join(curdir, 'golden', p.name + '.qisa')
        QISA_fn = os.path.join(output_dir, p.name+'.qisa')

        self.assertTrue(file_compare(QISA_fn, GOLD_fn))


    def test_smis_all_bundled(self):

        # You can specify a config location, here we use a default config
        config_fn = os.path.join(curdir, 'hardware_config_cc_light.json')
        platform = ql.Platform('seven_qubits_chip', config_fn)
        sweep_points = [1, 2]
        num_qubits = platform.get_qubit_number()
        p = ql.Program('test_smis_all_bundled', platform, num_qubits)
        p.set_sweep_points(sweep_points)

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
        GOLD_fn = curdir + '/golden/test_smis_all_bundled.qisa'

        self.assertTrue( file_compare(QISA_fn, GOLD_fn) )


    # two qubit mask generation test
    def test_smit(self):

        # You can specify a config location, here we use a default config
        config_fn = os.path.join(curdir, 'hardware_config_cc_light.json')
        platform = ql.Platform('seven_qubits_chip', config_fn)
        sweep_points = [1, 2]
        num_qubits = 7
        p = ql.Program('test_smit', platform, num_qubits)
        p.set_sweep_points(sweep_points)

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

        GOLD_fn = os.path.join(curdir, 'golden', p.name + '.qisa')
        QISA_fn = os.path.join(output_dir, p.name+'.qisa')

        self.assertTrue(file_compare(QISA_fn, GOLD_fn))


    def test_smit_all_bundled(self):

        # You can specify a config location, here we use a default config
        config_fn = os.path.join(curdir, 'hardware_config_cc_light.json')
        platform = ql.Platform('seven_qubits_chip', config_fn)
        sweep_points = [1, 2]
        num_qubits = platform.get_qubit_number()
        p = ql.Program('test_smit_all_bundled', platform, num_qubits)
        p.set_sweep_points(sweep_points)

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

        GOLD_fn = os.path.join(curdir, 'golden', p.name + '.qisa')
        QISA_fn = os.path.join(output_dir, p.name+'.qisa')

        self.assertTrue(file_compare(QISA_fn, GOLD_fn))

class Test_advance(unittest.TestCase):

    @classmethod
    def setUp(self):
        ql.initialize()

    def test_qubit_busy(self):
        ql.set_option('output_dir', output_dir)
        config_fn = os.path.join(curdir, 'hardware_config_cc_light.json')
        platform  = ql.Platform('seven_qubits_chip', config_fn)
        sweep_points = [1,2]
        num_qubits = 7
        p = ql.Program('test_qubit_busy', platform, num_qubits)
        p.set_sweep_points(sweep_points)

        # populate kernel using default gates
        k = ql.Kernel('aKernel', platform, num_qubits)

        k.prepz(0)
        k.prepz(0)

        # add the kernel to the program
        p.add_kernel(k)

        # compile the program
        p.compile()

        GOLD_fn = os.path.join(curdir, 'golden', p.name + '.qisa')
        QISA_fn = os.path.join(output_dir, p.name+'.qisa')

        self.assertTrue(file_compare(QISA_fn, GOLD_fn))


    def test_qwg_available_01(self):
        ql.set_option('output_dir', output_dir)
        config_fn = os.path.join(curdir, 'hardware_config_cc_light.json')
        platform  = ql.Platform('seven_qubits_chip', config_fn)
        sweep_points = [1,2]
        num_qubits = 7
        p = ql.Program('test_qwg_available_01', platform, num_qubits)
        p.set_sweep_points(sweep_points)

        # populate kernel using default gates
        k = ql.Kernel('aKernel', platform, num_qubits)

        k.gate('h', [0])
        k.x(2)

        # add the kernel to the program
        p.add_kernel(k)

        # compile the program
        p.compile()

        GOLD_fn = os.path.join(curdir, 'golden', p.name + '.qisa')
        QISA_fn = os.path.join(output_dir, p.name+'.qisa')

        self.assertTrue(file_compare(QISA_fn, GOLD_fn))


    def test_qwg_available_02(self):
        ql.set_option('output_dir', output_dir)
        config_fn = os.path.join(curdir, 'hardware_config_cc_light.json')
        platform  = ql.Platform('seven_qubits_chip', config_fn)
        sweep_points = [1,2]
        num_qubits = 7
        p = ql.Program('test_qwg_available_02', platform, num_qubits)
        p.set_sweep_points(sweep_points)

        # populate kernel using default gates
        k = ql.Kernel('aKernel', platform, num_qubits)

        k.x(0)
        k.x(1)

        # add the kernel to the program
        p.add_kernel(k)

        # compile the program
        p.compile()

        GOLD_fn = os.path.join(curdir, 'golden', p.name + '.qisa')
        QISA_fn = os.path.join(output_dir, p.name+'.qisa')

        self.assertTrue(file_compare(QISA_fn, GOLD_fn))


    def test_qwg_busy(self):
        ql.set_option('output_dir', output_dir)
        config_fn = os.path.join(curdir, 'hardware_config_cc_light.json')
        platform  = ql.Platform('seven_qubits_chip', config_fn)
        sweep_points = [1,2]
        num_qubits = 7
        p = ql.Program('test_qwg_busy', platform, num_qubits)
        p.set_sweep_points(sweep_points)

        # populate kernel using default gates
        k = ql.Kernel('aKernel', platform, num_qubits)

        k.gate('h', [0])
        k.x(1)

        # add the kernel to the program
        p.add_kernel(k)

        # compile the program
        p.compile()

        GOLD_fn = os.path.join(curdir, 'golden', p.name + '.qisa')
        QISA_fn = os.path.join(output_dir, p.name+'.qisa')

        self.assertTrue(file_compare(QISA_fn, GOLD_fn))


    def test_measure_available01(self):
        ql.set_option('output_dir', output_dir)
        config_fn = os.path.join(curdir, 'hardware_config_cc_light.json')
        platform  = ql.Platform('seven_qubits_chip', config_fn)
        sweep_points = [1,2]
        num_qubits = 7
        p = ql.Program('test_measure_available01', platform, num_qubits)
        p.set_sweep_points(sweep_points)

        k = ql.Kernel('aKernel', platform, num_qubits)

        # following should be packed together as measure resource is available
        # this is because measurement is starting on different meas unit
        k.measure(0)
        k.measure(1)

        # add the kernel to the program
        p.add_kernel(k)

        # compile the program
        p.compile()

        GOLD_fn = os.path.join(curdir, 'golden', p.name + '.qisa')
        QISA_fn = os.path.join(output_dir, p.name+'.qisa')

        self.assertTrue(file_compare(QISA_fn, GOLD_fn))


    def test_measure_available02(self):
        ql.set_option('output_dir', output_dir)
        config_fn = os.path.join(curdir, 'hardware_config_cc_light.json')
        platform  = ql.Platform('seven_qubits_chip', config_fn)
        sweep_points = [1,2]
        num_qubits = 7
        p = ql.Program('test_measure_available02', platform, num_qubits)
        p.set_sweep_points(sweep_points)

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

        GOLD_fn = os.path.join(curdir, 'golden', p.name + '.qisa')
        QISA_fn = os.path.join(output_dir, p.name+'.qisa')

        self.assertTrue(file_compare(QISA_fn, GOLD_fn))


    def test_measure_busy(self):
        ql.set_option('output_dir', output_dir)
        config_fn = os.path.join(curdir, 'hardware_config_cc_light.json')
        platform  = ql.Platform('seven_qubits_chip', config_fn)
        sweep_points = [1,2]
        num_qubits = 7
        p = ql.Program('test_measure_busy', platform, num_qubits)
        p.set_sweep_points(sweep_points)

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

        GOLD_fn = os.path.join(curdir, 'golden', p.name + '.qisa')
        QISA_fn = os.path.join(output_dir, p.name+'.qisa')

        self.assertTrue(file_compare(QISA_fn, GOLD_fn))


    def test_edge_available(self):
        ql.set_option('output_dir', output_dir)
        config_fn = os.path.join(curdir, 'hardware_config_cc_light.json')
        platform  = ql.Platform('seven_qubits_chip', config_fn)
        sweep_points = [1,2]
        num_qubits = 7
        p = ql.Program('test_edge_available', platform, num_qubits)
        p.set_sweep_points(sweep_points)

        # populate kernel using default gates
        k = ql.Kernel('aKernel', platform, num_qubits)

        k.gate('cz', [0, 2])
        k.gate('cz', [4, 6])

        # add the kernel to the program
        p.add_kernel(k)

        # compile the program
        p.compile()

        GOLD_fn = os.path.join(curdir, 'golden', p.name + '.qisa')
        QISA_fn = os.path.join(output_dir, p.name+'.qisa')

        self.assertTrue(file_compare(QISA_fn, GOLD_fn))


    def test_edge_busy(self):
        ql.set_option('output_dir', output_dir)
        config_fn = os.path.join(curdir, 'hardware_config_cc_light.json')
        platform  = ql.Platform('seven_qubits_chip', config_fn)
        sweep_points = [1,2]
        num_qubits = 7
        p = ql.Program('test_edge_busy', platform, num_qubits)
        p.set_sweep_points(sweep_points)

        # populate kernel using default gates
        k = ql.Kernel('aKernel', platform, num_qubits)

        k.gate('cz', [0, 2])
        k.gate('cz', [1, 3])

        # add the kernel to the program
        p.add_kernel(k)

        # compile the program
        p.compile()

        GOLD_fn = os.path.join(curdir, 'golden', p.name + '.qisa')
        QISA_fn = os.path.join(output_dir, p.name+'.qisa')

        self.assertTrue(file_compare(QISA_fn, GOLD_fn))


    def test_edge_illegal(self):
        ql.set_option('output_dir', output_dir)
        config_fn = os.path.join(curdir, 'hardware_config_cc_light.json')
        platform  = ql.Platform('seven_qubits_chip', config_fn)
        sweep_points = [1,2]
        num_qubits = 7
        p = ql.Program('test_edge_illegal', platform, num_qubits)
        p.set_sweep_points(sweep_points)

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


    # fast feedback test
    def test_fast_feedback(self):

        # You can specify a config location, here we use a default config
        ql.set_option('output_dir', output_dir)
        config_fn = os.path.join(curdir, 'hardware_config_cc_light.json')
        platform = ql.Platform('seven_qubits_chip', config_fn)
        sweep_points = [1, 2]
        num_qubits = platform.get_qubit_number()
        p = ql.Program('test_fast_feedback', platform, num_qubits)
        p.set_sweep_points(sweep_points)

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

        GOLD_fn = os.path.join(curdir, 'golden', p.name + '.qisa')
        QISA_fn = os.path.join(output_dir, p.name+'.qisa')

        self.assertTrue(file_compare(QISA_fn, GOLD_fn))

    @unittest.skip
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
            ql.set_option('output_dir', output_dir)
            print('Running test_ccl_buffers No: {}'.format(testNo))
            config_fn = os.path.join(curdir, 'test_cfg_cc_light_buffers_latencies.json')
            platform  = ql.Platform('seven_qubits_chip', config_fn)
            sweep_points = [1,2]
            num_qubits = 7
            p = ql.Program('test_ccl_buffers'+str(testNo), platform, num_qubits)
            p.set_sweep_points(sweep_points)
            k = ql.Kernel('aKernel', platform, num_qubits)

            for gate, qubits in testKernel:
                k.gate(gate, qubits)

            p.add_kernel(k)
            p.compile()

            GOLD_fn = os.path.join(curdir, 'golden', p.name + '.qisa')
            QISA_fn = os.path.join(output_dir, p.name+'.qisa')

            self.assertTrue(file_compare(QISA_fn, GOLD_fn))


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
            ql.set_option('output_dir', output_dir)
            config_fn = os.path.join(curdir, 'test_cfg_cc_light_buffers_latencies.json')
            platform  = ql.Platform('seven_qubits_chip', config_fn)
            sweep_points = [1,2]
            num_qubits = 7
            p = ql.Program('test_ccl_latencies'+str(testNo), platform, num_qubits)
            p.set_sweep_points(sweep_points)
            k = ql.Kernel('aKernel', platform, num_qubits)

            for gate, qubits in testKernel:
                k.gate(gate, qubits)

            p.add_kernel(k)
            p.compile()

            GOLD_fn = os.path.join(curdir, 'golden', p.name + '.qisa')
            QISA_fn = os.path.join(output_dir, p.name+'.qisa')

            self.assertTrue(file_compare(QISA_fn, GOLD_fn))

    def test_single_qubit_flux_manual01(self):
        ql.set_option('output_dir', output_dir)
        ql.set_option('cz_mode', 'manual')
        config_fn = os.path.join(curdir, 'hardware_config_cc_light.json')
        platform  = ql.Platform('seven_qubits_chip', config_fn)
        num_qubits = platform.get_qubit_number()

        p = ql.Program('test_single_qubit_flux_manual01', platform, num_qubits)
        k = ql.Kernel('aKernel', platform, num_qubits)

        k.gate('prepz', [0])
        k.gate('prepz', [2])
        k.gate('sqf', [2])
        p.add_kernel(k)
        p.compile()

        GOLD_fn = os.path.join(curdir, 'golden', p.name + '.qisa')
        QISA_fn = os.path.join(output_dir, p.name+'.qisa')

        self.assertTrue(file_compare(QISA_fn, GOLD_fn))

    def test_single_qubit_flux_manual02(self):
        ql.set_option('output_dir', output_dir)
        ql.set_option('cz_mode', 'manual')
        config_fn = os.path.join(curdir, 'hardware_config_cc_light.json')
        platform  = ql.Platform('seven_qubits_chip', config_fn)
        num_qubits = platform.get_qubit_number()

        p = ql.Program('test_single_qubit_flux_manual02', platform, num_qubits)
        k = ql.Kernel('aKernel', platform, num_qubits)

        #use barrier to manually insert single qubit flux operations
        k.gate('prepz', [0])
        k.gate('prepz', [2])
        k.gate('barrier', [])
        k.gate('sqf', [5])
        k.gate('sqf', [6])
        k.gate('cz', [2, 0])
        k.gate('barrier', [])
        p.add_kernel(k)
        p.compile()

        GOLD_fn = os.path.join(curdir, 'golden', p.name + '.qisa')
        QISA_fn = os.path.join(output_dir, p.name+'.qisa')

        self.assertTrue(file_compare(QISA_fn, GOLD_fn))

    def test_single_qubit_flux_auto(self):
        ql.set_option('output_dir', output_dir)
        ql.set_option('cz_mode', 'auto')
        config_fn = os.path.join(curdir, 'hardware_config_cc_light.json')
        platform  = ql.Platform('seven_qubits_chip', config_fn)
        num_qubits = platform.get_qubit_number()

        p = ql.Program('test_single_qubit_flux_auto', platform, num_qubits)
        k = ql.Kernel('aKernel', platform, num_qubits)

        k.gate('prepz', [0])
        k.gate('prepz', [2])
        k.gate('cz', [2, 0])
        k.gate('measure', [0])
        k.gate('measure', [2])

        p.add_kernel(k)
        p.compile()

        GOLD_fn = os.path.join(curdir, 'golden', p.name + '.qisa')
        QISA_fn = os.path.join(output_dir, p.name+'.qisa')

        self.assertTrue(file_compare(QISA_fn, GOLD_fn))

if __name__ == '__main__':
    unittest.main()
