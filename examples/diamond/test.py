import openql as ql

# turn off automatic scheduling by OpenQL to preserve instruction order
ql.set_option('prescheduler', 'no') 

# Specify the platform
platform = ql.Platform("diamond_test", "diamond")

# Put a cQASM reader before the diamond pass, so a cQASM file is read instead of the python code
#platform.get_compiler().prefix_pass('io.cqasm.Read', '', {'cqasm_file': 'cqasm_measure.cq', 'gateset_file': 'gateset.json'})

nqubits = 3
p = ql.Program("testProgram", platform, nqubits)
k = ql.Kernel("testKernel", platform, nqubits)


# Below follow examples for all gates that are supported, arranged by category:

# Initialization
k.gate('prep_z', [0])
k.gate('prep_x', [0])
k.gate('prep_y', [0])
k.gate('initialize', [0])

# Measurement
k.gate('measure', [0])
k.gate('measure_z', [0])
k.gate('measure_x', [0])
k.gate('measure_y', [0])

# single qubit gates
k.x(0)
k.y(0)
k.z(0)
k.s(0)
k.t(0)

# two qubit gates 
k.gate('cnot', [0, 1]) # only between color center - nuclear spin qubit
k.gate('cz', [0, 1])

# three qubit gate
k.gate('toffoli', [0, 1, 2])

# calibration
k.gate('cal_measure', [0])
k.gate('cal_pi', [0])
k.gate('cal_halfpi', [0])
k.gate('decouple', [0])

# custom rotations
k.gate('rx', [0], 0, 1.57)
k.gate('ry', [0], 0, 1.57)
k.gate('crk', [0, 1], 0, 1)
k.gate('cr', [0, 1], 0, 3.14)

# diamond protocols/sequences
k.diamond_crc(0,30,5)
k.diamond_rabi_check(0, 100, 2, 3) # qubit, duration, measurement, t_max
k.diamond_excite_mw(1, 100, 200, 0, 60, 0) # envelope, duration, frequency, phase, amplitude, qubit
k.diamond_qentangle(0,15) # qubit, nuclear qubit
k.gate('nventangle', [0, 1])
k.diamond_memswap(0,1) # qubit, nuclear qubit
k.diamond_sweep_bias(0, 10, 0, 0, 10, 100, 0) #qubit, value, dacreg, start, step, max, memaddress

# timing
k.gate('wait', [], 200)
k.gate('qnop', [0]) # qubit, mandatory from openQL

# calculate bias
k.gate('calculate_current', [0])

# calculate voltage
k.gate('calculate_voltage', [0])

# magnetic bias check
k.diamond_sweep_bias(0, 10, 0, 0, 10, 100, 0)
k.gate('calculate_voltage', [0])

p.add_kernel(k)
p.compile()


