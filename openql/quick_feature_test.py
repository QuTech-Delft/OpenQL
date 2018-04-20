import os
import openql as ql
import numpy as np

curdir = os.path.dirname(__file__)
output_dir = os.path.join(curdir, 'test_output')
ql.set_option("output_dir", output_dir)

def test_cclight():
    config_fn = os.path.join(curdir, '../tests/hardware_config_cc_light.json')
    platform  = ql.Platform('seven_qubits_chip', config_fn)
    num_qubits = 7
    p = ql.Program('aProgram', num_qubits, platform)
    sweep_points = [1,2]
    p.set_sweep_points(sweep_points, len(sweep_points))

    k = ql.Kernel('aKernel', platform)

    for i in range(4):
        k.prepz(i)

    k.x(2)
    k.gate("wait", [1,2], 100)
    k.x(2)
    k.x(3)
    k.cnot(2, 0)
    k.cnot(1, 4)

    for i in range(4):
        k.measure(i)

    p.add_kernel(k)

    p.compile(optimize=False, scheduler='ASAP', log_level='LOG_INFO')
    # p.compile(optimize=False, scheduler='ALAP', log_level='LOG_INFO')

def test_none():
    config_fn = os.path.join(curdir, '../tests/test_cfg_none.json')
    platform  = ql.Platform('platform_none', config_fn)
    num_qubits = 5
    p = ql.Program('aProgram', num_qubits, platform)

    k = ql.Kernel('aKernel', platform)

    k.gate("x",0)
    k.gate("x",0)
    k.gate("cz", [0, 1])
    k.gate("cz", [2, 3])

    p.add_kernel(k)

    p.compile(optimize=False, scheduler='ALAP', log_level='LOG_INFO')

def test_quantumsim():
    config_fn = os.path.join(curdir, '../tests/test_cfg_quantumsim.json')
    platform  = ql.Platform('platform_none', config_fn)
    num_qubits = 2
    k = ql.Kernel('aKernel', platform)

    k.gate("hadamard",0)
    k.gate("hadamard",1)
    k.gate("cphase", [0, 1])
    k.gate("measure", 0)
    k.gate("measure", 1)

    p = ql.Program('aProgram', num_qubits, platform)
    p.add_kernel(k)
    p.compile(optimize=False, scheduler='ASAP', log_level='LOG_INFO')

def test_display():
    ql.set_option('log_level', 'LOG_INFO')
    ql.set_option('use_default_gates', 'yes')
    ql.set_option('scheduler', 'ALAP')
    ql.print_options()

    config_fn = os.path.join(curdir, '../tests/test_config_default.json')
    platform  = ql.Platform('platform_none', config_fn)

    k = ql.Kernel("aKernel", platform)

    for i in range(4):
        k.prepz(i)

    # q1 dependence
    k.cz(0, 1)
    k.cz(2, 1)

    k.measure(0)
    k.measure(1)

    sweep_points = [2]
    num_circuits = 1
    nqubits = 4

    p = ql.Program("WAW", nqubits, platform)
    p.set_sweep_points(sweep_points, len(sweep_points))
    p.add_kernel(k)

    p.compile()

if __name__ == '__main__':
    test_display()


    # LOG_NOTHING,
    # LOG_CRITICAL,
    # LOG_ERROR,
    # LOG_WARNING,
    # LOG_INFO,
    # LOG_DEBUG
