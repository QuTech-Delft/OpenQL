import os
import openql as ql
import numpy as np

curdir = os.path.dirname(__file__)
output_dir = os.path.join(curdir, 'test_output')

ql.set_option('output_dir', output_dir)
ql.set_option('optimize', 'no')
ql.set_option('scheduler', 'ASAP')
ql.set_option('log_level', 'LOG_WARNING')

def test_cclight():
    config_fn = os.path.join(curdir, '../tests/hardware_config_cc_light.json')
    platform  = ql.Platform('seven_qubits_chip', config_fn)
    num_qubits = 7
    p = ql.Program('test_cclight', num_qubits, platform)
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
    p.compile()

def test_none():
    config_fn = os.path.join(curdir, '../tests/test_cfg_none.json')
    platform  = ql.Platform('platform_none', config_fn)
    num_qubits = 5
    p = ql.Program('test_none', num_qubits, platform)

    k = ql.Kernel('aKernel', platform)

    k.gate("x",0)
    k.gate("x",0)
    k.gate("cz", [0, 1])
    k.gate("cz", [2, 3])

    p.add_kernel(k)
    p.compile()

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

    p = ql.Program('test_quantumsim', num_qubits, platform)
    p.add_kernel(k)
    p.compile()

def test_controlled_kernel():
    config_fn = os.path.join(curdir, '../tests/test_cfg_none_simple.json')
    platform  = ql.Platform('platform_none', config_fn)
    num_qubits = 4
    p = ql.Program('test_controlled_kernel', num_qubits, platform)

    k = ql.Kernel('kernel1', platform)
    ck = ql.Kernel('controlled_kernel1', platform)

    k.gate("x", [1])
    # k.gate("y", [1])
    # k.gate("z", [1])
    # k.gate("h", [1])
    # k.gate("i", [1])
    # k.gate("s", [1])
    # k.gate("t", [1])
    # k.gate("sdag", [1])
    # k.gate("tdag", [1])

    # generate controlled version of k. qubit 2 is used as control qubit
    # ck.controlled(k, [2])

    p.add_kernel(k)
    p.add_kernel(ck)

    p.compile()


if __name__ == '__main__':
    test_controlled_kernel()
    # test_cclight()


    # LOG_NOTHING,
    # LOG_CRITICAL,
    # LOG_ERROR,
    # LOG_WARNING,
    # LOG_INFO,
    # LOG_DEBUG
