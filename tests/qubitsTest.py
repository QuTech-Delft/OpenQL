from openql import Kernel, Program

p = Program("aProgram", 2)
k = Kernel("aKernel")

# populate kernel
for i in range(5):
    k.prepz(i)

for i in range(3):
    k.hadamard(i)

for i in range(3):
    for j in range(3, 5):
        k.cnot(i, j)

p.add_kernel(k)  # add kernel to program
p.compile()     # compile program

print(p.qasm())
print(p.microcode())
