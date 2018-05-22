import os
import openql as ql
import numpy as np

curdir = os.path.dirname(__file__)
output_dir = os.path.join(curdir, 'test_output')

ql.set_option('output_dir', output_dir)
ql.set_option('optimize', 'no')
ql.set_option('scheduler', 'ASAP')
ql.set_option('log_level', 'LOG_DEBUG')
ql.set_option('decompose_toffoli', 'no')

def test_cclight():
    config_fn = os.path.join(curdir, '../tests/hardware_config_cc_light.json')
    platform  = ql.Platform('seven_qubits_chip', config_fn)
    num_qubits = 7
    p = ql.Program('test_cclight', num_qubits, platform)
    sweep_points = [1,2]
    p.set_sweep_points(sweep_points, len(sweep_points))

    k = ql.Kernel('aKernel', platform)

    for i in range(4):
        k.gate('prepz', [i])

    k.gate('x', [2])
    k.gate('cnot', [2, 0])
    k.gate('cnot', [1, 4])

    for i in range(4):
        k.gate('measure', [i])

    p.add_kernel(k)
    p.compile()

def test_none():
    config_fn = os.path.join(curdir, '../tests/test_cfg_none.json')
    platform  = ql.Platform('platform_none', config_fn)
    num_qubits = 5
    p = ql.Program('test_none', num_qubits, platform)

    k = ql.Kernel('aKernel', platform)

    k.gate("x", [0])
    k.gate("x", [0])
    k.gate("cz", [0, 1])
    k.gate("cz", [2, 3])
    k.gate("display", [])

    p.add_kernel(k)
    p.compile()


def test_qx():
    config_fn = os.path.join(curdir, '../tests/hardware_config_qx.json')
    platform  = ql.Platform('platform_qx', config_fn)
    num_qubits = 5
    p = ql.Program('test_qx', num_qubits, platform)

    k = ql.Kernel('aKernel', platform)

    k.gate("x", [0])
    k.gate("x",[0])
    k.gate("cz", [0, 1])
    k.gate("cz", [2, 3])
    k.gate("display")
    k.gate("display", [])
    k.display()

    p.add_kernel(k)
    p.compile()


def test_loop():
    config_fn = os.path.join(curdir, '../tests/test_cfg_none.json')
    platform  = ql.Platform('platform_none', config_fn)
    num_qubits = 5
    p = ql.Program('test_loop', num_qubits, platform)

    k = ql.Kernel('aKernel', platform)

    k.gate('x', [0])
    k.gate('x', [0])
    k.gate('cz', [0, 1])
    k.gate('cz', [2, 3])
    k.gate('measure', [0])

    classical_reg = 0

    # what if the following calls return label
    # these labels can then be utilized to create nested loops
    # this idea can also be extended to branch to even arbitrary places
    # in the code for instance by returning labels from k.gate() as well.

    # a kernel should have:
    # - prologue (can be used for if/for)
    # - Kernel   (used for all kernels)
    # - epilogue (can be used for while)

    p.add_kernel(k)
    p.add_if(k, classical_reg)
    p.add_if_else(k, k, classical_reg)
    p.add_while(k, classical_reg)

    p.compile()

if __name__ == '__main__':
    test_loop()


    # LOG_NOTHING,
    # LOG_CRITICAL,
    # LOG_ERROR,
    # LOG_WARNING,
    # LOG_INFO,
    # LOG_DEBUG

# TODO: check multiple kernels with same name
#       for now programer should provide unique name for each kernel

# TODO: check for empty kernel and classical code around it !