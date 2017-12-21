import os
import openql as ql
import numpy as np

curdir = os.path.dirname(__file__)
output_dir = os.path.join(curdir, 'test_output')
ql.set_output_dir(output_dir)

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

    # LOG_NOTHING,
    # LOG_CRITICAL,
    # LOG_ERROR,
    # LOG_WARNING,
    # LOG_INFO,
    # LOG_DEBUG

    p.compile(optimize=False, scheduler='ASAP', log_level='LOG_INFO')

def test_none():
    config_fn = os.path.join(curdir, '../tests/test_cfg_none.json')
    platform  = ql.Platform('platform_none', config_fn)
    num_qubits = 5
    p = ql.Program('aProgram', num_qubits, platform)

    k = ql.Kernel('aKernel', platform)

    k.gate("x",0);
    k.gate("x",0);
    k.gate("cz", [0, 1])
    k.gate("cz", [2, 3])

    p.add_kernel(k)
    p.compile(optimize=False, scheduler='ASAP', log_level='LOG_CRITICAL')


def test_gate_decomp():
    # config_fn = os.path.join(curdir, '../tests/hardware_config_cc_light.json')
    config_fn = os.path.join(curdir, '../tests/test_cfg_cbox.json')
    platform = ql.Platform("starmon", config_fn)
    num_qubits = 2
    p = ql.Program('aProgram', num_qubits, platform)
    sweep_points = [1,2]
    p.set_sweep_points(sweep_points, len(sweep_points))

    k = ql.Kernel('aKernel', platform)
    # print(k.get_custom_instructions())

    k.gate('prepz', 1)
    k.gate('x', 1)  # will use custom gate
    k.gate('x1', 0) # will use parameterized decomposition
    k.gate('x2', 1) # will use specialized decomposition
    k.gate('measure', 1)

    p.add_kernel(k)
    p.compile(optimize=False, scheduler='ASAP', log_level='LOG_CRITICAL')


if __name__ == '__main__':
    test_cclight()