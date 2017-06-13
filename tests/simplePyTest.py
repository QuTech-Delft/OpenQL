from openql import Kernel, Program

k = Kernel("kernel1")
k.prepz(0)
k.prepz(1)
k.x(0)
k.y(0)
k.cnot(0, 1)
k.toffoli(0, 1, 2)
k.rx90(0)
k.clifford(2, 0)
k.measure(2)

sweep_points = [2]
num_circuits = 1
nqubits = 3
p = Program("rbProgram", nqubits)
p.set_sweep_points(sweep_points, num_circuits)
p.add_kernel(k)
p.compile()
p.schedule()
