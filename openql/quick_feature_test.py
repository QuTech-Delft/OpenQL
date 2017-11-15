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
    p.compile(False, "ALAP", True)

def test_none():
    config_fn = os.path.join(curdir, '../tests/test_cfg_none.json')
    platform  = ql.Platform('platform_none', config_fn)
    num_qubits = 5
    p = ql.Program('aProgram', num_qubits, platform)
    # sweep_points = [1,2]
    # p.set_sweep_points(sweep_points, len(sweep_points))

    k = ql.Kernel('aKernel', platform)

    k.gate("x",0);
    k.gate("x",0);
    k.gate("cz", [0, 1])
    k.gate("cz", [2, 3])

    p.add_kernel(k)
    p.compile(False, "ASAP", True)


def test_bug():
    config_fn = os.path.join(curdir, '../tests/hardware_config_cc_light.json')
    platform  = ql.Platform('seven_qubits_chip', config_fn)
    num_qubits = 7
    p = ql.Program('aProgram', num_qubits, platform)
    sweep_points = [1,2]
    p.set_sweep_points(sweep_points, len(sweep_points))

    k = ql.Kernel('aKernel', platform)

    for i in range(6):
        k.prepz(i)

    k.gate('cz', 2, 0)
    k.gate('cz', 3, 5)
    k.gate('cz', 1, 4)

    p.add_kernel(k)
    # optimize=false, scheduler="ALAP", verbose=false
    p.compile(optimize=False, scheduler='ASAP', verbose=True)


if __name__ == '__main__':
    test_bug()
    # test_cclight()
    # test_none()
