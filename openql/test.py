import openql as ql

ql.set_instruction_map_file("instructions.map")
ql.init()

k = ql.Kernel("aKernel")
k.prepz(0)
k.prepz(1)
k.identity(0)
k.y(0)
# k.cnot(0, 1)
# k.toffoli(0, 1, 2)
# k.rx90(0)
# k.clifford(2, 0)
k.measure(0)

sweep_points = [2]
num_circuits = 1
nqubits = 3

p = ql.Program("rbProgram", nqubits)
p.set_sweep_points(sweep_points, num_circuits)
p.add_kernel(k)
p.compile(False, True)
p.schedule()
