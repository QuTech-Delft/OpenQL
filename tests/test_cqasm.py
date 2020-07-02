import os
import filecmp
import unittest
from openql import openql as ql
from test_QASM_assembler_present import assemble

rootDir = os.path.dirname(os.path.realpath(__file__))

curdir = os.path.dirname(__file__)
config_fn = os.path.join(curdir, 'hardware_config_qx.json')
platf = ql.Platform("starmon", config_fn)

output_dir = os.path.join(curdir, 'test_output')

def file_compare(fn1, fn2):
    isSame = False
    with open(fn1, 'r') as f1:
        with open(fn2, 'r') as f2:
            a = f1.read()
            b = f2.read()
            f1.close()
            f2.close()
            isSame = (a==b)
    return isSame

class Test_cqasm(unittest.TestCase):

    @classmethod
    def setUpClass(self):
        ql.set_option('output_dir', output_dir)
        ql.set_option('optimize', 'no')
        ql.set_option('scheduler', 'ASAP')
        ql.set_option('log_level', 'LOG_INFO')
        ql.set_option('write_qasm_files', 'yes')


    def test_cqasm_default_gates(self):
        self.setUpClass() 
        ql.set_option('use_default_gates', 'yes')

        nqubits = 4

        k = ql.Kernel("aKernel", platf, nqubits)
        k.gate('prep_z', [0])
        k.gate('identity', [0])
        k.gate('hadamard', [0])
        k.gate('x', [0])
        k.gate('y', [0])
        k.gate('z', [0])
        k.gate('rx90', [0])
        k.gate('ry90', [0])
        k.gate('mrx90', [0])
        k.gate('mry90', [0])
        k.gate('s', [0])
        k.gate('sdag', [0])
        k.gate('t', [0])
        k.gate('tdag', [0])
        k.gate('rx', [0], 0, 0.15)
        k.gate('ry', [0], 0, 0.15)
        k.gate('rz', [0], 0, 0.15)
        k.gate('cnot', [0, 1])
        k.gate('cz', [0, 1])
        k.gate('toffoli', [0, 1, 2])
        k.gate('swap', [0, 1])
        k.gate('measure', [0])
        k.gate('measure', [1])

        sweep_points = [2]

        p = ql.Program('test_cqasm_default_gates', platf, nqubits)
        p.set_sweep_points(sweep_points)
        p.add_kernel(k)
        p.compile()

        qasm_files = []
        qasm_files.append(os.path.join(output_dir, p.name+'_initialqasmwriter_out.qasm'))
        qasm_files.append(os.path.join(output_dir, p.name+'_scheduledqasmwriter_out.qasm'))

        for qasm_file in qasm_files:
            # print('assembling: {}'.format(qasm_file))
            assemble(qasm_file)

    # @unittest.skip
    def test_cqasm_custom_gates(self):
        self.setUpClass() 
        ql.set_option('use_default_gates', 'no')

        nqubits = 4

        k = ql.Kernel("aKernel", platf, nqubits)
        k.gate('prep_z', [0])
        k.gate('i', [0])
        k.gate('h', [0])
        k.gate('x', [0])
        k.gate('y', [0])
        k.gate('z', [0])
        k.gate('x90', [0])
        k.gate('y90', [0])
        k.gate('mx90', [0])
        k.gate('my90', [0])
        k.gate('s', [0])
        k.gate('sdag', [0])
        k.gate('t', [0])
        k.gate('tdag', [0])
        k.gate('rx', [0], 0, 0.15)
        k.gate('ry', [0], 0, 0.15)
        k.gate('rz', [0], 0, 0.15)
        k.gate('cnot', [0, 1])
        k.gate('cz', [0, 1])
        k.gate('toffoli', [0, 1, 2])
        k.gate('swap', [0, 1])
        k.gate('measure', [0])
        k.gate('measure', [1])

        sweep_points = [2]

        p = ql.Program('test_cqasm_custom_gates', platf, nqubits)
        p.set_sweep_points(sweep_points)
        p.add_kernel(k)
        p.compile()

        qasm_files = []
        qasm_files.append(os.path.join(output_dir, p.name+'_initialqasmwriter_out.qasm'))
        qasm_files.append(os.path.join(output_dir, p.name+'_scheduledqasmwriter_out.qasm'))

        for qasm_file in qasm_files:
            print('assembling: {}'.format(qasm_file))
            assemble(qasm_file)

if __name__ == '__main__':
    unittest.main()
