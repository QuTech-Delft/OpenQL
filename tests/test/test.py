from openql import openql as ql

ql.set_instruction_map_file("instructions.map")
ql.init()

k = ql.Kernel("aKernel")
k.prepz(0)
k.prepz(1)
k.cnot(0,1)
k.cnot(1,2)
k.cnot(1,2)
k.identity(0)
k.x(0)
k.y(1)
k.measure(0)

sweep_points = [2]
num_circuits = 1
nqubits = 3

p = ql.Program("rbProgram", nqubits)
p.set_sweep_points(sweep_points, num_circuits)
p.add_kernel(k)
p.compile(False, True)
p.schedule()
p.print_interaction_matrix()
p.write_interaction_matrix()

