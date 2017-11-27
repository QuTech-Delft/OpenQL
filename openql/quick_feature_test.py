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
    # config_fn = os.path.join(curdir, '../tests/hardware_config_cc_light.json')
    config_fn = os.path.join(curdir, '../tests/hardware_config_cc_light_bak.json')
    # config_fn = os.path.join(curdir, '../tests/test_cfg_cc_light_buffers_latencies.json')

    platform  = ql.Platform('seven_qubits_chip', config_fn)
    num_qubits = 7
    p = ql.Program('aProgram', num_qubits, platform)
    sweep_points = [1,2]
    p.set_sweep_points(sweep_points, len(sweep_points))

    k = ql.Kernel('aKernel', platform)

    qubit = 1
    k.prepz(qubit)
    k.gate('x', qubit)
    k.measure(qubit)

    k.prepz(qubit)
    k.gate('x', qubit)
    k.measure(qubit)

    p.add_kernel(k)
    p.compile(optimize=False, scheduler='ASAP', verbose=False)

def test_gate_decomp():
    # config_fn = os.path.join(curdir, '../tests/hardware_config_cc_light.json')
    config_fn = os.path.join(curdir, '../tests/test_cfg_cbox.json')
    platform = ql.Platform("starmon", config_fn)
    num_qubits = 2
    p = ql.Program('aProgram', num_qubits, platform)
    sweep_points = [1,2]
    p.set_sweep_points(sweep_points, len(sweep_points))

    k = ql.Kernel('aKernel', platform)
    # print(k.get_custom_instructions())

    k.gate('prepz', 1)
    k.gate('x', 1) # will use custom gate
    k.gate('x1', 0) # will use parameterized decomposition
    k.gate('x2', 1) # will use specialized decomposition
    k.gate('measure', 1)

    p.add_kernel(k)
    p.compile(optimize=False, scheduler='ASAP', verbose=False)


if __name__ == '__main__':
    test_gate_decomp()

      # "x1 %0" : ["rx180 %0", "ry180 %0"],
      # "x2 q1" : ["rx180 q1", "i q1", "ry180 q1"]

   # "gate_decomposition": {
   #    "x q0" : ["rx180 q0"],
   #    "y q0" : ["ry180 q0"],
   #    "z q0" : ["ry180 q0","rx180 q0"],
   #    "h q0" : ["ry90 q0"],
   #    "cnot q0,q1" : ["ry90 q1","cz q0,q1","ry90 q1"]
   # },

      # "ry180 q1" : {
      #    "duration": 40,
      #    "latency": 20,
      #    "qubits": ["q1"],
      #    "matrix" : [ [0.0,0.0], [1.0,0.0],
      #            [1.0,0.0], [0.0,0.0] ], 
      #    "disable_optimization": false,
      #    "type" : "mw",
      #    "qumis_instr": "pulse",
      #    "qumis_instr_kw": {
      #       "codeword": 3, 
      #       "awg_nr": 0
      #    }
      # },
