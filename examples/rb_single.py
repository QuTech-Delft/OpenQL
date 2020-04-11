import os
import random
from openql import openql as ql


rootDir = os.path.dirname(os.path.realpath(__file__))

curdir = os.path.dirname(__file__)
output_dir = os.path.join(curdir, 'test_output')

ql.set_option('output_dir', output_dir)
ql.set_option('write_qasm_files', 'yes')
ql.set_option('optimize', 'no')
ql.set_option('scheduler', 'ASAP')
ql.set_option('log_level', 'LOG_INFO')


def build_rb(qubit, num_cliffords, kernel):
    inv_clifford_lut_gs = [0, 2, 1, 3, 8, 10, 6, 11, 4, 9, 5, 7, 12, 16, 
    					   23, 21, 13, 17, 18, 19, 20, 15, 22, 14]

    cl = random.sample( range(0, len(inv_clifford_lut_gs) ),
    					int(num_cliffords/2) )
    inv_cl = cl[::-1]
    cliffords = inv_cl + cl

    kernel.prepz(qubit)
    for c in cliffords:
        kernel.clifford(c, qubit);
    kernel.measure(qubit)


config_fn  = os.path.join(curdir, '../tests/hardware_config_qx.json')
platform   = ql.Platform('platform_none', config_fn)
num_qubits = 1

# create a grover program
p = ql.Program('rb117', platform, num_qubits)

kernel = ql.Kernel("rb_kernel", platform, num_qubits)

num_cliffords = 32
for q in range(num_qubits):
    build_rb(q, num_cliffords, kernel)

p.add_kernel(kernel)
p.compile()

# qasm = p.qasm()
# print(qasm)
