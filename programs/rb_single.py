import random
from openql import Kernel, Program

def build_rb(num_cliffords, kernel):
    inv_clifford_lut_gs = [0, 2, 1, 3, 8, 10, 6, 11, 4, 9, 5, 7, 12, 16, 23, 21, 13, 17, 18, 19, 20, 15, 22, 14]

    cl = random.sample(range(0, len(inv_clifford_lut_gs) ), int(num_cliffords/2) )
    inv_cl = cl[::-1]
    cliffords = inv_cl + cl

    kernel.prepz(0)
    for c in cliffords:
        kernel.clifford(c, 0);
    kernel.measure(0)


kernel = Kernel("rbKernel")
num_cliffords = 16
build_rb(num_cliffords, kernel)

sweep_points = [2]
num_circuits = 1
nqubits = 1
p = Program("rbProgram", nqubits)
p.set_sweep_points(sweep_points, num_circuits)
p.add_kernel(kernel)
p.compile()
p.schedule()

qasm = p.qasm()
microcode = p.microcode()

print(qasm)
print(microcode)
