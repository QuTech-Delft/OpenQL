import os
import openql as ql
import numpy as np

curdir = os.path.dirname(__file__)
output_dir = os.path.join(curdir, 'test_output')

ql.set_option('output_dir', output_dir)
ql.set_option('optimize', 'no')
ql.set_option('scheduler', 'ALAP')
ql.set_option('log_level', 'LOG_DEBUG')
ql.set_option('decompose_toffoli', 'no')
ql.set_option('use_default_gates', 'yes')

def test_cclight():
    config_fn = os.path.join(curdir, '../tests/hardware_config_cc_light.json')
    platform  = ql.Platform('seven_qubits_chip', config_fn)
    num_qubits = 7
    p = ql.Program('test_cclight', platform, num_qubits)
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
    p = ql.Program('test_none', platform, num_qubits)

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
    p = ql.Program('test_qx', platform, num_qubits)

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
    # config_fn = os.path.join(curdir, '../tests/test_cfg_none.json')
    config_fn = os.path.join(curdir, '../tests/hardware_config_cc_light.json')
    platform  = ql.Platform('platform_none', config_fn)
    num_qubits = 5
    num_cregs = 5

    p = ql.Program('test_hybrid', platform, num_qubits, num_cregs)
    sweep_points = [1,2]
    p.set_sweep_points(sweep_points, len(sweep_points))
    
    sp1 = ql.Program('subprogram1', platform, num_qubits, num_cregs)
    sp2 = ql.Program('subprogram2', platform, num_qubits, num_cregs)

    k1 = ql.Kernel('aKernel1', platform, num_qubits, num_cregs)
    k2 = ql.Kernel('aKernel2', platform, num_qubits, num_cregs)

    k1.gate('x', [0])
    k1.gate('cz', [0, 2])

    k2.gate('y', [0])
    k2.gate('cz', [0, 2])

    p.add_kernel(k1)
    p.add_kernel(k2)
    p.compile()



    # k1.gate('cnot', [0, 2])
    # k1.classical('inc', [2])
    # k1.gate('measure', [0], [2])
    # k1.gate('measure', [1], [2])
    # k1.gate('measure', [2], [2])



    # k2 = ql.Kernel('aKernel2', platform, num_qubits, num_cregs)
    # k2.gate('cz', [0, 1])
    # k2.gate('cz', [2, 3])

    # sp.add_kernel(k)
    # sp.add_kernel(k)
    # p.add_program(sp)


    # classical_reg = 0

    # no control flow
    # p.add_kernel(k)

    # simple if
    # p.add_if(k1, classical_reg)

    # nested if
    # sp1.add_kernel(k1)
    # sp1.add_kernel(k2)    
    # p.add_if(sp1, classical_reg)

    # simple if/else
    # p.add_if_else(k1, k2, classical_reg)

    # nested if/else
    # sp1.add_kernel(k1)
    # sp2.add_kernel(k2)
    # p.add_if_else(sp1, sp2, classical_reg)

    # simple while
    # p.add_while(k1, classical_reg)

    # nested while
    # sp1.add_while(k1, classical_reg)
    # sp2.add_while(sp1, classical_reg)
    # p.add_program(sp2)

    # nesting while inside if
    # sp1.add_while(k1, classical_reg)
    # sp2.add_if(sp1, classical_reg)
    # p.add_program(sp2)

    # simple for
    # p.add_for(k1, 10)

    # nested for
    # sp1.add_for(k1, 10)
    # sp2.add_for(sp1, 20)
    # p.add_program(sp2)

    # # classical operations
    # k1.classical('set', [0], 10)
    # k1.classical('not', [0,1])
    # k1.classical('add', [0,1,2])
    # k1.classical('inc', [1])
    # # k1.classical('set', [5], 10) # out of range operand
    # # k1.classical('set', [7], 10) # out of range operand
    # p.add_kernel(k1)

    p.compile()


if __name__ == '__main__':
    test_loop()


# LOG_NOTHING,
# LOG_CRITICAL,
# LOG_ERROR,
# LOG_WARNING,
# LOG_INFO,
# LOG_DEBUG
