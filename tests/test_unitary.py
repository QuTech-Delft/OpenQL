import os
import unittest
from openql import openql as ql
from utils import file_compare

rootDir = os.path.dirname(os.path.realpath(__file__))
curdir = os.path.dirname(__file__)
output_dir = os.path.join(curdir, 'test_output')


class Test_conjugated_kernel(unittest.TestCase):

    @classmethod
    def setUpClass(self):
        ql.set_option('output_dir', output_dir)
        ql.set_option('optimize', 'no')
        ql.set_option('scheduler', 'ASAP')
        ql.set_option('log_level', 'LOG_WARNING')

    def test_unitary_basic(self):
        config_fn = os.path.join(curdir, 'test_cfg_none.json')
        platform = ql.Platform('platform_none', config_fn)
        num_qubits = 3
        p = ql.Program('test_unitary_pass', platform, num_qubits)
        k = ql.Kernel('akernel', platform, num_qubits)

        u = ql.Unitary('u1', [  complex(1.0, 0.0), complex(0.0, 0.0),
                                complex(0.0, 0.0), complex(0.0, 1.0)])
        u.decompose()

        k.gate("i", [0])
        k.gate("s", [0])
        k.gate(u, [2])

        p.add_kernel(k)
        p.compile()
	
    def test_unitary_undecomposed(self):
        config_fn = os.path.join(curdir, 'test_cfg_none.json')
        platform = ql.Platform('platform_none', config_fn)
        num_qubits = 3
        p = ql.Program('test_unitary_pass', platform, num_qubits)
        k = ql.Kernel('akernel', platform, num_qubits)

        u = ql.Unitary('u1', [  complex(1.0, 0.0), complex(0.0, 0.0),
                                complex(0.0, 0.0), complex(0.0, 1.0)])
        k.gate("s", [0])

        # adding un-decomposed u to kernel should raise error
        with self.assertRaises(Exception) as cm:
            k.gate(u, [2])

        self.assertEqual(str(cm.exception), 'Unitary \'u1\' not decomposed. Cannot be added to kernel!')

    def test_unitary_decompose_I(self):
        config_fn = os.path.join(curdir, 'test_cfg_none.json')
        platform = ql.Platform('platform_none', config_fn)
        num_qubits = 1
        p = ql.Program('test_unitary_I', platform, num_qubits)
        k = ql.Kernel('akernel', platform, num_qubits)

        u = ql.Unitary('u1', [  complex(1.0, 0.0), complex(0.0, 0.0),
                                complex(0.0, 0.0), complex(1.0, 0.0)])
        u.decompose()
        k.gate(u, [0])

        p.add_kernel(k)
        p.compile()
        
        gold_fn = rootDir + '/golden/test_unitary-decomp_1qubit_I.qasm'
        qasm_fn = os.path.join(output_dir, p.name+'.qasm')
        self.assertTrue( file_compare(qasm_fn, gold_fn) )

    def test_unitary_decompose_X(self):
        config_fn = os.path.join(curdir, 'test_cfg_none.json')
        platform = ql.Platform('platform_none', config_fn)
        num_qubits = 1
        p = ql.Program('test_unitary_X', platform, num_qubits)
        k = ql.Kernel('akernel', platform, num_qubits)

        u = ql.Unitary('u1', [  complex(0.0, 0.0), complex(1.0, 0.0),
                                complex(1.0, 0.0), complex(0.0, 0.0)])
        u.decompose()
        k.gate(u, [0])

        p.add_kernel(k)
        p.compile()
        
        gold_fn = rootDir + '/golden/test_unitary-decomp_1qubit_X.qasm'
        qasm_fn = os.path.join(output_dir, p.name+'.qasm')
        self.assertTrue( file_compare(qasm_fn, gold_fn) )

    def test_unitary_decompose_Y(self):
        config_fn = os.path.join(curdir, 'test_cfg_none.json')
        platform = ql.Platform('platform_none', config_fn)
        num_qubits = 1
        p = ql.Program('test_unitary_Y', platform, num_qubits)
        k = ql.Kernel('akernel', platform, num_qubits)

        u = ql.Unitary('u1', [  complex(0.0, 0.0), complex(0.0, -1.0),
                                complex(0.0, 1.0), complex(0.0, 0.0)])
        u.decompose()
        k.gate(u, [0])

        p.add_kernel(k)
        p.compile()
        
        gold_fn = rootDir + '/golden/test_unitary-decomp_1qubit_Y.qasm'
        qasm_fn = os.path.join(output_dir, p.name+'.qasm')
        self.assertTrue( file_compare(qasm_fn, gold_fn) )
 
    def test_unitary_decompose_Z(self):
        config_fn = os.path.join(curdir, 'test_cfg_none.json')
        platform = ql.Platform('platform_none', config_fn)
        num_qubits = 1
        p = ql.Program('test_unitary_Z', platform, num_qubits)
        k = ql.Kernel('akernel', platform, num_qubits)

        u = ql.Unitary('u1', [  complex(1.0, 0.0), complex(0.0, 0.0),
                                complex(0.0, 0.0), complex(-1.0, 0.0)])
        u.decompose()
        k.gate(u, [0])

        p.add_kernel(k)
        p.compile()
        
        gold_fn = rootDir + '/golden/test_unitary-decomp_1qubit_Z.qasm'
        qasm_fn = os.path.join(output_dir, p.name+'.qasm')
        self.assertTrue( file_compare(qasm_fn, gold_fn) )   

    def test_unitary_decompose_IYZ(self):
        config_fn = os.path.join(curdir, 'test_cfg_none.json')
        platform = ql.Platform('platform_none', config_fn)
        num_qubits = 1
        p = ql.Program('test_unitary_IYZ', platform, num_qubits)
        k = ql.Kernel('akernel', platform, num_qubits)

        u = ql.Unitary('X', [  complex(0.0, 0.0), complex(1.0, 0.0),
                                complex(1.0, 0.0), complex(0.0, 0.0)])
        u.decompose()
        k.gate(u, [0])
        u = ql.Unitary('y', [  complex(0.0, 0.0), complex(0.0, -1.0),
                                complex(0.0, 1.0), complex(0.0, 0.0)])
        u.decompose()
        k.gate(u, [0])
        u = ql.Unitary('Z', [  complex(1.0, 0.0), complex(0.0, 0.0),
                                complex(0.0, 0.0), complex(-1.0, 0.0)])
        u.decompose()
        k.gate(u, [0])

        p.add_kernel(k)
        p.compile()
        
        gold_fn = rootDir + '/golden/test_unitary-decomp_1qubit_IYZ.qasm'
        qasm_fn = os.path.join(output_dir, p.name+'.qasm')
        self.assertTrue( file_compare(qasm_fn, gold_fn) )   

    def test_unitary_decompose_nonunitary(self):
        config_fn = os.path.join(curdir, 'test_cfg_none.json')
        platform = ql.Platform('platform_none', config_fn)
        num_qubits = 1
        p = ql.Program('test_unitary_nonunitary', platform, num_qubits)
        k = ql.Kernel('akernel', platform, num_qubits)

        u = ql.Unitary('WRONG', [  complex(0.0, 0.0), complex(0.0, 0.0),
                                complex(0.0, 0.0), complex(0.0, 0.0)])
        u.decompose()
        k.gate(u, [0])

        p.add_kernel(k)
        with self.assertRaises(Exception) as cm:
            p.compile()

        self.assertEqual(str(cm.exception), 'Error: trying to decompose a non-unitary gate!')
  
    
    def test_unitary_decompose_nonunitary(self):
        config_fn = os.path.join(curdir, 'test_cfg_none.json')
        platform = ql.Platform('platform_none', config_fn)
        num_qubits = 1
        p = ql.Program('test_unitary_wrongtype', platform, num_qubits)
        k = ql.Kernel('akernel', platform, num_qubits)

        u = ql.Unitary('WRONG', [  1,0,
                                0, 1j])
        u.decompose()
        k.gate(u, [0])

        p.add_kernel(k)
        with self.assertRaises(Exception) as cm:
            p.compile()

        self.assertEqual(str(cm.exception), 'Error: wrong type!')

    def test_unitary_decompose_2qubit_CNOT(self):
        config_fn = os.path.join(curdir, 'test_cfg_none.json')
        platform = ql.Platform('platform_none', config_fn)
        num_qubits = 2
        p = ql.Program('test_unitary_2qubitCNOT', platform, num_qubits)
        k = ql.Kernel('akernel', platform, num_qubits)

        u = ql.Unitary('big_unitary', [  complex(1.0, 0.0), complex(0.0, 0.0), complex(0.0, 0.0), complex(0.0, 0.0),
                               complex(0.0, 0.0), complex(1.0, 0.0), complex(0.0, 0.0), complex(0.0, 0.0),
                               complex(0.0, 0.0), complex(0.0, 0.0), complex(0.0, 0.0), complex(1.0, 0.0),
                               complex(0.0, 0.0), complex(0.0, 0.0), complex(1.0, 0.0), complex(0.0, 0.0)])
        u.decompose()
        k.gate(u, [0, 1])
        p.add_kernel(k)
        p.compile()
        
        gold_fn = rootDir + '/golden/test_unitary-decomp_1qubit_IYZ.qasm'
        qasm_fn = os.path.join(output_dir, p.name+'.qasm')
        self.assertTrue( file_compare(qasm_fn, gold_fn) )   


if __name__ == '__main__':
    unittest.main()
