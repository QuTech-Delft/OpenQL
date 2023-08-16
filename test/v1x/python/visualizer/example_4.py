import openql as ql
import os

from config import json_visualizer_dir, output_dir


ql.set_option('output_dir', output_dir)
ql.set_option('optimize', 'no')
ql.set_option('scheduler', 'ASAP')
ql.set_option('log_level', 'LOG_INFO')
ql.set_option('unique_output', 'yes')
ql.set_option('write_qasm_files', 'no')
ql.set_option('write_report_files', 'no')

platformCustomGates = ql.Platform('starmon',  os.path.join(json_visualizer_dir, 'config_cc_light.json'))
num_qubits = 6
p = ql.Program("testProgram1", platformCustomGates, num_qubits, 0)
k = ql.Kernel("aKernel1", platformCustomGates, num_qubits, 0)
for i in range(num_qubits):
    k.gate('prepz', [i])
k.gate('x', [0])
k.gate('cnot', [0, 3])
k.gate('x', [0])
k.gate('h', [2])
k.gate('x', [4])
k.gate('h', [5])
k.gate('measure', [5])
k.gate('cz', [3, 1])
k.gate('cz', [2, 0])
k.gate('measure', [0])
k.gate('measure', [1])
k.gate('measure', [2])
k.gate('measure', [3])
p.add_kernel(k)

p.get_compiler().append_pass(
    'ana.visualize.Circuit',
    'visualize_circuit',
    {
        'output_prefix': output_dir + '/%N_circuit',
        'config':  os.path.join(json_visualizer_dir, 'config_example_4.json'),
        'waveform_mapping':  os.path.join(json_visualizer_dir, 'waveform_mapping.json'),
        'interactive': 'yes'
    }
)

p.compile()
