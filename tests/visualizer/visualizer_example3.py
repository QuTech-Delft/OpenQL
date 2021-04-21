from openql import openql as ql
import os

curdir = os.path.dirname(__file__)
output_dir = os.path.join(curdir, 'visualizer_example_output')

ql.set_option('output_dir', output_dir)
ql.set_option('optimize', 'no')
ql.set_option('scheduler', 'ASAP')
#ql.set_option('log_level', 'LOG_DEBUG')
ql.set_option('log_level', 'LOG_INFO')
ql.set_option('unique_output', 'yes')
ql.set_option('write_qasm_files', 'no')
ql.set_option('write_report_files', 'no')

platformCustomGates = ql.Platform('starmon', os.path.join(curdir, 'hardware_config_cc_light_visualizer2.json'))
nqubits = 4
p = ql.Program("testProgram1", platformCustomGates, nqubits, 0)
k = ql.Kernel("aKernel1", platformCustomGates, nqubits, 0)
k.gate('x', [0])
for i in range(nqubits):
    k.gate('prepz', [i])
k.gate('wait', [1], 40)
k.gate('wait', [2], 40)
k.gate('wait', [3], 40)
k.gate('x', [0])
k.gate('x', [0])
k.gate('x', [0])
k.gate('wait', [2], 40)
k.gate('h', [2])
k.gate('cz', [3, 1])
k.gate('cz', [2, 0])
k.gate('cz', [2, 0])
k.gate('wait', [3], 40)
k.gate('measure', [3])
k.gate('measure', [0])
k.gate('measure', [1])
k.gate('measure', [2])
#k.gate('measure', [3])
p.add_kernel(k)

#p.get_compiler().append_pass(
    #'ana.visualize.Interaction',
    #'visualize_interaction',
    #{
        #'output_prefix': output_dir + '/%N_interaction',
        #'config': os.path.join(curdir, "visualizer_config_example3.json"),
        #'interactive': 'yes'
    #}
#)

p.get_compiler().append_pass(
    'ana.visualize.Circuit',
    'visualize_circuit',
    {
        'output_prefix': output_dir + '/%N_circuit',
        'config': os.path.join(curdir, "visualizer_config_example3.json"),
        'waveform_mapping': os.path.join(curdir, "waveform_mapping.json"),
        'interactive': 'yes'
    }
)

p.compile()
