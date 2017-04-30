from openql import Kernel, Program

k = Kernel()
k.prepz(0);
k.prepz(1);
k.x(0);
k.y(0);
k.cnot(0,1);
k.toffoli(0,1,2);
k.measure(2);

p = Program()
p.Add(k)
p.Compile()
p.Schedule()
