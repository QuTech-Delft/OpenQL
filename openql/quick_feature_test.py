import os
import openql as ql
import numpy as np

curdir = os.path.dirname(__file__)
output_dir = os.path.join(curdir, 'test_output')
ql.set_output_dir(output_dir)

def test_cclight():
    config_fn = os.path.join(curdir, '../tests/hardware_config_cc_light.json')
    platform  = ql.Platform('seven_qubits_chip', config_fn)
    sweep_points = [1,2]
    num_circuits = 1
    num_qubits = 7
    p = ql.Program('aProgram', num_qubits, platform)
    p.set_sweep_points(sweep_points, num_circuits)

    k = ql.Kernel('aKernel', platform)

    k.prepz(0)
    k.prepz(1)
    k.prepz(2)
    k.prepz(3)
    k.hadamard(0)
    k.hadamard(1)
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
    p.compile(optimize=False, verbose=True)

def test_none():
    config_fn = os.path.join(curdir, '../tests/test_config_default.json')
    platform  = ql.Platform('seven_qubits_chip', config_fn)
    sweep_points = [1,2]
    num_circuits = 1
    num_qubits = 7
    p = ql.Program('aProgram', num_qubits, platform)
    p.set_sweep_points(sweep_points, num_circuits)

    k = ql.Kernel('aKernel', platform)

    k.prepz(0)
    k.prepz(1)
    # k.gate("rot_90",0)
    # k.gate("rot2_90",0)
    k.gate("rot3_90",1)
    # k.gate("cust_cnot", 0, 1 ) # ok
    k.gate("cust_cnot", 2, 3 )

    # k.gate("cnot", 2, 3 ) #???
    # k.gate("cnot", [2, 3] ) #???
    k.measure(0)
    k.measure(1)

    # add the kernel to the program
    p.add_kernel(k)

    # compile the program
    p.compile(optimize=False, verbose=True)
  
if __name__ == '__main__':
    test_none()


# composite gate names should be different than the other gates
#   composite gate name should start with "%"
# I think it is possible to keep them even same by
# saving list of gate names on the right side used in decomposition
# at the time of loading hardware configuration file
# and add them later only when actual gate is added to kernel.

# "%rot_90 0" : ["ry90 0"],
# "%rot2_90 0" : ["%rot_90 0", "ry90 0"],
# "%rot3_90 0" : ["%rot2_90 0", "ry90 0"],

# "%cnot 0,1" : ["ry90 1","cz 0,1","ry90 1"]
