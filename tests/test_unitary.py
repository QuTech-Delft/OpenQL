import os
import unittest
from openql import openql as ql
from utils import file_compare
from qxelarator import qxelarator
import re
import numpy as np


curdir = os.path.dirname(__file__)
config_fn = os.path.join(curdir, 'test_cfg_none_simple.json')
platform = ql.Platform('platform_none', config_fn)
rootDir = os.path.dirname(os.path.realpath(__file__))
output_dir = os.path.join(curdir, 'test_output')

qx = qxelarator.QX()
c0 = ""

def helper_regex(measurementstring):
    regex = re.findall('[0-9.e\-]+',measurementstring)
    i = 0
    array = []
    while i < len(regex):
        array.append(float(regex[i])**2+float(regex[i+1])**2)
        i +=3 # so we skip every third occurence, which is the bit string representing the qubit combi
    return array

def helper_prob(qubitstate):
    return qubitstate.real**2+qubitstate.imag**2

class Test_conjugated_kernel(unittest.TestCase):

    @classmethod
    def setUpClass(self):
        ql.set_option('output_dir', output_dir)
        ql.set_option('optimize', 'no')
        ql.set_option('scheduler', 'ASAP')
        ql.set_option('log_level', 'LOG_NOTHING')

    def test_unitary_basic(self):
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

    def test_unitary_called_hadamard(self):
        num_qubits = 3
        p = ql.Program('test_unitary_called_hadamard', platform, num_qubits)
        k = ql.Kernel('akernel', platform, num_qubits)

        u = ql.Unitary('hadamard', [  complex(1.0, 0.0), complex(0.0, 0.0),
                                complex(0.0, 0.0), complex(0.0, 1.0)])
        u.decompose()

        k.gate(u, [2])

        p.add_kernel(k)
        p.compile()
	
    def test_unitary_undecomposed(self):
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

    def test_unitary_wrongnumberofqubits(self):
        num_qubits = 3
        p = ql.Program('test_unitary_pass', platform, num_qubits)
        k = ql.Kernel('akernel', platform, num_qubits)

        u = ql.Unitary('u1', [  complex(1.0, 0.0), complex(0.0, 0.0),
                                complex(0.0, 0.0), complex(0.0, 1.0)])
        k.gate("s", [0])

        # adding un-decomposed u to kernel should raise error
        with self.assertRaises(Exception) as cm:
            k.gate(u, [1,2])

        self.assertEqual(str(cm.exception), 'Unitary \'u1\' has been applied to the wrong number of qubits. Cannot be added to kernel! 2 and not 1.000000')
    
    def test_unitary_wrongnumberofqubits_toofew(self):
        num_qubits = 3
        p = ql.Program('test_unitary_pass', platform, num_qubits)
        k = ql.Kernel('akernel', platform, num_qubits)


        u = ql.Unitary('u1', [-0.43874989-0.10659111j, -0.47325212+0.12917344j, -0.58227163+0.20750072j, -0.29075334+0.29807585j,  
                    0.30168601-0.22307459j,  0.32626   +0.4534935j , -0.20523265-0.42403593j, -0.01012565+0.5701683j , 
                    -0.40954341-0.49946371j,  0.28560698-0.06740801j,  0.52146754+0.1833513j , -0.37248653+0.22891636j,  
                    0.03113162-0.48703302j, -0.57180014+0.18486244j,  0.2943625 -0.06148912j,  0.55533888+0.04322811j])
        # adding un-decomposed u to kernel should raise error
        with self.assertRaises(Exception) as cm:
            k.gate(u, [0])

        self.assertEqual(str(cm.exception), 'Unitary \'u1\' has been applied to the wrong number of qubits. Cannot be added to kernel! 1 and not 2.000000')

    def test_unitary_decompose_I(self):
        num_qubits = 1
        p = ql.Program('test_unitary_I', platform, num_qubits)
        k = ql.Kernel('akernel', platform, num_qubits)

        matrix = [  complex(1.0, 0.0), complex(0.0, 0.0),  complex(0.0, 0.0), complex(1.0, 0.0) ]
        u = ql.Unitary('u1', matrix)
        u.decompose()
        k.hadamard(0)
        k.gate(u, [0])

        p.add_kernel(k)
        p.compile()

        qx.set(os.path.join(output_dir, p.name+'.qasm'))
        qx.execute()
        c0 = qx.get_state()
        self.assertAlmostEqual(0.5*(helper_prob(matrix[0])+helper_prob(matrix[1])), helper_regex(c0)[0], 5)
        self.assertAlmostEqual(0.5*(helper_prob(matrix[2])+helper_prob(matrix[3])), helper_regex(c0)[0], 5)

    def test_unitary_decompose_X(self):
        num_qubits = 1
        p = ql.Program('test_unitary_X', platform, num_qubits)
        k = ql.Kernel('akernel', platform, num_qubits)

        matrix = [  complex(0.0, 0.0), complex(1.0, 0.0),
                    complex(1.0, 0.0), complex(0.0, 0.0)]
        u = ql.Unitary('u1', matrix)
        u.decompose()
        k.gate(u, [0])

        p.add_kernel(k)
        p.compile()
        
        qx.set(os.path.join(output_dir, p.name+'.qasm'))
        qx.execute()
        c0 = qx.get_state()
        self.assertAlmostEqual(helper_prob(matrix[0]), helper_regex(c0)[0], 5)
        self.assertAlmostEqual(helper_prob(matrix[2]), helper_regex(c0)[1], 5)

    def test_unitary_decompose_Y(self):
        num_qubits = 1
        p = ql.Program('test_unitary_Y', platform, num_qubits)
        k = ql.Kernel('akernel', platform, num_qubits)

        matrix = [  complex(0.0, 0.0), complex(0.0, -1.0),
                    complex(0.0, 1.0), complex(0.0, 0.0)]
        u = ql.Unitary('u1', matrix)
        u.decompose()
        k.gate(u, [0])

        p.add_kernel(k)
        p.compile()
        
        qx.set(os.path.join(output_dir, p.name+'.qasm'))
        qx.execute()
        c0 = qx.get_state()
        self.assertAlmostEqual(helper_prob(matrix[0]), helper_regex(c0)[0], 5)
        self.assertAlmostEqual(helper_prob(matrix[2]), helper_regex(c0)[1], 5)
 
    def test_unitary_decompose_Z(self):
        num_qubits = 1
        p = ql.Program('test_unitary_Z', platform, num_qubits)
        k = ql.Kernel('akernel', platform, num_qubits)

        matrix =  [  complex(1.0, 0.0), complex(0.0, 0.0),
                     complex(0.0, 0.0), complex(-1.0, 0.0)]
        u = ql.Unitary('u1', matrix )
        u.decompose()
        k.gate("hadamard", [0])
        k.gate(u, [0])
        k.gate("hadamard", [0])

        p.add_kernel(k)
        p.compile()
        
        qx.set(os.path.join(output_dir, p.name+'.qasm'))
        qx.execute()
        c0 = qx.get_state()
		#HZH = X, so the result should be |0> + |1>
        self.assertAlmostEqual(0, helper_regex(c0)[0], 5)
        self.assertAlmostEqual(1, helper_regex(c0)[1], 5)
   

    def test_unitary_decompose_IYZ(self):
        num_qubits = 1
        p = ql.Program('test_unitary_IYZ', platform, num_qubits)
        k = ql.Kernel('akernel', platform, num_qubits)

        u = ql.Unitary('X', [  complex(0.0, 0.0), complex(1.0, 0.0),
                                complex(1.0, 0.0), complex(0.0, 0.0)])
        u.decompose()
        k.gate(u, [0])
        u = ql.Unitary('Y', [  complex(0.0, 0.0), complex(0.0, -1.0),
                                complex(0.0, 1.0), complex(0.0, 0.0)])
        u.decompose()
        k.gate(u, [0])
        u = ql.Unitary('Z', [  complex(1.0, 0.0), complex(0.0, 0.0),
                                complex(0.0, 0.0), complex(-1.0, 0.0)])
        u.decompose()
        k.gate("hadamard", [0])
        k.gate(u, [0])

        p.add_kernel(k)
        p.compile()
        
        qx.set(os.path.join(output_dir, p.name+'.qasm'))
        qx.execute()
        c0 = qx.get_state()

        #jXYZ = I, so this should change nothing about the state.
        self.assertAlmostEqual(0.5, helper_regex(c0)[0], 5)
        self.assertAlmostEqual(0.5, helper_regex(c0)[1], 5)  

    def test_unitary_decompose_IYZ_differentorder(self):
        num_qubits = 1
        p = ql.Program('test_unitary_IYZ', platform, num_qubits)
        k = ql.Kernel('akernel', platform, num_qubits)

        u = ql.Unitary('X', [  complex(0.0, 0.0), complex(1.0, 0.0),
                                complex(1.0, 0.0), complex(0.0, 0.0)])
        
        u1 = ql.Unitary('y', [  complex(0.0, 0.0), complex(0.0, -1.0),
                                complex(0.0, 1.0), complex(0.0, 0.0)])
        
        u2 = ql.Unitary('Z', [  complex(1.0, 0.0), complex(0.0, 0.0),
                                complex(0.0, 0.0), complex(-1.0, 0.0)])
        u.decompose()
        u1.decompose()
        u2.decompose()

        k.gate("hadamard", [0])
        k.gate(u, [0])
        k.gate(u1, [0])
        k.gate(u2, [0])

        p.add_kernel(k)
        p.compile()
        
        qx.set(os.path.join(output_dir, p.name+'.qasm'))
        qx.execute()
        c0 = qx.get_state()

        #jXYZ = I, so this should change nothing about the state.
        self.assertAlmostEqual(0.5, helper_regex(c0)[0], 5)
        self.assertAlmostEqual(0.5, helper_regex(c0)[1], 5)  

    def test_unitary_decompose_nonunitary(self):
        num_qubits = 1
        p = ql.Program('test_unitary_nonunitary', platform, num_qubits)
        k = ql.Kernel('akernel', platform, num_qubits)

        u = ql.Unitary('WRONG', [  complex(0.0, 0.0), complex(0.0, 0.0),
                                complex(0.0, 0.0), complex(0.0, 0.0)])
        
        with self.assertRaises(Exception) as cm:
            u.decompose()
            k.gate(u, [0])

            add_kernel(k)
            p.compile()

        self.assertEqual(str(cm.exception), "Error: Unitary 'WRONG' is not a unitary matrix. Cannot be decomposed!(0,0) (0,0)\n(0,0) (0,0)\n")
  
  # input for the unitary decomposition needs to be an array
    def test_unitary_decompose_matrixinsteadofarray(self):
        num_qubits = 1
        p = ql.Program('test_unitary_wrongtype', platform, num_qubits)
        k = ql.Kernel('akernel', platform, num_qubits)
        
        with self.assertRaises(TypeError) as cm:
            u = ql.Unitary('TypeError', [[1, 0], [0,1]])



    def test_unitary_decompose_2qubit_CNOT(self):
        num_qubits = 2
        p = ql.Program('test_unitary_2qubitCNOT', platform, num_qubits)
        k = ql.Kernel('akernel', platform, num_qubits)

        matrix = [  complex(1.0, 0.0), complex(0.0, 0.0), complex(0.0, 0.0), complex(0.0, 0.0),
                               complex(0.0, 0.0), complex(1.0, 0.0), complex(0.0, 0.0), complex(0.0, 0.0),
                               complex(0.0, 0.0), complex(0.0, 0.0), complex(0.0, 0.0), complex(1.0, 0.0),
                               complex(0.0, 0.0), complex(0.0, 0.0), complex(1.0, 0.0), complex(0.0, 0.0)]
        u = ql.Unitary('cnot',matrix)
        u.decompose()
        k.gate(u, [0, 1])
        p.add_kernel(k)
        p.compile()


        qx.set(os.path.join(output_dir, p.name+'.qasm'))
        qx.execute()
        c0 = qx.get_state()

        #only two states are nonzero
        self.assertAlmostEqual(helper_prob(matrix[0]), helper_regex(c0)[0], 5)
        self.assertAlmostEqual(helper_prob(matrix[4]), helper_regex(c0)[1], 5)

    def test_unitary_decompose_2qubit_CNOT_2(self):
        num_qubits = 2
        p = ql.Program('test_unitary_2qubitCNOT', platform, num_qubits)
        k = ql.Kernel('akernel', platform, num_qubits)

        matrix = [  complex(1.0, 0.0), complex(0.0, 0.0), complex(0.0, 0.0), complex(0.0, 0.0),
                               complex(0.0, 0.0), complex(1.0, 0.0), complex(0.0, 0.0), complex(0.0, 0.0),
                               complex(0.0, 0.0), complex(0.0, 0.0), complex(0.0, 0.0), complex(1.0, 0.0),
                               complex(0.0, 0.0), complex(0.0, 0.0), complex(1.0, 0.0), complex(0.0, 0.0)]
        u = ql.Unitary('cnot2',matrix)
        u.decompose()
        k.x(1)
        k.gate(u, [0, 1])
        p.add_kernel(k)
        p.compile()


        qx.set(os.path.join(output_dir, p.name+'.qasm'))
        qx.execute()
        c0 = qx.get_state()

        #only two states are nonzero
        self.assertAlmostEqual(0.0, helper_regex(c0)[0], 5)
        self.assertAlmostEqual(1.0, helper_regex(c0)[1], 5)


    def test_non_90_degree_angle(self):
        num_qubits = 2
        p = ql.Program('test_unitary_non_90_degree_angle', platform, num_qubits)
        k = ql.Kernel('akernel', platform, num_qubits)

        matrix =  [-0.43874989-0.10659111j, -0.47325212+0.12917344j, -0.58227163+0.20750072j, -0.29075334+0.29807585j,  
                    0.30168601-0.22307459j,  0.32626   +0.4534935j , -0.20523265-0.42403593j, -0.01012565+0.5701683j , 
                    -0.40954341-0.49946371j,  0.28560698-0.06740801j,  0.52146754+0.1833513j , -0.37248653+0.22891636j,  
                    0.03113162-0.48703302j, -0.57180014+0.18486244j,  0.2943625 -0.06148912j,  0.55533888+0.04322811j]

        u1 = ql.Unitary("testname",matrix)
        u1.decompose()
        k.gate(u1, [0,1])

        p.add_kernel(k)
        p.compile()
        
        qx.set(os.path.join(output_dir, p.name+'.qasm'))
        qx.execute()
        c0 = qx.get_state()
        self.assertAlmostEqual(helper_prob(matrix[0]), helper_regex(c0)[0], 5)
        self.assertAlmostEqual(helper_prob(matrix[4]), helper_regex(c0)[1], 5)
        self.assertAlmostEqual(helper_prob(matrix[8]), helper_regex(c0)[2], 5)
        self.assertAlmostEqual(helper_prob(matrix[12]), helper_regex(c0)[3], 5)   

    def test_usingqx_00(self):
        num_qubits = 2
        p = ql.Program('test_usingqx00', platform, num_qubits)
        k = ql.Kernel('akernel', platform, num_qubits)

        matrix =  [-0.15050486+0.32164259j, -0.29086861+0.76699622j,
        0.17865218+0.18573699j, -0.31380116+0.19005417j,
       -0.65629705+0.20915109j,  0.32782708+0.16363753j,
       -0.54511727-0.21100055j,  0.0601221 -0.21446079j,
       -0.38935965-0.47787084j,  0.30279699-0.10056307j,
        0.04076564+0.54046282j, -0.23847619+0.40939808j,
        0.13874319-0.01460122j, -0.27256915+0.12950497j,
       -0.49774672+0.22449364j,  0.6159743 +0.46032394j]

        u1 = ql.Unitary("testname",matrix)
        u1.decompose()
        k.gate(u1, [0,1])
        p.add_kernel(k)
        p.compile()

        qx.set(os.path.join(output_dir, p.name+'.qasm'))
        qx.execute()
        c0 = qx.get_state()
        self.assertAlmostEqual(helper_prob(matrix[0]), helper_regex(c0)[0], 5)
        self.assertAlmostEqual(helper_prob(matrix[4]), helper_regex(c0)[1], 5)
        self.assertAlmostEqual(helper_prob(matrix[8]), helper_regex(c0)[2], 5)
        self.assertAlmostEqual(helper_prob(matrix[12]), helper_regex(c0)[3], 5)
        


    def test_usingqx_01(self):
        num_qubits = 2
        p = ql.Program('test_usingqx01', platform, num_qubits)
        k = ql.Kernel('akernel', platform, num_qubits)

        matrix =  [-0.15050486+0.32164259j, -0.29086861+0.76699622j,
        0.17865218+0.18573699j, -0.31380116+0.19005417j,
       -0.65629705+0.20915109j,  0.32782708+0.16363753j,
       -0.54511727-0.21100055j,  0.0601221 -0.21446079j,
       -0.38935965-0.47787084j,  0.30279699-0.10056307j,
        0.04076564+0.54046282j, -0.23847619+0.40939808j,
        0.13874319-0.01460122j, -0.27256915+0.12950497j,
       -0.49774672+0.22449364j,  0.6159743 +0.46032394j]

        k.x(0)
        
        u1 = ql.Unitary("testname",matrix)
        u1.decompose()   
        k.gate(u1, [0,1])

        p.add_kernel(k)
        p.compile()

        qx.set(os.path.join(output_dir, p.name+'.qasm'))
        qx.execute()
        c0 = qx.get_state()
        
        
        self.assertAlmostEqual(helper_prob(matrix[1]), helper_regex(c0)[0], 5)
        self.assertAlmostEqual(helper_prob(matrix[5]), helper_regex(c0)[1], 5)
        self.assertAlmostEqual(helper_prob(matrix[9]), helper_regex(c0)[2], 5)
        self.assertAlmostEqual(helper_prob(matrix[13]), helper_regex(c0)[3], 5)



    def test_usingqx_10(self):
        num_qubits = 2
        p = ql.Program('test_usingqx10', platform, num_qubits)
        k = ql.Kernel('akernel', platform, num_qubits)

        matrix =  [-0.15050486+0.32164259j, -0.29086861+0.76699622j,
        0.17865218+0.18573699j, -0.31380116+0.19005417j,
       -0.65629705+0.20915109j,  0.32782708+0.16363753j,
       -0.54511727-0.21100055j,  0.0601221 -0.21446079j,
       -0.38935965-0.47787084j,  0.30279699-0.10056307j,
        0.04076564+0.54046282j, -0.23847619+0.40939808j,
        0.13874319-0.01460122j, -0.27256915+0.12950497j,
       -0.49774672+0.22449364j,  0.6159743 +0.46032394j]



        u1 = ql.Unitary("testname",matrix)
        u1.decompose()
        k.x(1)
        k.gate(u1, [0,1])

        p.add_kernel(k)
        p.compile()

        qx.set(os.path.join(output_dir, p.name+'.qasm'))
        qx.execute()
        c0 = qx.get_state()
        
        self.assertAlmostEqual(helper_prob(matrix[2]), helper_regex(c0)[0], 5)
        self.assertAlmostEqual(helper_prob(matrix[6]), helper_regex(c0)[1], 5)
        self.assertAlmostEqual(helper_prob(matrix[10]), helper_regex(c0)[2], 5)
        self.assertAlmostEqual(helper_prob(matrix[14]), helper_regex(c0)[3], 5)
 

    
    def test_usingqx_11(self):
        num_qubits = 2
        p = ql.Program('test_usingqx11', platform, num_qubits)
        k = ql.Kernel('akernel', platform, num_qubits)

        matrix =  [-0.15050486+0.32164259j, -0.29086861+0.76699622j,
        0.17865218+0.18573699j, -0.31380116+0.19005417j,
       -0.65629705+0.20915109j,  0.32782708+0.16363753j,
       -0.54511727-0.21100055j,  0.0601221 -0.21446079j,
       -0.38935965-0.47787084j,  0.30279699-0.10056307j,
        0.04076564+0.54046282j, -0.23847619+0.40939808j,
        0.13874319-0.01460122j, -0.27256915+0.12950497j,
       -0.49774672+0.22449364j,  0.6159743 +0.46032394j]



        u1 = ql.Unitary("testname",matrix)
        u1.decompose()
        k.x(0)
        k.x(1)
        k.gate(u1, [0,1])

        p.add_kernel(k)
        p.compile()
        qx.set(os.path.join(output_dir, p.name+'.qasm'))
        qx.execute()
        c0 = qx.get_state()
        
        self.assertAlmostEqual(helper_prob(matrix[3]), helper_regex(c0)[0], 5)
        self.assertAlmostEqual(helper_prob(matrix[7]), helper_regex(c0)[1], 5)
        self.assertAlmostEqual(helper_prob(matrix[11]), helper_regex(c0)[2], 5)
        self.assertAlmostEqual(helper_prob(matrix[15]), helper_regex(c0)[3], 5)
    
    def test_usingqx_bellstate(self):
        num_qubits = 2
        p = ql.Program('test_usingqxbellstate', platform, num_qubits)
        k = ql.Kernel('akernel', platform, num_qubits)

        matrix =  [-0.15050486+0.32164259j, -0.29086861+0.76699622j,
        0.17865218+0.18573699j, -0.31380116+0.19005417j,
       -0.65629705+0.20915109j,  0.32782708+0.16363753j,
       -0.54511727-0.21100055j,  0.0601221 -0.21446079j,
       -0.38935965-0.47787084j,  0.30279699-0.10056307j,
        0.04076564+0.54046282j, -0.23847619+0.40939808j,
        0.13874319-0.01460122j, -0.27256915+0.12950497j,
       -0.49774672+0.22449364j,  0.6159743 +0.46032394j]



        u1 = ql.Unitary("testname",matrix)
        u1.decompose()
        k.hadamard(0)
        k.cnot(0, 1)

        k.gate(u1, [0,1])

        p.add_kernel(k)
        p.compile()
        qx.set(os.path.join(output_dir, p.name+'.qasm'))
        qx.execute()
        c0 = qx.get_state()
        
        self.assertAlmostEqual(0.5*helper_prob((matrix[0]+ matrix[3])), helper_regex(c0)[0], 5)
        self.assertAlmostEqual(0.5*helper_prob((matrix[4]+matrix[7])), helper_regex(c0)[1], 5)
        self.assertAlmostEqual(0.5*helper_prob((matrix[8]+ matrix[11])), helper_regex(c0)[2], 5)
        self.assertAlmostEqual(0.5*helper_prob((matrix[12]+ matrix[15])), helper_regex(c0)[3], 5)

    
    def test_usingqx_fullyentangled(self):
        num_qubits = 2
        p = ql.Program('test_usingqxfullentangled', platform, num_qubits)
        k = ql.Kernel('akernel', platform, num_qubits)

        matrix =  [-0.15050486+0.32164259j, -0.29086861+0.76699622j,
        0.17865218+0.18573699j, -0.31380116+0.19005417j,
       -0.65629705+0.20915109j,  0.32782708+0.16363753j,
       -0.54511727-0.21100055j,  0.0601221 -0.21446079j,
       -0.38935965-0.47787084j,  0.30279699-0.10056307j,
        0.04076564+0.54046282j, -0.23847619+0.40939808j,
        0.13874319-0.01460122j, -0.27256915+0.12950497j,
       -0.49774672+0.22449364j,  0.6159743 +0.46032394j]



        u1 = ql.Unitary("testname",matrix)
        u1.decompose()
        k.hadamard(0)
        k.hadamard(1)
        k.cnot(0, 1)

        k.gate(u1, [0,1])

        p.add_kernel(k)
        p.compile()
        qx.set(os.path.join(output_dir, p.name+'.qasm'))
        qx.execute()
        c0 = qx.get_state()
        
        self.assertAlmostEqual(0.25*helper_prob((matrix[0] + matrix[1] + matrix[2] + matrix[3] )), helper_regex(c0)[0], 5)
        self.assertAlmostEqual(0.25*helper_prob((matrix[4] + matrix[5] + matrix[6] + matrix[7] )), helper_regex(c0)[1], 5)
        self.assertAlmostEqual(0.25*helper_prob((matrix[8] + matrix[9] + matrix[10]+ matrix[11])), helper_regex(c0)[2], 5)
        self.assertAlmostEqual(0.25*helper_prob((matrix[12]+ matrix[13]+ matrix[14]+ matrix[15])), helper_regex(c0)[3], 5)

    def test_usingqx_fullyentangled_3qubit(self):
        num_qubits = 3
        p = ql.Program('test_usingqxfullentangled_3qubit', platform, num_qubits)
        k = ql.Kernel('akernel', platform, num_qubits)

        matrix = [-6.71424156e-01+0.20337111j, -1.99816541e-02+0.41660484j,
       -1.54134851e-01+0.1944923j ,  3.01737845e-01-0.27546267j,
        4.44627514e-02+0.09208708j,  2.15969389e-01+0.05602634j,
       -4.34685756e-02-0.18679037j, -1.06595244e-02-0.09070964j,
       -2.97602383e-01+0.04481956j, -2.02480684e-01-0.38012451j,
        2.82743377e-02+0.15314781j, -2.85687313e-01-0.15726268j,
       -1.70730583e-04-0.153385j  ,  2.29149718e-01+0.30741448j,
        2.10312109e-01+0.49212858j, -3.59933779e-01-0.0825481j ,
       -2.53660227e-01+0.17691007j,  3.13791732e-01+0.27041001j,
       -2.02215264e-01-0.11204691j, -8.86804543e-02+0.66976204j,
        7.33676579e-02-0.13082467j, -9.97863275e-02+0.01447563j,
       -5.89120641e-02+0.4293754j ,  3.27562664e-02+0.03619316j,
       -1.13124674e-01-0.00735472j,  9.70967395e-02-0.24582965j,
       -1.01309756e-01-0.39478792j,  1.63747452e-01-0.02575567j,
       -3.55829560e-01+0.48277026j,  2.92508802e-01-0.34856674j,
       -2.35490557e-01+0.20291125j, -2.05196931e-01+0.13496088j,
        1.00349815e-01-0.11576965j,  2.96991063e-01+0.12974051j,
       -4.63456662e-01+0.26944686j, -3.95037450e-01-0.08454082j,
       -4.48111249e-01+0.2913104j , -5.36978251e-02+0.25408623j,
        2.01534991e-01-0.12684844j,  6.37083728e-03+0.10506858j,
       -3.22312837e-01-0.15860928j,  1.56365350e-01-0.10492928j,
        3.40842166e-01+0.16770422j,  3.86154376e-02-0.15216514j,
       -2.21887101e-01-0.16990398j, -2.64609917e-01-0.06773659j,
       -2.76899134e-02+0.12469262j,  2.53068805e-01+0.65839736j,
       -1.47285402e-01+0.02010648j, -1.02440846e-01-0.36282662j,
       -4.07092321e-01+0.18619875j, -1.51508597e-01+0.08507565j,
       -1.60692869e-01-0.43953586j,  1.67488911e-01-0.42784326j,
       -1.68066530e-01-0.1864466j ,  3.03353139e-01-0.14152888j,
       -3.65022492e-01+0.03013316j,  2.23190344e-01-0.25817333j,
        8.15042751e-02-0.23635981j, -1.36339839e-01+0.07858377j,
        3.21627053e-02+0.00460768j, -4.58271701e-01-0.15103882j,
        2.70920093e-01-0.42767654j, -3.77444900e-01-0.17163736j]

        u1 = ql.Unitary("testname",matrix)
        u1.decompose()
        k.hadamard(0)
        k.hadamard(1)
        k.hadamard(2)
        k.cnot(0, 1)
        k.cnot(0, 2)
        k.cnot(1, 2)

        k.gate(u1, [0,1, 2])

        p.add_kernel(k)
        p.compile()
        qx.set(os.path.join(output_dir, p.name+'.qasm'))
        qx.execute()
        c0 = qx.get_state()
        
        self.assertAlmostEqual(0.125*helper_prob((matrix[0]  + matrix[1] + matrix[2] + matrix[3] + matrix[4] + matrix[5] + matrix[6] + matrix[7])), helper_regex(c0)[0], 5)
        self.assertAlmostEqual(0.125*helper_prob((matrix[8]  + matrix[9] + matrix[10]+ matrix[11]+ matrix[12]+ matrix[13]+ matrix[14]+ matrix[15])), helper_regex(c0)[1], 5)
        self.assertAlmostEqual(0.125*helper_prob((matrix[16] + matrix[17]+ matrix[18]+ matrix[19]+ matrix[20]+ matrix[21]+ matrix[22]+ matrix[23])), helper_regex(c0)[2], 5)
        self.assertAlmostEqual(0.125*helper_prob((matrix[24] + matrix[25]+ matrix[26]+ matrix[27]+ matrix[28]+ matrix[29]+ matrix[30]+ matrix[31])), helper_regex(c0)[3], 5)
        self.assertAlmostEqual(0.125*helper_prob((matrix[32] + matrix[33]+ matrix[34]+ matrix[35]+ matrix[36]+ matrix[37]+ matrix[38]+ matrix[39])), helper_regex(c0)[4], 5)
        self.assertAlmostEqual(0.125*helper_prob((matrix[40] + matrix[41]+ matrix[42]+ matrix[43]+ matrix[44]+ matrix[45]+ matrix[46]+ matrix[47])), helper_regex(c0)[5], 5)
        self.assertAlmostEqual(0.125*helper_prob((matrix[48] + matrix[49]+ matrix[50]+ matrix[51]+ matrix[52]+ matrix[53]+ matrix[54]+ matrix[55])), helper_regex(c0)[6], 5)
        self.assertAlmostEqual(0.125*helper_prob((matrix[56] + matrix[57]+ matrix[58]+ matrix[59]+ matrix[60]+ matrix[61]+ matrix[62]+ matrix[63])), helper_regex(c0)[7], 5)

    def test_usingqx_fullyentangled_4qubit(self):
        num_qubits = 4
        p = ql.Program('test_usingqxfullentangled_4qubit', platform, num_qubits)
        k = ql.Kernel('akernel', platform, num_qubits)

        matrix = [-0.11921901-0.30382154j, -0.10645804-0.11760155j,       -0.09639953-0.0353926j , -0.32605797+0.19552924j,        0.0168262 -0.26748208j, -0.17808469+0.25265196j,       -0.24676084-0.23228431j, -0.02960302+0.23697569j,
       -0.12435741-0.07223017j,  0.00178745+0.14813263j,       -0.11173158+0.26636089j,  0.27656908+0.05229833j,       -0.02964214-0.01505502j, -0.26959616+0.23274949j,       -0.18183627-0.04041783j,  0.05385991-0.05587908j,
        0.17894461-0.25668366j, -0.01553181-0.07446613j,        0.1876467 -0.49135878j, -0.18292006-0.04599956j,       -0.01618695+0.21951951j,  0.06003169-0.12728871j,       -0.04276406+0.08327372j,  0.30102765+0.18403071j,
       -0.08122018-0.08375638j, -0.02971758+0.09096399j,        0.10753511-0.03359547j, -0.1596309 +0.20649279j,       -0.13684564+0.29450386j,  0.20557421+0.24856224j,        0.0683444 +0.01780095j, -0.22317907-0.12123145j,
       -0.0323504 -0.02668934j,  0.08743777-0.49956832j,       -0.30202031-0.22517221j, -0.10642491-0.11883126j,       -0.13756817-0.20632933j,  0.02593802+0.00583978j,        0.05130972+0.06678859j, -0.10426135-0.14411822j,
        0.12318252+0.28583957j,  0.04903179-0.31898637j,       -0.07650819-0.07261235j, -0.22918932-0.28329527j,       -0.26553775+0.04563403j, -0.07728053+0.14952931j,       -0.10271285-0.00216319j, -0.09000117+0.09055528j,
       -0.15385903+0.01767834j,  0.42229431-0.05610483j,       -0.11330491-0.05458018j,  0.01740815-0.01605897j,       -0.11908997-0.01830574j,  0.21139794-0.10602858j,       -0.23249721-0.25516076j, -0.29066084-0.19129198j,
        0.21273108-0.14369238j, -0.20662513+0.14463032j,        0.2512466 -0.20356141j,  0.0869495 +0.24425667j,        0.09736427-0.03954332j,  0.1446303 +0.14263171j,       -0.25679664+0.09389641j, -0.04020309-0.19362247j,
        0.12577257-0.14527364j,  0.00371525+0.14235318j,       -0.22416134+0.02069087j,  0.03851418-0.05351593j,       -0.00289848-0.33289946j,  0.15454716-0.126633j  ,       -0.08996296-0.09119411j, -0.00804455-0.19149767j,
       -0.13311475-0.47100304j, -0.13920624-0.16994321j,       -0.05030304+0.16820614j,  0.05770089-0.15422191j,       -0.23739468-0.05651883j,  0.19202883+0.03893001j,        0.48514604+0.01905479j, -0.01593819-0.06475285j,
        0.31543713+0.41579542j, -0.08776349+0.24207219j,       -0.07984699-0.12818844j,  0.00359655+0.02677178j,       -0.12110453-0.25327887j, -0.21175671-0.1650074j ,       -0.14570465-0.05140668j,  0.06873883-0.01768705j,
       -0.13804809-0.16458822j,  0.15096981-0.02802171j,       -0.05750448-0.18911017j, -0.01552104+0.03159908j,       -0.0482418 +0.09434822j,  0.1336447 +0.22030451j,       -0.3771364 -0.17773263j,  0.16023381+0.26613455j,
        0.12688452-0.07290393j,  0.14834649+0.08064162j,       -0.06224533+0.04404318j,  0.03464369+0.19965444j,       -0.38140629-0.18927599j, -0.19710535-0.178657j  ,       -0.0507885 +0.19579635j,  0.11741615+0.13922702j,
        0.2673399 -0.01439493j,  0.10844591-0.19799688j,        0.01177533+0.031846j  , -0.07643954+0.25870281j,        0.28971442-0.25385986j, -0.23713666+0.01838019j,        0.1731864 -0.09372299j, -0.36912353-0.02243029j,
        0.03562803-0.09449815j,  0.13578229-0.19205153j,        0.21279127+0.14541266j, -0.20195524+0.187477j  ,       -0.06326783+0.0134827j ,  0.26953438-0.11153784j,       -0.28939961-0.08995754j,  0.20662437-0.15535337j,
       -0.03615272+0.00848631j,  0.14124129-0.10250932j,        0.08990493-0.13010897j, -0.04547667+0.17579099j,       -0.01292137+0.10354402j, -0.21338733-0.11928412j,        0.19733294+0.12876129j,  0.35162495+0.45226713j,
        0.17112722-0.18496045j, -0.34024071-0.09520237j,        0.18864652-0.07147408j,  0.31340662+0.24027412j,       -0.0720874 -0.11081564j,  0.08727975+0.02830958j,       -0.07584662-0.22555917j,  0.07086867-0.27714915j,
       -0.19116148-0.02164144j, -0.24831911+0.1055229j ,       -0.09939105-0.24800283j, -0.15274706-0.12267535j,        0.05237777-0.09974669j, -0.18435891-0.1737002j ,       -0.20884292+0.1076081j , -0.31368958-0.02539025j,
        0.03436293-0.19794965j,  0.11892581-0.17440358j,       -0.03488877+0.02305411j,  0.29835292-0.08836461j,        0.07893495-0.16881403j,  0.21025843+0.13204032j,        0.17194288-0.06285539j, -0.0500497 +0.35833208j,
       -0.14979745-0.07567974j,  0.00193804+0.04092128j,       -0.07528403-0.18508153j, -0.16873521-0.09470809j,        0.50335605+0.00445803j,  0.11671956+0.30273552j,        0.10253226-0.13365319j,  0.16676135+0.18345473j,
       -0.10096334-0.24031019j, -0.18452241+0.05766426j,        0.18102499-0.13532486j,  0.06252468-0.18030042j,       -0.00591499+0.07587582j, -0.35209025-0.12186396j,       -0.25282963-0.26651504j, -0.13074882+0.14059941j,
        0.18125386-0.03889917j,  0.06983104-0.3425076j ,        0.37124455-0.00305632j,  0.04469806-0.31220629j,        0.16501585+0.00125887j,  0.15895714-0.14115809j,       -0.01515444+0.06836136j,  0.03934186+0.13425449j,
        0.0513499 +0.21915368j,  0.00089628-0.3044611j ,        0.05443815-0.05530296j,  0.12091374-0.16717579j,       -0.06795704-0.2515947j , -0.43324316+0.13138954j,        0.03753289-0.00666299j,  0.16823686-0.22736152j,
       -0.00567807+0.05485941j, -0.11705816+0.39078352j,        0.29136164+0.18699453j, -0.09255109+0.08923507j,        0.11214398+0.00806872j,  0.02971631+0.05584961j,        0.2561    +0.22302638j,  0.12491596+0.01725833j,
        0.23473354-0.19203316j, -0.09144197-0.04827201j,       -0.0630975 -0.16831612j,  0.01497053+0.11121057j,        0.1426864 -0.15815582j,  0.21509872-0.0821851j ,        0.00650273+0.42560079j, -0.15721229+0.09919403j,
        0.18076365-0.05697395j, -0.10596487+0.23118383j,        0.30913352+0.24695589j, -0.03403863-0.01778209j,       -0.07783213-0.25923847j,  0.06847369-0.2460447j ,       -0.24223779-0.10590238j,  0.15920784+0.21435437j,
        0.26632193-0.02864663j,  0.06457043+0.0577428j ,       -0.38792984+0.08474334j,  0.00944311+0.22274564j,        0.11762823+0.36687658j, -0.1058428 -0.2103637j ,       -0.12970051-0.27031414j,  0.12684307+0.08398822j,
        0.06711923+0.23195763j, -0.04537262+0.26478843j,        0.10253668-0.07706414j, -0.13531665-0.27150259j,       -0.09124132-0.23306839j, -0.08631867+0.17221145j,        0.17654328-0.10341264j,  0.11171903-0.05824829j,
        0.04708668-0.13436316j, -0.10544253+0.07083904j,        0.04191629+0.28190845j, -0.4212947 -0.28704399j,        0.10278485+0.05713015j,  0.02057009-0.19126408j,        0.04856717+0.26648423j,  0.05388858-0.32433511j,
       -0.09408669-0.12159016j, -0.01355394+0.04757554j,        0.10925003-0.0453999j , -0.02512057-0.23836324j,        0.31375479-0.0993564j , -0.14702106+0.33395328j,       -0.1608029 +0.11439592j, -0.11028577-0.0093615j ,
       -0.08440005-0.12376623j,  0.12932188+0.09711828j,        0.18574716-0.06392924j, -0.13048059+0.0287961j ,       -0.29552716-0.08768809j, -0.02439943-0.01548155j,        0.07775135+0.00727332j,  0.1561534 -0.06489038j,
        0.46665242-0.07708219j, -0.05251139+0.37781248j,       -0.3549081 -0.10086123j,  0.11180645-0.40408473j,        0.03031085+0.16928711j,  0.1190129 -0.10061168j,        0.0318046 -0.12504866j,  0.08689947+0.07223655j]

        u1 = ql.Unitary("testname",matrix)
        u1.decompose()
        k.hadamard(0)
        k.hadamard(1)
        k.hadamard(2)
        k.hadamard(3)
        k.cnot(0, 1)
        k.cnot(0, 2)
        k.cnot(0, 3)
        k.cnot(1, 2)
        k.cnot(1, 3)
        k.cnot(2, 3)

        k.gate(u1, [0, 1, 2, 3])

        p.add_kernel(k)
        p.compile()
        qx.set(os.path.join(output_dir, p.name+'.qasm'))
        qx.execute()
        c0 = qx.get_state()
        
        self.assertAlmostEqual(0.0625*helper_prob((matrix[0]  +  matrix[1] +  matrix[2] +  matrix[3] +  matrix[4] +  matrix[5] +  matrix[6] +  matrix[7] +  matrix[8]  +  matrix[9] +  matrix[10]+  matrix[11]+  matrix[12]+  matrix[13]+  matrix[14]+  matrix[15])), helper_regex(c0)[0], 5)
        self.assertAlmostEqual(0.0625*helper_prob((matrix[16] +  matrix[17]+  matrix[18]+  matrix[19]+  matrix[20]+  matrix[21]+  matrix[22]+  matrix[23]+  matrix[24] +  matrix[25]+  matrix[26]+  matrix[27]+  matrix[28]+  matrix[29]+  matrix[30]+  matrix[31])), helper_regex(c0)[1], 5)
        self.assertAlmostEqual(0.0625*helper_prob((matrix[32] +  matrix[33]+  matrix[34]+  matrix[35]+  matrix[36]+  matrix[37]+  matrix[38]+  matrix[39]+  matrix[40] +  matrix[41]+  matrix[42]+  matrix[43]+  matrix[44]+  matrix[45]+  matrix[46]+  matrix[47])), helper_regex(c0)[2], 5)
        self.assertAlmostEqual(0.0625*helper_prob((matrix[48] +  matrix[49]+  matrix[50]+  matrix[51]+  matrix[52]+  matrix[53]+  matrix[54]+  matrix[55]+  matrix[56] +  matrix[57]+  matrix[58]+  matrix[59]+  matrix[60]+  matrix[61]+  matrix[62]+  matrix[63])), helper_regex(c0)[3], 5)
        self.assertAlmostEqual(0.0625*helper_prob((matrix[64] +  matrix[65]+  matrix[66]+  matrix[67]+  matrix[68]+  matrix[69]+  matrix[70]+  matrix[71]+  matrix[72] +  matrix[73]+  matrix[74]+  matrix[75]+  matrix[76]+  matrix[77]+  matrix[78]+  matrix[79])), helper_regex(c0)[4], 5)
        self.assertAlmostEqual(0.0625*helper_prob((matrix[80] +  matrix[81]+  matrix[82]+  matrix[83]+  matrix[84]+  matrix[85]+  matrix[86]+  matrix[87]+  matrix[88] +  matrix[89]+  matrix[90]+  matrix[91]+  matrix[92]+  matrix[93]+  matrix[94]+  matrix[95])), helper_regex(c0)[5], 5)
        self.assertAlmostEqual(0.0625*helper_prob((matrix[96] +  matrix[97]+  matrix[98]+  matrix[99]+  matrix[100]+ matrix[101]+ matrix[102]+ matrix[103]+ matrix[104] + matrix[105]+ matrix[106]+ matrix[107]+ matrix[108]+ matrix[109]+ matrix[110]+ matrix[111])), helper_regex(c0)[6], 5)
        self.assertAlmostEqual(0.0625*helper_prob((matrix[112] + matrix[113]+ matrix[114]+ matrix[115]+ matrix[116]+ matrix[117]+ matrix[118]+ matrix[119]+ matrix[120] + matrix[121]+ matrix[122]+ matrix[123]+ matrix[124]+ matrix[125]+ matrix[126]+ matrix[127])), helper_regex(c0)[7], 5)
        self.assertAlmostEqual(0.0625*helper_prob((matrix[128] + matrix[129]+ matrix[130]+ matrix[131]+ matrix[132]+ matrix[133]+ matrix[134]+ matrix[135]+ matrix[136] + matrix[137]+ matrix[138]+ matrix[139]+ matrix[140]+ matrix[141]+ matrix[142]+ matrix[143])), helper_regex(c0)[8], 5)
        self.assertAlmostEqual(0.0625*helper_prob((matrix[144] + matrix[145]+ matrix[146]+ matrix[147]+ matrix[148]+ matrix[149]+ matrix[150]+ matrix[151]+ matrix[152] + matrix[153]+ matrix[154]+ matrix[155]+ matrix[156]+ matrix[157]+ matrix[158]+ matrix[159])), helper_regex(c0)[9], 5)
        self.assertAlmostEqual(0.0625*helper_prob((matrix[160] + matrix[161]+ matrix[162]+ matrix[163]+ matrix[164]+ matrix[165]+ matrix[166]+ matrix[167]+ matrix[168] + matrix[169]+ matrix[170]+ matrix[171]+ matrix[172]+ matrix[173]+ matrix[174]+ matrix[175])), helper_regex(c0)[10], 5)
        self.assertAlmostEqual(0.0625*helper_prob((matrix[176] + matrix[177]+ matrix[178]+ matrix[179]+ matrix[180]+ matrix[181]+ matrix[182]+ matrix[183]+ matrix[184] + matrix[185]+ matrix[186]+ matrix[187]+ matrix[188]+ matrix[189]+ matrix[190]+ matrix[191])), helper_regex(c0)[11], 5)
        self.assertAlmostEqual(0.0625*helper_prob((matrix[192] + matrix[193]+ matrix[194]+ matrix[195]+ matrix[196]+ matrix[197]+ matrix[198]+ matrix[199]+ matrix[200] + matrix[201]+ matrix[202]+ matrix[203]+ matrix[204]+ matrix[205]+ matrix[206]+ matrix[207])), helper_regex(c0)[12], 5)
        self.assertAlmostEqual(0.0625*helper_prob((matrix[208] + matrix[209]+ matrix[210]+ matrix[211]+ matrix[212]+ matrix[213]+ matrix[214]+ matrix[215]+ matrix[216] + matrix[217]+ matrix[218]+ matrix[219]+ matrix[220]+ matrix[221]+ matrix[222]+ matrix[223])), helper_regex(c0)[13], 5)
        self.assertAlmostEqual(0.0625*helper_prob((matrix[224] + matrix[225]+ matrix[226]+ matrix[227]+ matrix[228]+ matrix[229]+ matrix[230]+ matrix[231]+ matrix[232] + matrix[233]+ matrix[234]+ matrix[235]+ matrix[236]+ matrix[237]+ matrix[238]+ matrix[239])), helper_regex(c0)[14], 5)
        self.assertAlmostEqual(0.0625*helper_prob((matrix[240] + matrix[241]+ matrix[242]+ matrix[243]+ matrix[244]+ matrix[245]+ matrix[246]+ matrix[247]+ matrix[248] + matrix[249]+ matrix[250]+ matrix[251]+ matrix[252]+ matrix[253]+ matrix[254]+ matrix[255])), helper_regex(c0)[15], 5)



    def test_usingqx_fullyentangled_5qubit(self):
        num_qubits = 5
        p = ql.Program('test_usingqxfullentangled_5qubit', platform, num_qubits)
        k = ql.Kernel('akernel', platform, num_qubits)

        matrix = [ 0.31869031-9.28734622e-02j,  0.38980148+2.14592427e-01j,  0.20279154-1.56580826e-01j,  0.00210273-2.83970875e-01j, -0.04010672-9.26812940e-02j,  0.15747049+6.08283291e-02j, -0.05274772-6.45225841e-02j,  0.05909439-3.23496026e-02j,  0.07868009+6.77298732e-02j,  0.11290142+2.48468763e-02j,  0.10092033-7.03646679e-02j, -0.03557972-3.39117261e-02j,  0.02241562+1.02714684e-02j, -0.04088796-1.12143173e-01j, -0.00144015+1.83925572e-02j, -0.1257948 -1.15394681e-01j,  0.13531631-1.84271421e-01j,  0.11293592-1.44170392e-01j, -0.06985893-1.36606289e-01j, -0.08924113-6.35923942e-02j,  0.19393965-6.35656579e-02j, -0.02153233-2.76955143e-01j, -0.12094999+1.05716669e-01j, -0.00624879+1.27471575e-01j, -0.13608981-1.22140261e-01j, -0.08243959-1.72734247e-02j,  0.06562052+1.12197102e-01j, -0.01102025-1.53811069e-01j,  0.13576338-8.95540026e-02j,  0.05730484-5.62973657e-02j, -0.08835493-4.13159024e-02j, -0.04533295+3.70144188e-02j,  0.04116657+1.58298973e-01j,  0.22270976-1.18654603e-01j,
        0.09219815+8.41433858e-02j, -0.0227499 +2.33343627e-01j, -0.13334752-6.98059663e-02j, -0.07967301-6.54413909e-02j, -0.09158972-7.03627564e-02j, -0.02332636-1.58060290e-02j,  0.23523954-3.48258461e-02j,  0.21329683+6.37364245e-02j,  0.25821394+1.30648709e-03j, -0.1118809 -1.95903237e-01j, -0.07887212-8.81857936e-02j,  0.047632  +9.40535593e-02j, -0.11726632+5.65815234e-02j,  0.08229469-4.38047420e-02j,  0.03117762+1.08469602e-01j,  0.16064616-1.30734886e-01j,  0.22181279-3.31211823e-02j,  0.07309884-2.01468425e-01j, -0.28437382+6.38292807e-02j, -0.23661479+1.17105208e-01j, -0.10436123-5.56443645e-02j,  0.20587544+1.99848797e-02j,  0.20705551+1.75496198e-02j,  0.01791984-8.31956939e-02j,  0.09737992+6.64713628e-03j,  0.0617566 -7.11998970e-02j,  0.03073603-7.25651496e-02j, -0.12056479-1.55358931e-01j, -0.09661589+4.90232280e-03j,  0.03110935-1.72748422e-01j,  0.01473304+3.62629583e-01j,  0.11031868-3.67435519e-03j,  0.04301239-1.64601498e-01j, -0.16432353+7.52258615e-02j,
       -0.00648176+6.66215752e-02j,  0.1257427 -3.71759618e-02j, -0.07793634-7.01586304e-02j,  0.0571156 +2.67410653e-01j, -0.00880782+2.09982981e-01j, -0.04961517-6.86953748e-02j, -0.15766986-1.12761472e-01j, -0.07076146-3.12372790e-02j, -0.08715236+9.29279186e-02j,  0.03843817+1.13545744e-01j, -0.10838483-1.19949631e-02j, -0.06342622+1.73231077e-01j,  0.23221532-4.28875040e-02j,  0.16514567-4.90633428e-02j, -0.0109654 +6.37891447e-02j,  0.02171279+5.89435085e-02j,  0.0199925 -2.96609865e-01j,  0.16702912+8.82996853e-02j,  0.19820571+5.01863737e-02j, -0.01237253-1.21196566e-01j,  0.11042294-2.16811518e-01j, -0.01004066-2.36556101e-01j, -0.1286729 -4.01294536e-03j, -0.11254789+5.56025311e-02j, -0.18328088-1.46524167e-02j, -0.13640243-3.71920766e-02j, -0.03793125+9.57786229e-02j,  0.07600542+1.53449546e-01j, -0.12371969+2.19599454e-01j,  0.18106889-3.10905810e-02j, -0.12698597+7.45431556e-02j,  0.02402161-1.66514384e-02j, -0.1431313 +4.63501164e-02j,  0.13656065-8.55748891e-02j,
       -0.2085481 -3.99831111e-02j, -0.05354994+8.14822771e-02j,  0.22188566+8.04753102e-02j, -0.09702996-1.54653045e-01j, -0.1018562 -1.01790676e-01j, -0.00335582+2.52475965e-01j, -0.04858119-1.52409323e-01j,  0.09335149-3.90325714e-02j,  0.08277598-4.08970043e-02j,  0.08119049+5.11051755e-02j, -0.01223217+6.17029648e-04j, -0.15137858-9.30742653e-03j,  0.1064706 +3.35870801e-02j, -0.04014048+1.33388615e-01j, -0.04315986+1.74367473e-01j, -0.04774817-9.82134162e-02j, -0.08203436+7.74038565e-02j, -0.2339137 +1.62306949e-01j,  0.11791971+2.08966161e-01j, -0.07867189-4.18549851e-02j, -0.17291904+9.47505092e-02j,  0.23630015-3.07786925e-01j, -0.09104365-1.33230919e-01j,  0.07679465+1.45726380e-01j,  0.09397243-9.26081447e-02j, -0.16092509+7.38633830e-02j, -0.02289017+1.59777790e-01j,  0.12053722+6.55620680e-02j,  0.12646833+1.04585433e-01j,  0.0264461 +1.92009889e-01j,  0.28749442+1.11327480e-01j,  0.15837267+6.12698488e-02j,  0.18181681-5.61278874e-02j,  0.07897084-1.17114924e-01j,
        0.19787494-1.67863968e-01j,  0.03852499-4.21534504e-02j, -0.14091989-1.27321872e-01j, -0.01747868-2.27419946e-01j, -0.02073576-6.24641146e-02j, -0.14241142+2.28697604e-02j,  0.15468346+2.08734157e-02j,  0.04505001-9.46333157e-02j, -0.09148317-2.26152985e-01j,  0.08577989+2.30863031e-01j, -0.08152502-8.74340592e-02j, -0.06465018-7.35108360e-02j, -0.21595512-8.66695180e-03j,  0.03304541-1.91886499e-01j,  0.04448112+3.67289455e-02j,  0.01969849-5.34777195e-02j, -0.03209367+3.02797601e-02j,  0.17389718+1.19098672e-01j, -0.10192559-3.02362398e-01j, -0.10224653-8.09241417e-02j, -0.12601597-6.86631211e-02j,  0.11275009+1.40915100e-01j,  0.03214418+1.48701215e-01j, -0.11571373+3.57083353e-02j,  0.03222285-5.19221441e-02j, -0.00250593+7.32186819e-02j, -0.15549573+1.77404851e-01j,  0.08568275+1.02973155e-01j,  0.11314133-2.13171498e-01j,  0.22748546-5.69951563e-02j,  0.14929197+1.31715414e-01j, -0.16301646+3.65045553e-03j,  0.13494125+2.31602145e-01j,  0.24267648-1.77780817e-01j,
       -0.00736974+7.72680290e-02j, -0.04867011+1.22605525e-01j,  0.05051748+8.06196677e-02j, -0.01749415+6.33486594e-02j,  0.06408343-4.51492484e-02j, -0.02543799-1.33253676e-01j, -0.05785745-4.71803669e-02j, -0.24358675-1.18083337e-02j,  0.00061111-5.96573111e-03j,  0.17487681-8.55625475e-02j,  0.13181372-5.47918839e-02j, -0.14072394+1.08475923e-01j,  0.05919593+1.78284217e-01j, -0.00292253-2.17168452e-02j,  0.19948189-1.95177980e-01j, -0.11146926-1.55684600e-01j,  0.02692391-9.18282632e-02j, -0.03804582-7.62499524e-02j,  0.18562866+1.41900953e-01j,  0.15468899+7.03401694e-03j,  0.10617502+2.96183301e-01j,  0.12124371+1.42279327e-02j, -0.12974516+1.87600027e-01j, -0.23270386+1.59618723e-01j, -0.14928652-8.27397154e-02j,  0.06670844+6.35681954e-02j,  0.03466266+1.89244901e-01j, -0.00961646-1.02903972e-01j,  0.19899344-2.38199195e-01j, -0.16594523-4.43067845e-02j,  0.27258801-7.49570862e-02j,  0.08182091+2.03334342e-02j, -0.02454051-4.82668158e-02j, -0.08557127+3.04552132e-02j,
        0.01303575+8.88057122e-02j,  0.13253621+5.05712004e-02j,  0.07763894+1.13841327e-01j, -0.28730914-4.49839630e-02j,  0.07031891-3.38973984e-03j, -0.04794523-1.67972335e-01j, -0.18102171+2.08760542e-01j, -0.08845631-9.70771934e-02j,  0.03546718+4.80846877e-02j, -0.08866794-5.19207245e-02j, -0.15099182-9.77070105e-02j, -0.05473901+2.33013978e-01j, -0.04645356-8.89489153e-02j,  0.07548625+1.80517821e-01j,  0.16840691+2.15268260e-01j, -0.09962074-7.35772706e-03j, -0.10104245+1.74017433e-02j, -0.09302095-1.10945928e-01j, -0.035157  +2.50873519e-02j,  0.10622507+1.29398421e-01j, -0.20965829-4.55862887e-02j,  0.05220909+6.66834897e-02j, -0.09672108+1.09403436e-01j,  0.12697788-6.19212188e-02j,  0.05729917+1.44378294e-01j, -0.15808366-3.82581632e-02j,  0.10716098-4.62942406e-02j,  0.24961573+1.32840816e-01j,  0.0627808 +2.39056706e-02j,  0.18311487+7.98582936e-03j,  0.02661541+7.40701198e-02j,  0.04699528+1.88767961e-01j,  0.04750332-2.25995642e-02j,  0.07193907+5.68042859e-02j,
       -0.02763382-3.43370436e-02j,  0.11205088-1.98370084e-02j, -0.11424545+1.14269285e-03j,  0.06885854-2.09915302e-01j, -0.05625496-2.88701666e-01j, -0.08183393+2.31774294e-01j, -0.09301037-4.03842722e-02j, -0.06069309+2.57502142e-02j, -0.18634877+9.85377377e-02j,  0.01474395+9.22414287e-02j,  0.04147054-1.74520016e-01j,  0.06794102-1.59508183e-01j, -0.13292245-5.38490513e-02j, -0.18753702+2.82751900e-01j,  0.00899866-2.80951474e-01j,  0.20377748+8.19207845e-02j, -0.14028603-1.39624056e-01j,  0.10741114+2.60308736e-02j, -0.22833573+6.02730469e-02j,  0.16071761-1.25615817e-01j,  0.08060865-5.81044643e-03j,  0.16141495-4.86994019e-02j,  0.11263654-1.87107794e-02j,  0.06859488-1.90280093e-01j,  0.10733862+4.45380867e-02j, -0.11489398-2.10578670e-01j, -0.0864833 -1.73958604e-01j,  0.12293904-1.50703768e-01j, -0.00629652+2.98133822e-02j,  0.16452648+7.76415767e-02j,  0.16219009+2.46017766e-01j, -0.04582374-1.16806045e-01j,  0.15109013+9.45671154e-03j,  0.04773975-1.54745106e-01j,
        0.21822796-1.87975523e-03j,  0.16596711-1.31539539e-01j,  0.25621724-4.31460990e-02j,  0.23071937+1.38086347e-01j,  0.10615413-1.24941186e-01j,  0.00343902+5.79927212e-02j,  0.10569282-2.64952183e-01j, -0.13601637+5.39259774e-02j, -0.13182901+9.15307749e-02j,  0.03524551-1.35956850e-01j,  0.0957622 -1.71837228e-02j, -0.03707841-5.40514827e-02j, -0.05729147-7.33164458e-02j, -0.18631267+1.00759871e-01j, -0.02434746-2.22824245e-02j, -0.02570765-3.90074186e-02j,  0.17946901+1.86288095e-02j,  0.07742897+3.19064602e-02j, -0.17538437+2.18666227e-01j,  0.10029635-8.92554711e-03j, -0.13379504-1.03683797e-02j, -0.11115363-4.48983441e-02j,  0.00163172-2.05325390e-02j,  0.02753729-1.28591197e-01j,  0.01933548+2.37393316e-01j,  0.07102201-1.19729329e-01j, -0.07517562+7.47947883e-02j, -0.03383077-1.23796732e-01j, -0.05575669-2.97673975e-02j, -0.04860924+1.02986531e-01j,  0.11384071+2.36429953e-01j, -0.26355793+1.30580300e-02j, -0.2665541 +6.72995742e-02j, -0.04663019-5.64857364e-03j,
        0.15703188+2.25734925e-02j, -0.02611337-4.94904084e-03j,  0.31897764+7.13870567e-02j,  0.15501272-1.48107261e-01j,  0.18034006-2.37497106e-01j,  0.21350438-8.08313917e-02j,  0.0620688 +5.98457145e-02j, -0.04996277+1.09069384e-02j, -0.09282843-5.59942293e-02j, -0.05363359+4.83839918e-02j, -0.20946404-1.85419524e-01j, -0.05481421+8.78420662e-02j, -0.19261214-1.04875021e-01j, -0.00083685-2.70091261e-02j, -0.17618069+8.47803615e-02j,  0.16055652+5.98821538e-03j, -0.0635621 +8.16228775e-02j, -0.22507829-4.14268948e-02j, -0.00791737+7.40481825e-02j,  0.19970347-1.15972939e-01j, -0.08619572+1.19678540e-01j, -0.184931  -1.13524848e-01j,  0.04824348-5.85668978e-02j, -0.22446284+1.11472258e-02j,  0.09629746-1.88788542e-01j, -0.18228537-3.89615785e-04j,  0.09084756+4.78058679e-02j, -0.00889583-1.74554680e-01j, -0.07447697-4.48823321e-02j, -0.05076077-6.24206002e-02j, -0.14050658+8.38982646e-02j,  0.07428934-6.30440232e-02j,  0.05701074-1.00819168e-01j,  0.07009481+8.11789378e-02j,
       -0.01155395+2.46650660e-01j,  0.25264038-1.98419236e-01j, -0.09155521-2.79616998e-02j,  0.06750545-1.52831769e-01j,  0.1068504 -1.78545409e-01j, -0.06203486+2.86380485e-02j,  0.23067545-1.23638547e-01j, -0.02397582+2.15643846e-01j,  0.01810979+1.08067552e-01j,  0.09130261-4.68850866e-02j,  0.13093158-1.51875212e-01j,  0.25207024+5.80216347e-02j,  0.02600999-1.27799785e-01j,  0.03509714-5.46256940e-02j,  0.04362407-3.07563727e-02j,  0.12488249+6.70881126e-02j,  0.05539679+1.15790184e-01j,  0.23107124-9.66168589e-02j, -0.09977799-9.88045601e-02j, -0.05633534+2.79762650e-01j, -0.02304281-2.42282613e-02j,  0.1276518 -2.32550502e-02j,  0.25634814-3.02429796e-02j,  0.30827737-2.19288542e-01j, -0.02512791+2.13783374e-01j,  0.15453741-7.04163688e-02j, -0.21441715+2.05959415e-01j, -0.02827968+4.71161636e-02j, -0.03837343+6.70595235e-02j, -0.07854483-3.12796395e-02j, -0.03615927+1.10733671e-01j,  0.21779074-1.39489256e-01j,  0.06619192+5.38352432e-02j, -0.0680216 -1.05456770e-01j,
       -0.10316128+1.32947183e-01j, -0.06345699-1.85555087e-01j, -0.00893067+1.52852423e-01j,  0.03135864+1.10423220e-01j, -0.182351  +2.77954961e-02j, -0.09252137+1.09061091e-01j, -0.0797695 +1.15176285e-02j,  0.05051009+2.57152535e-01j,  0.09344185-1.12906975e-01j,  0.04587659+8.90786692e-02j, -0.02471718+4.53344925e-02j, -0.08880236+3.62673481e-02j, -0.05981018+9.78912466e-02j, -0.00118743-5.07702805e-02j, -0.08119522+5.16043940e-02j,  0.11854883+9.79110177e-02j,  0.28543526-8.57540624e-02j,  0.28069514+2.00242562e-01j, -0.01036913-1.12897546e-01j, -0.08470073+1.91009156e-02j,  0.01428585+7.92620224e-02j, -0.15567322-4.85322282e-02j,  0.14203257-2.97248067e-01j,  0.15075682-1.33294241e-01j,  0.08107944-6.10561974e-02j,  0.07558358-3.29486035e-02j, -0.04042877-2.49142675e-02j,  0.05557881-1.93956804e-01j, -0.00589446+9.44678255e-02j,  0.1653676 -4.08479951e-02j,  0.14382039+1.57269491e-02j, -0.22311804-8.79699173e-02j,  0.09825566+1.52084444e-02j,  0.00938452-3.15649024e-01j,
       -0.03982978+1.22637508e-01j, -0.15670677-1.93426126e-01j,  0.22741559-4.39643322e-02j, -0.13362661-9.94353174e-02j, -0.09445278+4.93195562e-02j, -0.04684833-8.59308253e-02j,  0.08890091-1.16876390e-01j, -0.18726614+7.50813913e-02j, -0.01806267-2.00321404e-02j,  0.09338316-1.69711164e-01j, -0.03114173+1.43744729e-01j, -0.01515641+7.72217128e-02j,  0.21301431+5.92330407e-02j,  0.12250555+1.16784140e-01j,  0.0492589 +1.82225369e-01j,  0.09296806+1.58772507e-02j, -0.06700279+7.86869873e-02j, -0.02389099-5.36170682e-02j,  0.00545649-1.25491880e-03j,  0.09228983+2.64561438e-01j, -0.08125859-3.27415023e-02j, -0.03769576-2.64763488e-01j,  0.0914594 +1.90133579e-01j, -0.27352411+6.32203214e-02j, -0.00087467-8.61094525e-02j,  0.13735927+8.73538830e-03j, -0.08484748+1.68213884e-01j, -0.07040327-1.76254392e-01j, -0.08363638-8.30902369e-02j,  0.14643814+6.63711322e-02j, -0.31659998-1.04317904e-01j,  0.07830602+2.93183609e-02j,  0.11041879+1.50347028e-01j, -0.00433818-1.72946523e-01j,
       -0.15422074+9.38342826e-02j, -0.14194574+3.64105744e-02j,  0.06297505+7.88548664e-02j,  0.01106598-2.65402837e-01j,  0.07164956-8.12953528e-02j, -0.12514039-1.84403896e-01j, -0.17295559+3.58171829e-02j, -0.03655755-7.10559195e-02j, -0.03821694-1.10973040e-01j,  0.18546141+1.10662839e-01j, -0.10195195+1.32367694e-01j, -0.05016084+2.78628720e-02j, -0.19079393+1.40041256e-02j,  0.07878861-3.38112621e-02j,  0.13730204+1.52897765e-01j,  0.11815899+3.49910958e-02j, -0.02648579-1.03280312e-01j,  0.11970226-2.50844123e-01j,  0.10406627-9.12689026e-02j,  0.07247411-2.51491332e-01j,  0.06550999-2.94078974e-01j, -0.12079469-5.00355065e-02j,  0.06000345-1.02467349e-01j,  0.08823557+3.66136284e-02j,  0.02291526+3.63437115e-02j, -0.07124433+2.31676270e-01j,  0.17493588+5.98421039e-02j, -0.17824122-1.97944096e-02j, -0.00491994-9.10500757e-02j,  0.19633245+6.47815099e-02j, -0.03770238-7.48581318e-02j, -0.18402244+9.04322714e-02j, -0.2842845 -1.40285725e-01j,  0.00138785+1.08042021e-01j,
        0.16088797+2.75183648e-01j, -0.0865634 -7.36993417e-02j,  0.05178655+5.29576072e-02j, -0.05123872-1.29090313e-02j, -0.03079395-1.69557846e-01j,  0.23219467-7.71729365e-03j, -0.01135984-6.49120093e-02j,  0.08717993-9.71885487e-02j, -0.02043387+1.16518568e-01j, -0.08256357+1.23302259e-02j,  0.13742387-2.00583302e-01j,  0.11087765+2.22213806e-02j,  0.10703254+3.53681589e-02j, -0.04998617+7.13667998e-02j, -0.2463513 -2.30198030e-01j,  0.09831487+1.92709068e-01j,  0.10999549+1.28740664e-01j, -0.04056669+9.08016920e-02j, -0.03342728-9.92407179e-02j,  0.09352449-4.93911914e-02j, -0.11841153-8.25481366e-02j, -0.0263205 +1.91245810e-01j, -0.15521225-6.80613837e-02j,  0.18223827-2.68908129e-01j,  0.0411945 +1.12072453e-01j, -0.02954085+6.34315408e-02j, -0.01019486-1.24389083e-01j,  0.19174201-7.05812742e-02j,  0.19351232+1.42610666e-02j,  0.12937332-1.45006522e-01j,  0.01111355+1.11974478e-01j,  0.25163849+7.58955475e-02j,  0.06967535+1.51710939e-01j, -0.26064587+5.85797495e-02j,
       -0.05364486-8.37537594e-02j,  0.02473011+1.57624180e-01j,  0.0788524 +4.24067733e-02j, -0.05353458-3.59527188e-02j,  0.24630053-3.09146961e-01j,  0.12673895+2.54501080e-02j,  0.13328446+1.27420821e-01j, -0.03311185+8.64376474e-02j, -0.10126181-3.33404906e-02j,  0.05819512-2.42452607e-01j,  0.12966928+1.92345459e-01j, -0.10237172+7.87217786e-02j, -0.12008   +8.02686485e-02j,  0.11309599+3.47582961e-02j,  0.16921731-6.49645604e-02j,  0.10847358+1.32814258e-01j,  0.09620142+8.29342007e-02j, -0.03836254+1.18815716e-01j,  0.10334279-1.21404737e-02j, -0.19366239-5.73423731e-02j,  0.11445103-7.69154143e-02j,  0.10345336+3.42165082e-02j,  0.0870257 +5.06977361e-02j, -0.0262899 +4.49013808e-02j, -0.0367388 +1.36360059e-02j,  0.00976608-1.86860241e-01j,  0.15386709+1.46930831e-01j,  0.11249562+2.57278918e-02j,  0.08496388-1.11776874e-01j, -0.07162906-1.41500632e-02j,  0.076649  -2.72671417e-01j,  0.12029408-2.43316647e-01j,  0.29672048-6.39702972e-02j,  0.1939962 -1.55753156e-02j,
        0.08677801+1.96162231e-01j,  0.1237203 -9.61956522e-02j, -0.16119964+3.89474786e-02j,  0.09207276-2.54823411e-01j,  0.03475803-5.50657361e-02j, -0.21263069-7.05923500e-02j, -0.06092331+4.49331302e-02j, -0.02461632+3.39504695e-02j,  0.0345771 +2.39801010e-01j,  0.00891307+1.12739196e-01j,  0.13060822-1.11694609e-01j,  0.09803224+1.73134288e-01j, -0.07234639+1.28840723e-02j,  0.11471256+3.77081748e-02j,  0.26422031-1.58104940e-01j,  0.02654836-1.38859272e-01j, -0.04278432-1.46473478e-02j,  0.28242235-5.27977137e-02j, -0.13193171+1.21081065e-01j,  0.06133633-4.22267064e-02j, -0.08280452+2.04720842e-02j,  0.02083153+5.42313877e-02j,  0.12016721+1.60501416e-01j, -0.12225807-2.76668735e-01j,  0.02102305+9.09998231e-02j,  0.11747776+3.62319311e-01j,  0.09912628+1.50849159e-02j, -0.15640258+2.48010133e-02j,  0.0546514 +2.59861755e-02j, -0.01002552+6.65787181e-02j, -0.02898995+9.89915581e-02j, -0.00717829-1.37245312e-01j,  0.00055712+1.86303843e-01j,  0.04400996+1.19535156e-01j,
        0.05636026-2.16779961e-01j, -0.05913502-2.77370485e-01j,  0.20737913+2.02848948e-01j, -0.07528717+7.93840780e-02j,  0.05451348+1.33425914e-01j, -0.12290648+7.58881312e-02j,  0.10321466+1.24983234e-01j,  0.13371569-1.00033395e-01j, -0.05791197+2.68575587e-02j,  0.04652659+4.60658211e-02j,  0.00131806+5.92140285e-02j, -0.13481008+2.31672893e-02j, -0.13689412+1.17954022e-01j,  0.04998811-2.02833835e-02j, -0.14165519+3.56756720e-01j, -0.07856852+6.35118506e-02j,  0.05564543-9.74922249e-02j, -0.21743105-5.41409515e-02j, -0.16217875+7.97524864e-02j, -0.19486651-9.99801439e-02j,  0.19124263-4.71432152e-02j,  0.01035372+4.63843919e-02j, -0.07218978+1.32977797e-01j, -0.21002818-1.31679835e-01j,  0.05305256-1.54340691e-02j, -0.19401635+6.69582303e-02j,  0.05139607+1.93092981e-01j, -0.09946416+4.37566125e-02j,  0.10192397-4.16617603e-02j, -0.12405403-9.78811063e-02j,  0.05812519-3.95372746e-05j,  0.00099002-1.14739425e-02j,  0.05247123-8.40118490e-02j,  0.04501583+1.44968196e-01j,
        0.03301905-4.80048648e-02j,  0.02823323-1.78191318e-02j,  0.08196271-6.89791351e-03j,  0.02038618+7.47434818e-02j, -0.0372285 +4.85572384e-02j,  0.03293172+1.46606707e-02j, -0.18339563+6.37443359e-02j, -0.11214925-1.13505125e-01j, -0.06063048+1.45448526e-01j,  0.34233663-3.24467430e-01j,  0.07664073-1.44247607e-01j, -0.28993637+1.68069982e-01j, -0.18237161+2.05640159e-01j,  0.05552109+1.44200044e-01j, -0.08757971-2.66443942e-01j,  0.1558971 +2.49096061e-02j, -0.16421838+3.76276364e-02j,  0.08841799+3.97688126e-02j,  0.06127403+8.79601343e-02j, -0.11912802+9.38176361e-02j, -0.01512363+1.98779455e-01j, -0.10425168-2.89846412e-02j,  0.16046822-1.77676457e-02j,  0.04871864-4.15742404e-02j,  0.19325653-1.69328440e-01j, -0.059617  +1.87459639e-02j, -0.23497369+7.78523444e-02j,  0.03611522+4.96031106e-02j,  0.02849174+1.81422119e-01j, -0.13379238-1.46097376e-01j,  0.13609017-3.86016200e-02j, -0.13135849-9.04921922e-02j,  0.1333314 +1.02431026e-01j,  0.23331259-1.23168916e-02j,
        0.05106233+7.45228667e-02j,  0.12189655+6.42850380e-02j,  0.04943332+2.95462475e-02j, -0.00657986+8.06994964e-02j,  0.3267687 +8.99736406e-02j, -0.14260504+1.75648292e-01j,  0.23063331+1.78219849e-01j,  0.10883261+1.28389828e-01j, -0.10597288-1.40386755e-01j,  0.01512138+5.26532655e-02j, -0.24270928-2.39424278e-01j,  0.08582485+1.81061208e-01j,  0.15870471+3.82006536e-03j, -0.14205944-7.88677962e-02j,  0.26780226+1.17371947e-01j, -0.05734894+1.19255894e-01j, -0.00708958-3.87661515e-02j, -0.0123964 +1.17519163e-01j, -0.05307584+9.65095189e-02j, -0.04746522+6.19590573e-02j, -0.03677784-2.76400184e-02j,  0.14375332+9.66259420e-02j, -0.09175759-1.16174795e-01j, -0.03764456-1.92660525e-02j, -0.14188024+1.51742465e-01j, -0.00855281-6.80432250e-02j, -0.13963677+6.77341017e-02j,  0.27226725-5.95170973e-03j,  0.07653268+6.86308978e-02j, -0.18769106+2.15711501e-01j,  0.16136537-4.42590061e-02j, -0.26360596+1.04686372e-01j, -0.09155045-8.77382640e-02j, -0.09151231+4.28425508e-02j,
       -0.06373803+3.71389234e-03j,  0.12982765+1.93272289e-01j, -0.00676779+9.13731987e-02j, -0.15669531-1.03446811e-01j,  0.13876435+1.73790050e-01j,  0.03724986-6.80542715e-02j,  0.13536485+2.06818477e-02j,  0.2134622 +7.76124343e-02j, -0.21930729-1.35324670e-02j, -0.15870289-1.47418944e-01j, -0.10525782-3.21051509e-02j,  0.02651126-7.35636015e-02j,  0.21299852-4.88944015e-02j,  0.1520081 +1.18419448e-01j,  0.02837817-1.66734491e-02j, -0.05583337+7.15315919e-02j,  0.00827433-9.71346050e-02j, -0.10828369+1.50029916e-01j, -0.05569338+1.56736408e-01j, -0.10498402-3.01603733e-02j,  0.01972677-3.28801228e-02j, -0.02545466-4.69757450e-02j,  0.23575113-6.13720081e-02j, -0.29064723+8.84431226e-02j, -0.07433566+3.29740230e-02j,  0.07812021-1.02151861e-01j, -0.0425074 +1.72860999e-01j,  0.18038703-4.84824047e-02j,  0.06760615-1.08861841e-01j,  0.25746276+7.60469955e-02j, -0.01916844-4.17126464e-02j,  0.05801   +5.47708435e-02j, -0.01089018+7.64307991e-02j, -0.01047013+4.46021957e-01j,
        0.1335352 -7.65699052e-02j,  0.04482942-4.66701163e-02j,  0.078575  -6.73430309e-02j, -0.16287533+3.68253915e-02j, -0.00326996+8.45469473e-03j, -0.30822025+1.56452958e-01j,  0.05607748+2.71034262e-02j, -0.15156807-3.99754306e-02j, -0.11732098-2.62196417e-01j, -0.12910811+1.37840055e-01j, -0.11324172+4.51349322e-02j, -0.07238787+1.95625832e-01j,  0.20782018-1.53159222e-01j, -0.0029301 -2.05737302e-02j, -0.06307179-7.76953233e-02j,  0.09180174-3.94918551e-02j,  0.09905065+1.64050652e-01j,  0.02119278-1.28555941e-01j, -0.13011198+7.50039022e-02j,  0.00364899+1.56741679e-01j,  0.12967172-4.59481445e-02j,  0.04371742-9.22641848e-02j,  0.01217403+1.42468441e-01j, -0.03321802-6.06556050e-02j, -0.33791815+2.19614336e-02j, -0.02079545-1.62357371e-01j, -0.06115126-1.31463603e-01j, -0.0408845 -1.17685338e-01j, -0.00096192-1.80065774e-04j,  0.11813011+6.17214729e-03j,  0.00417459-8.29797298e-02j,  0.08729658+2.67707592e-01j, -0.01118896-8.32911057e-02j,  0.03707148-5.83134279e-02j,
       -0.12156735+2.64457587e-01j, -0.02989997-5.84706432e-02j,  0.03302162-6.01695667e-02j,  0.05437034-9.13677674e-02j,  0.10179674+1.40846743e-01j, -0.16635965+1.27949594e-01j,  0.1454272 -1.12363084e-01j, -0.11077774-1.22746452e-01j, -0.00185723-4.56720198e-02j,  0.02600582-8.89150489e-02j, -0.227494  -6.67592408e-02j,  0.08533135-1.88751082e-01j, -0.22714608+1.14661652e-01j, -0.02690036-1.30865882e-01j, -0.10121037-6.45046877e-02j,  0.01443838-2.37190078e-02j, -0.02303954-1.61468225e-01j,  0.06055609-2.63250332e-01j,  0.13332186+2.20700929e-01j, -0.02063736+2.65142722e-01j,  0.08332522+5.90342871e-03j,  0.1721038 -2.09054691e-01j, -0.15490146-9.27267743e-02j,  0.00453307-3.86153521e-02j, -0.02048122+3.02503499e-01j,  0.01458757-4.21067447e-02j,  0.32754022-3.88409256e-03j,  0.13811814-1.38612894e-01j, -0.12084009+3.73954159e-02j,  0.07102218-2.06344326e-02j,  0.11515796+1.13122542e-01j, -0.08863491-1.39456440e-01j, -0.17917237-4.99661020e-02j,  0.04442141+1.63664359e-01j,
        0.13732508-1.50992137e-01j, -0.05139775+8.97494329e-02j,  0.08732507+1.98929252e-01j,  0.05072904-2.99507910e-02j,  0.07108474+1.65205788e-02j,  0.0194683 -8.78868281e-02j,  0.09380987-4.44516495e-02j,  0.17168417+3.91109903e-02j, -0.04639245+2.60582960e-01j,  0.05875205+2.56753485e-01j, -0.07728994-3.17930589e-02j,  0.0452049 -1.76709840e-02j,  0.00134074-7.72090938e-02j,  0.20413611-1.94606554e-02j, -0.01910068+1.39711619e-01j,  0.0356499 +1.37434345e-01j,  0.30794216-1.09704366e-01j,  0.05284195-5.64409889e-02j, -0.08984104+6.94748250e-02j,  0.02163001+1.03288462e-01j,  0.03915384-9.81971938e-02j, -0.06670679+5.81276525e-02j, -0.11508668+9.47872135e-02j, -0.0354503 -3.13816097e-01j, -0.10991197+8.71705953e-02j,  0.06963248-5.65881100e-03j,  0.06577731-1.54696868e-01j,  0.15321403-6.74125684e-03j,  0.08071347+4.48458418e-01j,  0.03395331+1.27262600e-01j,  0.04183345+4.00904175e-03j, -0.01187089-2.21527903e-02j, -0.01851325-1.47271834e-02j, -0.23601169-8.54312072e-02j,
        0.11541862-1.04823473e-01j, -0.11268181-3.21825004e-02j, -0.09001661-1.37514659e-01j, -0.11748964-2.64871993e-02j, -0.0523941 -4.95790343e-02j,  0.1465224 -1.03971485e-01j,  0.18034942+7.79615744e-02j,  0.0936947 -2.22585205e-01j,  0.16751339+8.86578713e-02j,  0.19444547+1.21567778e-02j,  0.04771682+2.78729300e-02j,  0.19635398+9.50820346e-02j,  0.05185621+1.76230346e-01j,  0.07939533-2.18013091e-02j, -0.03596743-9.94674395e-03j, -0.28690236+7.06630196e-02j, -0.01620677+1.20756724e-01j, -0.11835929+3.00001593e-02j, -0.09989775+8.70560634e-02j,  0.0627832 -9.47387901e-02j, -0.09171898+2.82637561e-02j,  0.23849114+8.96858167e-02j, -0.24665889+2.03994686e-02j,  0.22245419-4.30604068e-04j, -0.0820849 -2.36258787e-01j, -0.11703521-6.52100265e-02j,  0.05282283-4.48185202e-02j,  0.02544553-7.56696642e-02j,  0.0320835 +2.09744844e-01j,  0.02707166+2.74207623e-01j,  0.14071592+1.51271647e-02j,  0.0599873 -2.12130726e-02j, -0.1323865 +1.32285712e-01j, -0.0634149 -1.88468438e-01j,
       -0.25236535-9.40009628e-02j,  0.05804886-1.24296266e-02j,  0.01463484-1.92411356e-01j,  0.02901968-1.95938408e-01j,  0.05900759-5.70542063e-02j,  0.0039289 +1.79168231e-01j,  0.21463779+6.06068544e-02j,  0.02601461+3.44313390e-02j, -0.06804777-1.37777019e-01j, -0.04134655-7.71930937e-02j,  0.26993183+7.04042029e-03j, -0.22478871-1.11219112e-01j,  0.02216198+8.72296592e-03j,  0.00900783-1.81402113e-01j, -0.12568783+5.40727822e-02j,  0.20728804+1.49611785e-01j, -0.1540311 -1.04748775e-01j,  0.11524429+1.78469618e-01j, -0.01784981-2.05987600e-01j, -0.04310959+6.72693611e-02j,  0.09204926+1.24870358e-01j,  0.23044143-1.67687891e-01j, -0.04829998+6.96651901e-03j, -0.09200799-1.69548082e-01j, -0.08599541-5.05360845e-02j,  0.05605567-1.23777989e-02j,  0.09183321+2.03716921e-01j,  0.06046671+1.88609777e-01j, -0.35443088-2.50139768e-01j, -0.10418443+1.53252364e-01j,  0.02878444+4.48922173e-02j, -0.07655359-8.92321321e-03j,  0.06322981-5.98952645e-03j, -0.05895737-7.06273994e-02j,
        0.045825  +3.45008534e-02j, -0.0356972 +7.58719989e-03j, -0.18135564+2.31658738e-02j, -0.11367649-1.11799943e-01j, -0.02278714-7.26386363e-03j,  0.00527786+1.70825935e-01j,  0.08659992-2.45702546e-02j, -0.16132867-1.40266073e-01j, -0.16275372+1.57528240e-01j,  0.10098732-4.90008453e-02j, -0.14020292-1.26389028e-01j, -0.13387933-1.77044519e-01j,  0.15233209+1.56396670e-01j, -0.01186549+1.66770551e-01j, -0.05911019-9.37506242e-02j, -0.11032036+2.22422316e-01j, -0.32768772+1.23945218e-01j,  0.11693884+1.01153011e-01j,  0.06380873+2.73797332e-02j, -0.15242003-1.78297868e-01j,  0.08576241-9.81537390e-02j, -0.05483774+2.39049552e-02j,  0.08818984-4.80048981e-02j,  0.13795202+3.39576952e-02j,  0.15420435+1.04809134e-01j, -0.1475717 -8.35910546e-02j,  0.25587669-9.78491595e-02j,  0.12582976+1.06377618e-01j, -0.04885733+5.85877129e-02j,  0.02768058-8.97097222e-02j, -0.09572117-3.14848747e-03j, -0.09107239-1.48042190e-01j,  0.02633147+1.28206292e-02j, -0.00231722+2.40935942e-01j,
        0.08505098+4.43364674e-02j,  0.0454237 -1.20532432e-01j, -0.17493652+1.27595262e-01j,  0.01816818-1.93108598e-01j, -0.17931339+1.06943524e-01j, -0.10671963+1.39014451e-05j,  0.11498691+1.93066834e-02j,  0.23000235+1.04393237e-01j, -0.18572715-1.37399401e-01j,  0.13220069-3.09402657e-02j,  0.02990116+1.12157118e-02j, -0.02942283-1.36235506e-03j, -0.13275055-2.01044204e-02j, -0.05513657-1.15044942e-01j, -0.07453253+1.24162429e-01j,  0.19348874+1.65304419e-01j, -0.01684765-8.28080868e-02j, -0.06982499+2.38660768e-02j, -0.29603023-1.04892178e-01j,  0.31520605-5.80965984e-02j, -0.16452607-3.29924630e-02j,  0.13918688+1.34255787e-01j,  0.1461848 -5.91517992e-02j,  0.05246915+2.47209477e-02j,  0.09600898+4.48951448e-02j, -0.03040775+2.25636252e-01j, -0.03104364-1.03378328e-02j,  0.12457642-1.13129265e-02j,  0.25722645-1.07390381e-01j,  0.12453095-1.46405362e-01j, -0.20675192+5.37923723e-02j, -0.08581718-3.76855478e-02j,  0.12881044-7.30139037e-02j,  0.0330913 -1.45171188e-02j,
        0.04964024-2.63320298e-01j, -0.06851158+3.25029578e-01j, -0.06736655+3.31741867e-02j,  0.10672767-1.33657818e-02j, -0.03911025+2.71696686e-03j,  0.18376597-2.21089267e-01j, -0.02949093-2.24178456e-02j, -0.07672718+8.63619859e-02j,  0.09011718+1.34863000e-01j, -0.03532004+2.01221708e-02j, -0.0175005 +6.62198244e-02j,  0.17146927+8.06205588e-02j, -0.09194709+3.45310559e-02j,  0.05919149+8.90830332e-03j, -0.03890834+6.41903709e-02j, -0.13501504-3.99845589e-02j, -0.10279042+1.03768681e-01j, -0.05259741-5.87714457e-02j, -0.11719418-2.08936502e-02j,  0.09836174-5.44608131e-02j, -0.22877539+2.08355637e-01j,  0.04941838-9.51260734e-02j, -0.06203396+2.28604981e-02j,  0.12098849-1.44576480e-02j, -0.08451115-1.43356466e-01j,  0.15635672-1.46530151e-02j, -0.20437704-3.79884112e-02j, -0.19636121-1.71501698e-02j,  0.07255917-2.20625252e-01j, -0.28061719+7.71279703e-02j, -0.02084913-1.12045005e-01j,  0.04322484+8.40757110e-03j,  0.06080093-9.41283647e-03j, -0.08052508-4.49212849e-03j,
        0.35296504-9.29569583e-02j,  0.30139399+2.02446885e-01j, -0.1043556 -1.29299775e-01j,  0.19156652+1.16499747e-02j,  0.20865974-1.46798336e-01j, -0.13729311-1.50569340e-02j,  0.02152623+1.61941611e-01j,  0.05963193-1.30770158e-01j,  0.06462617-1.66387108e-01j, -0.11403091-8.19783097e-02j, -0.17890047-3.87091987e-02j,  0.05041442+2.34797464e-02j,  0.33716094-2.18366246e-01j,  0.04749898+7.95121529e-02j, -0.14276611+5.91160893e-03j,  0.0727171 -6.38166930e-02j, -0.03268621+7.25536789e-02j, -0.00994355-5.45862875e-02j, -0.10977213+1.11554079e-01j,  0.01783962-2.78702484e-01j, -0.00306595+2.65697928e-02j,  0.1392672 -1.06298955e-01j, -0.10153158-8.24641251e-02j, -0.2693985 +6.47512729e-02j,  0.03236581+4.07523974e-02j, -0.14424473+2.51731413e-01j, -0.01873542-2.47536237e-02j, -0.01356796-8.16033886e-02j, -0.07272296-1.47884513e-01j,  0.01967111-1.46373935e-01j,  0.23073323-8.09684340e-02j,  0.14428689+1.18128043e-01j, -0.06420706+1.47110727e-01j, -0.014288  -1.01535529e-01j,
        0.05505723+1.85830802e-02j,  0.00835002+2.95374366e-02j,  0.00710951+4.40654128e-02j,  0.17926977-3.38549657e-01j]

        u1 = ql.Unitary("testname",matrix)
        u1.decompose()
        k.gate(u1, [0, 1, 2, 3, 4])

        p.add_kernel(k)
        p.compile()
        qx.set(os.path.join(output_dir, p.name+'.qasm'))
        qx.execute()
        c0 = qx.get_state()


        self.assertAlmostEqual(helper_prob(matrix[0]),   helper_regex(c0)[0], 5)
        self.assertAlmostEqual(helper_prob(matrix[32]),  helper_regex(c0)[1], 5)
        self.assertAlmostEqual(helper_prob(matrix[64]),  helper_regex(c0)[2], 5)
        self.assertAlmostEqual(helper_prob(matrix[96]),  helper_regex(c0)[3], 5)
        self.assertAlmostEqual(helper_prob(matrix[128]), helper_regex(c0)[4], 5)
        self.assertAlmostEqual(helper_prob(matrix[160]), helper_regex(c0)[5], 5)
        self.assertAlmostEqual(helper_prob(matrix[192]), helper_regex(c0)[6], 5)
        self.assertAlmostEqual(helper_prob(matrix[224]), helper_regex(c0)[7], 5)
        self.assertAlmostEqual(helper_prob(matrix[256]), helper_regex(c0)[8], 5)
        self.assertAlmostEqual(helper_prob(matrix[288]), helper_regex(c0)[9], 5)
        self.assertAlmostEqual(helper_prob(matrix[320]), helper_regex(c0)[10], 5)
        self.assertAlmostEqual(helper_prob(matrix[352]), helper_regex(c0)[11], 5)
        self.assertAlmostEqual(helper_prob(matrix[384]), helper_regex(c0)[12], 5)
        self.assertAlmostEqual(helper_prob(matrix[416]), helper_regex(c0)[13], 5)
        self.assertAlmostEqual(helper_prob(matrix[448]), helper_regex(c0)[14], 5)
        self.assertAlmostEqual(helper_prob(matrix[480]), helper_regex(c0)[15], 5)
        self.assertAlmostEqual(helper_prob(matrix[512]), helper_regex(c0)[16], 5)
        self.assertAlmostEqual(helper_prob(matrix[544]), helper_regex(c0)[17], 5)
        self.assertAlmostEqual(helper_prob(matrix[576]), helper_regex(c0)[18], 5)
        self.assertAlmostEqual(helper_prob(matrix[608]), helper_regex(c0)[19], 5)
        self.assertAlmostEqual(helper_prob(matrix[640]), helper_regex(c0)[20], 5)
        self.assertAlmostEqual(helper_prob(matrix[672]), helper_regex(c0)[21], 5)
        self.assertAlmostEqual(helper_prob(matrix[704]), helper_regex(c0)[22], 5)
        self.assertAlmostEqual(helper_prob(matrix[736]), helper_regex(c0)[23], 5)
        self.assertAlmostEqual(helper_prob(matrix[768]), helper_regex(c0)[24], 5)
        self.assertAlmostEqual(helper_prob(matrix[800]), helper_regex(c0)[25], 5)
        self.assertAlmostEqual(helper_prob(matrix[832]), helper_regex(c0)[26], 5)
        self.assertAlmostEqual(helper_prob(matrix[864]), helper_regex(c0)[27], 5)
        self.assertAlmostEqual(helper_prob(matrix[896]), helper_regex(c0)[28], 5)
        self.assertAlmostEqual(helper_prob(matrix[928]), helper_regex(c0)[29], 5)
        self.assertAlmostEqual(helper_prob(matrix[960]), helper_regex(c0)[30], 5)
        self.assertAlmostEqual(helper_prob(matrix[992]), helper_regex(c0)[31], 5)


    def test_usingqx_fullyentangled_5qubit_10011(self):
        num_qubits = 5
        p = ql.Program('test_usingqxfullentangled_5qubit_10011', platform, num_qubits)
        k = ql.Kernel('akernel', platform, num_qubits)

        matrix = [ 0.31869031-9.28734622e-02j,  0.38980148+2.14592427e-01j,  0.20279154-1.56580826e-01j,  0.00210273-2.83970875e-01j, -0.04010672-9.26812940e-02j,  0.15747049+6.08283291e-02j, -0.05274772-6.45225841e-02j,  0.05909439-3.23496026e-02j,  0.07868009+6.77298732e-02j,  0.11290142+2.48468763e-02j,  0.10092033-7.03646679e-02j, -0.03557972-3.39117261e-02j,  0.02241562+1.02714684e-02j, -0.04088796-1.12143173e-01j, -0.00144015+1.83925572e-02j, -0.1257948 -1.15394681e-01j,  0.13531631-1.84271421e-01j,  0.11293592-1.44170392e-01j, -0.06985893-1.36606289e-01j, -0.08924113-6.35923942e-02j,  0.19393965-6.35656579e-02j, -0.02153233-2.76955143e-01j, -0.12094999+1.05716669e-01j, -0.00624879+1.27471575e-01j, -0.13608981-1.22140261e-01j, -0.08243959-1.72734247e-02j,  0.06562052+1.12197102e-01j, -0.01102025-1.53811069e-01j,  0.13576338-8.95540026e-02j,  0.05730484-5.62973657e-02j, -0.08835493-4.13159024e-02j, -0.04533295+3.70144188e-02j,  0.04116657+1.58298973e-01j,  0.22270976-1.18654603e-01j,
        0.09219815+8.41433858e-02j, -0.0227499 +2.33343627e-01j, -0.13334752-6.98059663e-02j, -0.07967301-6.54413909e-02j, -0.09158972-7.03627564e-02j, -0.02332636-1.58060290e-02j,  0.23523954-3.48258461e-02j,  0.21329683+6.37364245e-02j,  0.25821394+1.30648709e-03j, -0.1118809 -1.95903237e-01j, -0.07887212-8.81857936e-02j,  0.047632  +9.40535593e-02j, -0.11726632+5.65815234e-02j,  0.08229469-4.38047420e-02j,  0.03117762+1.08469602e-01j,  0.16064616-1.30734886e-01j,  0.22181279-3.31211823e-02j,  0.07309884-2.01468425e-01j, -0.28437382+6.38292807e-02j, -0.23661479+1.17105208e-01j, -0.10436123-5.56443645e-02j,  0.20587544+1.99848797e-02j,  0.20705551+1.75496198e-02j,  0.01791984-8.31956939e-02j,  0.09737992+6.64713628e-03j,  0.0617566 -7.11998970e-02j,  0.03073603-7.25651496e-02j, -0.12056479-1.55358931e-01j, -0.09661589+4.90232280e-03j,  0.03110935-1.72748422e-01j,  0.01473304+3.62629583e-01j,  0.11031868-3.67435519e-03j,  0.04301239-1.64601498e-01j, -0.16432353+7.52258615e-02j,
       -0.00648176+6.66215752e-02j,  0.1257427 -3.71759618e-02j, -0.07793634-7.01586304e-02j,  0.0571156 +2.67410653e-01j, -0.00880782+2.09982981e-01j, -0.04961517-6.86953748e-02j, -0.15766986-1.12761472e-01j, -0.07076146-3.12372790e-02j, -0.08715236+9.29279186e-02j,  0.03843817+1.13545744e-01j, -0.10838483-1.19949631e-02j, -0.06342622+1.73231077e-01j,  0.23221532-4.28875040e-02j,  0.16514567-4.90633428e-02j, -0.0109654 +6.37891447e-02j,  0.02171279+5.89435085e-02j,  0.0199925 -2.96609865e-01j,  0.16702912+8.82996853e-02j,  0.19820571+5.01863737e-02j, -0.01237253-1.21196566e-01j,  0.11042294-2.16811518e-01j, -0.01004066-2.36556101e-01j, -0.1286729 -4.01294536e-03j, -0.11254789+5.56025311e-02j, -0.18328088-1.46524167e-02j, -0.13640243-3.71920766e-02j, -0.03793125+9.57786229e-02j,  0.07600542+1.53449546e-01j, -0.12371969+2.19599454e-01j,  0.18106889-3.10905810e-02j, -0.12698597+7.45431556e-02j,  0.02402161-1.66514384e-02j, -0.1431313 +4.63501164e-02j,  0.13656065-8.55748891e-02j,
       -0.2085481 -3.99831111e-02j, -0.05354994+8.14822771e-02j,  0.22188566+8.04753102e-02j, -0.09702996-1.54653045e-01j, -0.1018562 -1.01790676e-01j, -0.00335582+2.52475965e-01j, -0.04858119-1.52409323e-01j,  0.09335149-3.90325714e-02j,  0.08277598-4.08970043e-02j,  0.08119049+5.11051755e-02j, -0.01223217+6.17029648e-04j, -0.15137858-9.30742653e-03j,  0.1064706 +3.35870801e-02j, -0.04014048+1.33388615e-01j, -0.04315986+1.74367473e-01j, -0.04774817-9.82134162e-02j, -0.08203436+7.74038565e-02j, -0.2339137 +1.62306949e-01j,  0.11791971+2.08966161e-01j, -0.07867189-4.18549851e-02j, -0.17291904+9.47505092e-02j,  0.23630015-3.07786925e-01j, -0.09104365-1.33230919e-01j,  0.07679465+1.45726380e-01j,  0.09397243-9.26081447e-02j, -0.16092509+7.38633830e-02j, -0.02289017+1.59777790e-01j,  0.12053722+6.55620680e-02j,  0.12646833+1.04585433e-01j,  0.0264461 +1.92009889e-01j,  0.28749442+1.11327480e-01j,  0.15837267+6.12698488e-02j,  0.18181681-5.61278874e-02j,  0.07897084-1.17114924e-01j,
        0.19787494-1.67863968e-01j,  0.03852499-4.21534504e-02j, -0.14091989-1.27321872e-01j, -0.01747868-2.27419946e-01j, -0.02073576-6.24641146e-02j, -0.14241142+2.28697604e-02j,  0.15468346+2.08734157e-02j,  0.04505001-9.46333157e-02j, -0.09148317-2.26152985e-01j,  0.08577989+2.30863031e-01j, -0.08152502-8.74340592e-02j, -0.06465018-7.35108360e-02j, -0.21595512-8.66695180e-03j,  0.03304541-1.91886499e-01j,  0.04448112+3.67289455e-02j,  0.01969849-5.34777195e-02j, -0.03209367+3.02797601e-02j,  0.17389718+1.19098672e-01j, -0.10192559-3.02362398e-01j, -0.10224653-8.09241417e-02j, -0.12601597-6.86631211e-02j,  0.11275009+1.40915100e-01j,  0.03214418+1.48701215e-01j, -0.11571373+3.57083353e-02j,  0.03222285-5.19221441e-02j, -0.00250593+7.32186819e-02j, -0.15549573+1.77404851e-01j,  0.08568275+1.02973155e-01j,  0.11314133-2.13171498e-01j,  0.22748546-5.69951563e-02j,  0.14929197+1.31715414e-01j, -0.16301646+3.65045553e-03j,  0.13494125+2.31602145e-01j,  0.24267648-1.77780817e-01j,
       -0.00736974+7.72680290e-02j, -0.04867011+1.22605525e-01j,  0.05051748+8.06196677e-02j, -0.01749415+6.33486594e-02j,  0.06408343-4.51492484e-02j, -0.02543799-1.33253676e-01j, -0.05785745-4.71803669e-02j, -0.24358675-1.18083337e-02j,  0.00061111-5.96573111e-03j,  0.17487681-8.55625475e-02j,  0.13181372-5.47918839e-02j, -0.14072394+1.08475923e-01j,  0.05919593+1.78284217e-01j, -0.00292253-2.17168452e-02j,  0.19948189-1.95177980e-01j, -0.11146926-1.55684600e-01j,  0.02692391-9.18282632e-02j, -0.03804582-7.62499524e-02j,  0.18562866+1.41900953e-01j,  0.15468899+7.03401694e-03j,  0.10617502+2.96183301e-01j,  0.12124371+1.42279327e-02j, -0.12974516+1.87600027e-01j, -0.23270386+1.59618723e-01j, -0.14928652-8.27397154e-02j,  0.06670844+6.35681954e-02j,  0.03466266+1.89244901e-01j, -0.00961646-1.02903972e-01j,  0.19899344-2.38199195e-01j, -0.16594523-4.43067845e-02j,  0.27258801-7.49570862e-02j,  0.08182091+2.03334342e-02j, -0.02454051-4.82668158e-02j, -0.08557127+3.04552132e-02j,
        0.01303575+8.88057122e-02j,  0.13253621+5.05712004e-02j,  0.07763894+1.13841327e-01j, -0.28730914-4.49839630e-02j,  0.07031891-3.38973984e-03j, -0.04794523-1.67972335e-01j, -0.18102171+2.08760542e-01j, -0.08845631-9.70771934e-02j,  0.03546718+4.80846877e-02j, -0.08866794-5.19207245e-02j, -0.15099182-9.77070105e-02j, -0.05473901+2.33013978e-01j, -0.04645356-8.89489153e-02j,  0.07548625+1.80517821e-01j,  0.16840691+2.15268260e-01j, -0.09962074-7.35772706e-03j, -0.10104245+1.74017433e-02j, -0.09302095-1.10945928e-01j, -0.035157  +2.50873519e-02j,  0.10622507+1.29398421e-01j, -0.20965829-4.55862887e-02j,  0.05220909+6.66834897e-02j, -0.09672108+1.09403436e-01j,  0.12697788-6.19212188e-02j,  0.05729917+1.44378294e-01j, -0.15808366-3.82581632e-02j,  0.10716098-4.62942406e-02j,  0.24961573+1.32840816e-01j,  0.0627808 +2.39056706e-02j,  0.18311487+7.98582936e-03j,  0.02661541+7.40701198e-02j,  0.04699528+1.88767961e-01j,  0.04750332-2.25995642e-02j,  0.07193907+5.68042859e-02j,
       -0.02763382-3.43370436e-02j,  0.11205088-1.98370084e-02j, -0.11424545+1.14269285e-03j,  0.06885854-2.09915302e-01j, -0.05625496-2.88701666e-01j, -0.08183393+2.31774294e-01j, -0.09301037-4.03842722e-02j, -0.06069309+2.57502142e-02j, -0.18634877+9.85377377e-02j,  0.01474395+9.22414287e-02j,  0.04147054-1.74520016e-01j,  0.06794102-1.59508183e-01j, -0.13292245-5.38490513e-02j, -0.18753702+2.82751900e-01j,  0.00899866-2.80951474e-01j,  0.20377748+8.19207845e-02j, -0.14028603-1.39624056e-01j,  0.10741114+2.60308736e-02j, -0.22833573+6.02730469e-02j,  0.16071761-1.25615817e-01j,  0.08060865-5.81044643e-03j,  0.16141495-4.86994019e-02j,  0.11263654-1.87107794e-02j,  0.06859488-1.90280093e-01j,  0.10733862+4.45380867e-02j, -0.11489398-2.10578670e-01j, -0.0864833 -1.73958604e-01j,  0.12293904-1.50703768e-01j, -0.00629652+2.98133822e-02j,  0.16452648+7.76415767e-02j,  0.16219009+2.46017766e-01j, -0.04582374-1.16806045e-01j,  0.15109013+9.45671154e-03j,  0.04773975-1.54745106e-01j,
        0.21822796-1.87975523e-03j,  0.16596711-1.31539539e-01j,  0.25621724-4.31460990e-02j,  0.23071937+1.38086347e-01j,  0.10615413-1.24941186e-01j,  0.00343902+5.79927212e-02j,  0.10569282-2.64952183e-01j, -0.13601637+5.39259774e-02j, -0.13182901+9.15307749e-02j,  0.03524551-1.35956850e-01j,  0.0957622 -1.71837228e-02j, -0.03707841-5.40514827e-02j, -0.05729147-7.33164458e-02j, -0.18631267+1.00759871e-01j, -0.02434746-2.22824245e-02j, -0.02570765-3.90074186e-02j,  0.17946901+1.86288095e-02j,  0.07742897+3.19064602e-02j, -0.17538437+2.18666227e-01j,  0.10029635-8.92554711e-03j, -0.13379504-1.03683797e-02j, -0.11115363-4.48983441e-02j,  0.00163172-2.05325390e-02j,  0.02753729-1.28591197e-01j,  0.01933548+2.37393316e-01j,  0.07102201-1.19729329e-01j, -0.07517562+7.47947883e-02j, -0.03383077-1.23796732e-01j, -0.05575669-2.97673975e-02j, -0.04860924+1.02986531e-01j,  0.11384071+2.36429953e-01j, -0.26355793+1.30580300e-02j, -0.2665541 +6.72995742e-02j, -0.04663019-5.64857364e-03j,
        0.15703188+2.25734925e-02j, -0.02611337-4.94904084e-03j,  0.31897764+7.13870567e-02j,  0.15501272-1.48107261e-01j,  0.18034006-2.37497106e-01j,  0.21350438-8.08313917e-02j,  0.0620688 +5.98457145e-02j, -0.04996277+1.09069384e-02j, -0.09282843-5.59942293e-02j, -0.05363359+4.83839918e-02j, -0.20946404-1.85419524e-01j, -0.05481421+8.78420662e-02j, -0.19261214-1.04875021e-01j, -0.00083685-2.70091261e-02j, -0.17618069+8.47803615e-02j,  0.16055652+5.98821538e-03j, -0.0635621 +8.16228775e-02j, -0.22507829-4.14268948e-02j, -0.00791737+7.40481825e-02j,  0.19970347-1.15972939e-01j, -0.08619572+1.19678540e-01j, -0.184931  -1.13524848e-01j,  0.04824348-5.85668978e-02j, -0.22446284+1.11472258e-02j,  0.09629746-1.88788542e-01j, -0.18228537-3.89615785e-04j,  0.09084756+4.78058679e-02j, -0.00889583-1.74554680e-01j, -0.07447697-4.48823321e-02j, -0.05076077-6.24206002e-02j, -0.14050658+8.38982646e-02j,  0.07428934-6.30440232e-02j,  0.05701074-1.00819168e-01j,  0.07009481+8.11789378e-02j,
       -0.01155395+2.46650660e-01j,  0.25264038-1.98419236e-01j, -0.09155521-2.79616998e-02j,  0.06750545-1.52831769e-01j,  0.1068504 -1.78545409e-01j, -0.06203486+2.86380485e-02j,  0.23067545-1.23638547e-01j, -0.02397582+2.15643846e-01j,  0.01810979+1.08067552e-01j,  0.09130261-4.68850866e-02j,  0.13093158-1.51875212e-01j,  0.25207024+5.80216347e-02j,  0.02600999-1.27799785e-01j,  0.03509714-5.46256940e-02j,  0.04362407-3.07563727e-02j,  0.12488249+6.70881126e-02j,  0.05539679+1.15790184e-01j,  0.23107124-9.66168589e-02j, -0.09977799-9.88045601e-02j, -0.05633534+2.79762650e-01j, -0.02304281-2.42282613e-02j,  0.1276518 -2.32550502e-02j,  0.25634814-3.02429796e-02j,  0.30827737-2.19288542e-01j, -0.02512791+2.13783374e-01j,  0.15453741-7.04163688e-02j, -0.21441715+2.05959415e-01j, -0.02827968+4.71161636e-02j, -0.03837343+6.70595235e-02j, -0.07854483-3.12796395e-02j, -0.03615927+1.10733671e-01j,  0.21779074-1.39489256e-01j,  0.06619192+5.38352432e-02j, -0.0680216 -1.05456770e-01j,
       -0.10316128+1.32947183e-01j, -0.06345699-1.85555087e-01j, -0.00893067+1.52852423e-01j,  0.03135864+1.10423220e-01j, -0.182351  +2.77954961e-02j, -0.09252137+1.09061091e-01j, -0.0797695 +1.15176285e-02j,  0.05051009+2.57152535e-01j,  0.09344185-1.12906975e-01j,  0.04587659+8.90786692e-02j, -0.02471718+4.53344925e-02j, -0.08880236+3.62673481e-02j, -0.05981018+9.78912466e-02j, -0.00118743-5.07702805e-02j, -0.08119522+5.16043940e-02j,  0.11854883+9.79110177e-02j,  0.28543526-8.57540624e-02j,  0.28069514+2.00242562e-01j, -0.01036913-1.12897546e-01j, -0.08470073+1.91009156e-02j,  0.01428585+7.92620224e-02j, -0.15567322-4.85322282e-02j,  0.14203257-2.97248067e-01j,  0.15075682-1.33294241e-01j,  0.08107944-6.10561974e-02j,  0.07558358-3.29486035e-02j, -0.04042877-2.49142675e-02j,  0.05557881-1.93956804e-01j, -0.00589446+9.44678255e-02j,  0.1653676 -4.08479951e-02j,  0.14382039+1.57269491e-02j, -0.22311804-8.79699173e-02j,  0.09825566+1.52084444e-02j,  0.00938452-3.15649024e-01j,
       -0.03982978+1.22637508e-01j, -0.15670677-1.93426126e-01j,  0.22741559-4.39643322e-02j, -0.13362661-9.94353174e-02j, -0.09445278+4.93195562e-02j, -0.04684833-8.59308253e-02j,  0.08890091-1.16876390e-01j, -0.18726614+7.50813913e-02j, -0.01806267-2.00321404e-02j,  0.09338316-1.69711164e-01j, -0.03114173+1.43744729e-01j, -0.01515641+7.72217128e-02j,  0.21301431+5.92330407e-02j,  0.12250555+1.16784140e-01j,  0.0492589 +1.82225369e-01j,  0.09296806+1.58772507e-02j, -0.06700279+7.86869873e-02j, -0.02389099-5.36170682e-02j,  0.00545649-1.25491880e-03j,  0.09228983+2.64561438e-01j, -0.08125859-3.27415023e-02j, -0.03769576-2.64763488e-01j,  0.0914594 +1.90133579e-01j, -0.27352411+6.32203214e-02j, -0.00087467-8.61094525e-02j,  0.13735927+8.73538830e-03j, -0.08484748+1.68213884e-01j, -0.07040327-1.76254392e-01j, -0.08363638-8.30902369e-02j,  0.14643814+6.63711322e-02j, -0.31659998-1.04317904e-01j,  0.07830602+2.93183609e-02j,  0.11041879+1.50347028e-01j, -0.00433818-1.72946523e-01j,
       -0.15422074+9.38342826e-02j, -0.14194574+3.64105744e-02j,  0.06297505+7.88548664e-02j,  0.01106598-2.65402837e-01j,  0.07164956-8.12953528e-02j, -0.12514039-1.84403896e-01j, -0.17295559+3.58171829e-02j, -0.03655755-7.10559195e-02j, -0.03821694-1.10973040e-01j,  0.18546141+1.10662839e-01j, -0.10195195+1.32367694e-01j, -0.05016084+2.78628720e-02j, -0.19079393+1.40041256e-02j,  0.07878861-3.38112621e-02j,  0.13730204+1.52897765e-01j,  0.11815899+3.49910958e-02j, -0.02648579-1.03280312e-01j,  0.11970226-2.50844123e-01j,  0.10406627-9.12689026e-02j,  0.07247411-2.51491332e-01j,  0.06550999-2.94078974e-01j, -0.12079469-5.00355065e-02j,  0.06000345-1.02467349e-01j,  0.08823557+3.66136284e-02j,  0.02291526+3.63437115e-02j, -0.07124433+2.31676270e-01j,  0.17493588+5.98421039e-02j, -0.17824122-1.97944096e-02j, -0.00491994-9.10500757e-02j,  0.19633245+6.47815099e-02j, -0.03770238-7.48581318e-02j, -0.18402244+9.04322714e-02j, -0.2842845 -1.40285725e-01j,  0.00138785+1.08042021e-01j,
        0.16088797+2.75183648e-01j, -0.0865634 -7.36993417e-02j,  0.05178655+5.29576072e-02j, -0.05123872-1.29090313e-02j, -0.03079395-1.69557846e-01j,  0.23219467-7.71729365e-03j, -0.01135984-6.49120093e-02j,  0.08717993-9.71885487e-02j, -0.02043387+1.16518568e-01j, -0.08256357+1.23302259e-02j,  0.13742387-2.00583302e-01j,  0.11087765+2.22213806e-02j,  0.10703254+3.53681589e-02j, -0.04998617+7.13667998e-02j, -0.2463513 -2.30198030e-01j,  0.09831487+1.92709068e-01j,  0.10999549+1.28740664e-01j, -0.04056669+9.08016920e-02j, -0.03342728-9.92407179e-02j,  0.09352449-4.93911914e-02j, -0.11841153-8.25481366e-02j, -0.0263205 +1.91245810e-01j, -0.15521225-6.80613837e-02j,  0.18223827-2.68908129e-01j,  0.0411945 +1.12072453e-01j, -0.02954085+6.34315408e-02j, -0.01019486-1.24389083e-01j,  0.19174201-7.05812742e-02j,  0.19351232+1.42610666e-02j,  0.12937332-1.45006522e-01j,  0.01111355+1.11974478e-01j,  0.25163849+7.58955475e-02j,  0.06967535+1.51710939e-01j, -0.26064587+5.85797495e-02j,
       -0.05364486-8.37537594e-02j,  0.02473011+1.57624180e-01j,  0.0788524 +4.24067733e-02j, -0.05353458-3.59527188e-02j,  0.24630053-3.09146961e-01j,  0.12673895+2.54501080e-02j,  0.13328446+1.27420821e-01j, -0.03311185+8.64376474e-02j, -0.10126181-3.33404906e-02j,  0.05819512-2.42452607e-01j,  0.12966928+1.92345459e-01j, -0.10237172+7.87217786e-02j, -0.12008   +8.02686485e-02j,  0.11309599+3.47582961e-02j,  0.16921731-6.49645604e-02j,  0.10847358+1.32814258e-01j,  0.09620142+8.29342007e-02j, -0.03836254+1.18815716e-01j,  0.10334279-1.21404737e-02j, -0.19366239-5.73423731e-02j,  0.11445103-7.69154143e-02j,  0.10345336+3.42165082e-02j,  0.0870257 +5.06977361e-02j, -0.0262899 +4.49013808e-02j, -0.0367388 +1.36360059e-02j,  0.00976608-1.86860241e-01j,  0.15386709+1.46930831e-01j,  0.11249562+2.57278918e-02j,  0.08496388-1.11776874e-01j, -0.07162906-1.41500632e-02j,  0.076649  -2.72671417e-01j,  0.12029408-2.43316647e-01j,  0.29672048-6.39702972e-02j,  0.1939962 -1.55753156e-02j,
        0.08677801+1.96162231e-01j,  0.1237203 -9.61956522e-02j, -0.16119964+3.89474786e-02j,  0.09207276-2.54823411e-01j,  0.03475803-5.50657361e-02j, -0.21263069-7.05923500e-02j, -0.06092331+4.49331302e-02j, -0.02461632+3.39504695e-02j,  0.0345771 +2.39801010e-01j,  0.00891307+1.12739196e-01j,  0.13060822-1.11694609e-01j,  0.09803224+1.73134288e-01j, -0.07234639+1.28840723e-02j,  0.11471256+3.77081748e-02j,  0.26422031-1.58104940e-01j,  0.02654836-1.38859272e-01j, -0.04278432-1.46473478e-02j,  0.28242235-5.27977137e-02j, -0.13193171+1.21081065e-01j,  0.06133633-4.22267064e-02j, -0.08280452+2.04720842e-02j,  0.02083153+5.42313877e-02j,  0.12016721+1.60501416e-01j, -0.12225807-2.76668735e-01j,  0.02102305+9.09998231e-02j,  0.11747776+3.62319311e-01j,  0.09912628+1.50849159e-02j, -0.15640258+2.48010133e-02j,  0.0546514 +2.59861755e-02j, -0.01002552+6.65787181e-02j, -0.02898995+9.89915581e-02j, -0.00717829-1.37245312e-01j,  0.00055712+1.86303843e-01j,  0.04400996+1.19535156e-01j,
        0.05636026-2.16779961e-01j, -0.05913502-2.77370485e-01j,  0.20737913+2.02848948e-01j, -0.07528717+7.93840780e-02j,  0.05451348+1.33425914e-01j, -0.12290648+7.58881312e-02j,  0.10321466+1.24983234e-01j,  0.13371569-1.00033395e-01j, -0.05791197+2.68575587e-02j,  0.04652659+4.60658211e-02j,  0.00131806+5.92140285e-02j, -0.13481008+2.31672893e-02j, -0.13689412+1.17954022e-01j,  0.04998811-2.02833835e-02j, -0.14165519+3.56756720e-01j, -0.07856852+6.35118506e-02j,  0.05564543-9.74922249e-02j, -0.21743105-5.41409515e-02j, -0.16217875+7.97524864e-02j, -0.19486651-9.99801439e-02j,  0.19124263-4.71432152e-02j,  0.01035372+4.63843919e-02j, -0.07218978+1.32977797e-01j, -0.21002818-1.31679835e-01j,  0.05305256-1.54340691e-02j, -0.19401635+6.69582303e-02j,  0.05139607+1.93092981e-01j, -0.09946416+4.37566125e-02j,  0.10192397-4.16617603e-02j, -0.12405403-9.78811063e-02j,  0.05812519-3.95372746e-05j,  0.00099002-1.14739425e-02j,  0.05247123-8.40118490e-02j,  0.04501583+1.44968196e-01j,
        0.03301905-4.80048648e-02j,  0.02823323-1.78191318e-02j,  0.08196271-6.89791351e-03j,  0.02038618+7.47434818e-02j, -0.0372285 +4.85572384e-02j,  0.03293172+1.46606707e-02j, -0.18339563+6.37443359e-02j, -0.11214925-1.13505125e-01j, -0.06063048+1.45448526e-01j,  0.34233663-3.24467430e-01j,  0.07664073-1.44247607e-01j, -0.28993637+1.68069982e-01j, -0.18237161+2.05640159e-01j,  0.05552109+1.44200044e-01j, -0.08757971-2.66443942e-01j,  0.1558971 +2.49096061e-02j, -0.16421838+3.76276364e-02j,  0.08841799+3.97688126e-02j,  0.06127403+8.79601343e-02j, -0.11912802+9.38176361e-02j, -0.01512363+1.98779455e-01j, -0.10425168-2.89846412e-02j,  0.16046822-1.77676457e-02j,  0.04871864-4.15742404e-02j,  0.19325653-1.69328440e-01j, -0.059617  +1.87459639e-02j, -0.23497369+7.78523444e-02j,  0.03611522+4.96031106e-02j,  0.02849174+1.81422119e-01j, -0.13379238-1.46097376e-01j,  0.13609017-3.86016200e-02j, -0.13135849-9.04921922e-02j,  0.1333314 +1.02431026e-01j,  0.23331259-1.23168916e-02j,
        0.05106233+7.45228667e-02j,  0.12189655+6.42850380e-02j,  0.04943332+2.95462475e-02j, -0.00657986+8.06994964e-02j,  0.3267687 +8.99736406e-02j, -0.14260504+1.75648292e-01j,  0.23063331+1.78219849e-01j,  0.10883261+1.28389828e-01j, -0.10597288-1.40386755e-01j,  0.01512138+5.26532655e-02j, -0.24270928-2.39424278e-01j,  0.08582485+1.81061208e-01j,  0.15870471+3.82006536e-03j, -0.14205944-7.88677962e-02j,  0.26780226+1.17371947e-01j, -0.05734894+1.19255894e-01j, -0.00708958-3.87661515e-02j, -0.0123964 +1.17519163e-01j, -0.05307584+9.65095189e-02j, -0.04746522+6.19590573e-02j, -0.03677784-2.76400184e-02j,  0.14375332+9.66259420e-02j, -0.09175759-1.16174795e-01j, -0.03764456-1.92660525e-02j, -0.14188024+1.51742465e-01j, -0.00855281-6.80432250e-02j, -0.13963677+6.77341017e-02j,  0.27226725-5.95170973e-03j,  0.07653268+6.86308978e-02j, -0.18769106+2.15711501e-01j,  0.16136537-4.42590061e-02j, -0.26360596+1.04686372e-01j, -0.09155045-8.77382640e-02j, -0.09151231+4.28425508e-02j,
       -0.06373803+3.71389234e-03j,  0.12982765+1.93272289e-01j, -0.00676779+9.13731987e-02j, -0.15669531-1.03446811e-01j,  0.13876435+1.73790050e-01j,  0.03724986-6.80542715e-02j,  0.13536485+2.06818477e-02j,  0.2134622 +7.76124343e-02j, -0.21930729-1.35324670e-02j, -0.15870289-1.47418944e-01j, -0.10525782-3.21051509e-02j,  0.02651126-7.35636015e-02j,  0.21299852-4.88944015e-02j,  0.1520081 +1.18419448e-01j,  0.02837817-1.66734491e-02j, -0.05583337+7.15315919e-02j,  0.00827433-9.71346050e-02j, -0.10828369+1.50029916e-01j, -0.05569338+1.56736408e-01j, -0.10498402-3.01603733e-02j,  0.01972677-3.28801228e-02j, -0.02545466-4.69757450e-02j,  0.23575113-6.13720081e-02j, -0.29064723+8.84431226e-02j, -0.07433566+3.29740230e-02j,  0.07812021-1.02151861e-01j, -0.0425074 +1.72860999e-01j,  0.18038703-4.84824047e-02j,  0.06760615-1.08861841e-01j,  0.25746276+7.60469955e-02j, -0.01916844-4.17126464e-02j,  0.05801   +5.47708435e-02j, -0.01089018+7.64307991e-02j, -0.01047013+4.46021957e-01j,
        0.1335352 -7.65699052e-02j,  0.04482942-4.66701163e-02j,  0.078575  -6.73430309e-02j, -0.16287533+3.68253915e-02j, -0.00326996+8.45469473e-03j, -0.30822025+1.56452958e-01j,  0.05607748+2.71034262e-02j, -0.15156807-3.99754306e-02j, -0.11732098-2.62196417e-01j, -0.12910811+1.37840055e-01j, -0.11324172+4.51349322e-02j, -0.07238787+1.95625832e-01j,  0.20782018-1.53159222e-01j, -0.0029301 -2.05737302e-02j, -0.06307179-7.76953233e-02j,  0.09180174-3.94918551e-02j,  0.09905065+1.64050652e-01j,  0.02119278-1.28555941e-01j, -0.13011198+7.50039022e-02j,  0.00364899+1.56741679e-01j,  0.12967172-4.59481445e-02j,  0.04371742-9.22641848e-02j,  0.01217403+1.42468441e-01j, -0.03321802-6.06556050e-02j, -0.33791815+2.19614336e-02j, -0.02079545-1.62357371e-01j, -0.06115126-1.31463603e-01j, -0.0408845 -1.17685338e-01j, -0.00096192-1.80065774e-04j,  0.11813011+6.17214729e-03j,  0.00417459-8.29797298e-02j,  0.08729658+2.67707592e-01j, -0.01118896-8.32911057e-02j,  0.03707148-5.83134279e-02j,
       -0.12156735+2.64457587e-01j, -0.02989997-5.84706432e-02j,  0.03302162-6.01695667e-02j,  0.05437034-9.13677674e-02j,  0.10179674+1.40846743e-01j, -0.16635965+1.27949594e-01j,  0.1454272 -1.12363084e-01j, -0.11077774-1.22746452e-01j, -0.00185723-4.56720198e-02j,  0.02600582-8.89150489e-02j, -0.227494  -6.67592408e-02j,  0.08533135-1.88751082e-01j, -0.22714608+1.14661652e-01j, -0.02690036-1.30865882e-01j, -0.10121037-6.45046877e-02j,  0.01443838-2.37190078e-02j, -0.02303954-1.61468225e-01j,  0.06055609-2.63250332e-01j,  0.13332186+2.20700929e-01j, -0.02063736+2.65142722e-01j,  0.08332522+5.90342871e-03j,  0.1721038 -2.09054691e-01j, -0.15490146-9.27267743e-02j,  0.00453307-3.86153521e-02j, -0.02048122+3.02503499e-01j,  0.01458757-4.21067447e-02j,  0.32754022-3.88409256e-03j,  0.13811814-1.38612894e-01j, -0.12084009+3.73954159e-02j,  0.07102218-2.06344326e-02j,  0.11515796+1.13122542e-01j, -0.08863491-1.39456440e-01j, -0.17917237-4.99661020e-02j,  0.04442141+1.63664359e-01j,
        0.13732508-1.50992137e-01j, -0.05139775+8.97494329e-02j,  0.08732507+1.98929252e-01j,  0.05072904-2.99507910e-02j,  0.07108474+1.65205788e-02j,  0.0194683 -8.78868281e-02j,  0.09380987-4.44516495e-02j,  0.17168417+3.91109903e-02j, -0.04639245+2.60582960e-01j,  0.05875205+2.56753485e-01j, -0.07728994-3.17930589e-02j,  0.0452049 -1.76709840e-02j,  0.00134074-7.72090938e-02j,  0.20413611-1.94606554e-02j, -0.01910068+1.39711619e-01j,  0.0356499 +1.37434345e-01j,  0.30794216-1.09704366e-01j,  0.05284195-5.64409889e-02j, -0.08984104+6.94748250e-02j,  0.02163001+1.03288462e-01j,  0.03915384-9.81971938e-02j, -0.06670679+5.81276525e-02j, -0.11508668+9.47872135e-02j, -0.0354503 -3.13816097e-01j, -0.10991197+8.71705953e-02j,  0.06963248-5.65881100e-03j,  0.06577731-1.54696868e-01j,  0.15321403-6.74125684e-03j,  0.08071347+4.48458418e-01j,  0.03395331+1.27262600e-01j,  0.04183345+4.00904175e-03j, -0.01187089-2.21527903e-02j, -0.01851325-1.47271834e-02j, -0.23601169-8.54312072e-02j,
        0.11541862-1.04823473e-01j, -0.11268181-3.21825004e-02j, -0.09001661-1.37514659e-01j, -0.11748964-2.64871993e-02j, -0.0523941 -4.95790343e-02j,  0.1465224 -1.03971485e-01j,  0.18034942+7.79615744e-02j,  0.0936947 -2.22585205e-01j,  0.16751339+8.86578713e-02j,  0.19444547+1.21567778e-02j,  0.04771682+2.78729300e-02j,  0.19635398+9.50820346e-02j,  0.05185621+1.76230346e-01j,  0.07939533-2.18013091e-02j, -0.03596743-9.94674395e-03j, -0.28690236+7.06630196e-02j, -0.01620677+1.20756724e-01j, -0.11835929+3.00001593e-02j, -0.09989775+8.70560634e-02j,  0.0627832 -9.47387901e-02j, -0.09171898+2.82637561e-02j,  0.23849114+8.96858167e-02j, -0.24665889+2.03994686e-02j,  0.22245419-4.30604068e-04j, -0.0820849 -2.36258787e-01j, -0.11703521-6.52100265e-02j,  0.05282283-4.48185202e-02j,  0.02544553-7.56696642e-02j,  0.0320835 +2.09744844e-01j,  0.02707166+2.74207623e-01j,  0.14071592+1.51271647e-02j,  0.0599873 -2.12130726e-02j, -0.1323865 +1.32285712e-01j, -0.0634149 -1.88468438e-01j,
       -0.25236535-9.40009628e-02j,  0.05804886-1.24296266e-02j,  0.01463484-1.92411356e-01j,  0.02901968-1.95938408e-01j,  0.05900759-5.70542063e-02j,  0.0039289 +1.79168231e-01j,  0.21463779+6.06068544e-02j,  0.02601461+3.44313390e-02j, -0.06804777-1.37777019e-01j, -0.04134655-7.71930937e-02j,  0.26993183+7.04042029e-03j, -0.22478871-1.11219112e-01j,  0.02216198+8.72296592e-03j,  0.00900783-1.81402113e-01j, -0.12568783+5.40727822e-02j,  0.20728804+1.49611785e-01j, -0.1540311 -1.04748775e-01j,  0.11524429+1.78469618e-01j, -0.01784981-2.05987600e-01j, -0.04310959+6.72693611e-02j,  0.09204926+1.24870358e-01j,  0.23044143-1.67687891e-01j, -0.04829998+6.96651901e-03j, -0.09200799-1.69548082e-01j, -0.08599541-5.05360845e-02j,  0.05605567-1.23777989e-02j,  0.09183321+2.03716921e-01j,  0.06046671+1.88609777e-01j, -0.35443088-2.50139768e-01j, -0.10418443+1.53252364e-01j,  0.02878444+4.48922173e-02j, -0.07655359-8.92321321e-03j,  0.06322981-5.98952645e-03j, -0.05895737-7.06273994e-02j,
        0.045825  +3.45008534e-02j, -0.0356972 +7.58719989e-03j, -0.18135564+2.31658738e-02j, -0.11367649-1.11799943e-01j, -0.02278714-7.26386363e-03j,  0.00527786+1.70825935e-01j,  0.08659992-2.45702546e-02j, -0.16132867-1.40266073e-01j, -0.16275372+1.57528240e-01j,  0.10098732-4.90008453e-02j, -0.14020292-1.26389028e-01j, -0.13387933-1.77044519e-01j,  0.15233209+1.56396670e-01j, -0.01186549+1.66770551e-01j, -0.05911019-9.37506242e-02j, -0.11032036+2.22422316e-01j, -0.32768772+1.23945218e-01j,  0.11693884+1.01153011e-01j,  0.06380873+2.73797332e-02j, -0.15242003-1.78297868e-01j,  0.08576241-9.81537390e-02j, -0.05483774+2.39049552e-02j,  0.08818984-4.80048981e-02j,  0.13795202+3.39576952e-02j,  0.15420435+1.04809134e-01j, -0.1475717 -8.35910546e-02j,  0.25587669-9.78491595e-02j,  0.12582976+1.06377618e-01j, -0.04885733+5.85877129e-02j,  0.02768058-8.97097222e-02j, -0.09572117-3.14848747e-03j, -0.09107239-1.48042190e-01j,  0.02633147+1.28206292e-02j, -0.00231722+2.40935942e-01j,
        0.08505098+4.43364674e-02j,  0.0454237 -1.20532432e-01j, -0.17493652+1.27595262e-01j,  0.01816818-1.93108598e-01j, -0.17931339+1.06943524e-01j, -0.10671963+1.39014451e-05j,  0.11498691+1.93066834e-02j,  0.23000235+1.04393237e-01j, -0.18572715-1.37399401e-01j,  0.13220069-3.09402657e-02j,  0.02990116+1.12157118e-02j, -0.02942283-1.36235506e-03j, -0.13275055-2.01044204e-02j, -0.05513657-1.15044942e-01j, -0.07453253+1.24162429e-01j,  0.19348874+1.65304419e-01j, -0.01684765-8.28080868e-02j, -0.06982499+2.38660768e-02j, -0.29603023-1.04892178e-01j,  0.31520605-5.80965984e-02j, -0.16452607-3.29924630e-02j,  0.13918688+1.34255787e-01j,  0.1461848 -5.91517992e-02j,  0.05246915+2.47209477e-02j,  0.09600898+4.48951448e-02j, -0.03040775+2.25636252e-01j, -0.03104364-1.03378328e-02j,  0.12457642-1.13129265e-02j,  0.25722645-1.07390381e-01j,  0.12453095-1.46405362e-01j, -0.20675192+5.37923723e-02j, -0.08581718-3.76855478e-02j,  0.12881044-7.30139037e-02j,  0.0330913 -1.45171188e-02j,
        0.04964024-2.63320298e-01j, -0.06851158+3.25029578e-01j, -0.06736655+3.31741867e-02j,  0.10672767-1.33657818e-02j, -0.03911025+2.71696686e-03j,  0.18376597-2.21089267e-01j, -0.02949093-2.24178456e-02j, -0.07672718+8.63619859e-02j,  0.09011718+1.34863000e-01j, -0.03532004+2.01221708e-02j, -0.0175005 +6.62198244e-02j,  0.17146927+8.06205588e-02j, -0.09194709+3.45310559e-02j,  0.05919149+8.90830332e-03j, -0.03890834+6.41903709e-02j, -0.13501504-3.99845589e-02j, -0.10279042+1.03768681e-01j, -0.05259741-5.87714457e-02j, -0.11719418-2.08936502e-02j,  0.09836174-5.44608131e-02j, -0.22877539+2.08355637e-01j,  0.04941838-9.51260734e-02j, -0.06203396+2.28604981e-02j,  0.12098849-1.44576480e-02j, -0.08451115-1.43356466e-01j,  0.15635672-1.46530151e-02j, -0.20437704-3.79884112e-02j, -0.19636121-1.71501698e-02j,  0.07255917-2.20625252e-01j, -0.28061719+7.71279703e-02j, -0.02084913-1.12045005e-01j,  0.04322484+8.40757110e-03j,  0.06080093-9.41283647e-03j, -0.08052508-4.49212849e-03j,
        0.35296504-9.29569583e-02j,  0.30139399+2.02446885e-01j, -0.1043556 -1.29299775e-01j,  0.19156652+1.16499747e-02j,  0.20865974-1.46798336e-01j, -0.13729311-1.50569340e-02j,  0.02152623+1.61941611e-01j,  0.05963193-1.30770158e-01j,  0.06462617-1.66387108e-01j, -0.11403091-8.19783097e-02j, -0.17890047-3.87091987e-02j,  0.05041442+2.34797464e-02j,  0.33716094-2.18366246e-01j,  0.04749898+7.95121529e-02j, -0.14276611+5.91160893e-03j,  0.0727171 -6.38166930e-02j, -0.03268621+7.25536789e-02j, -0.00994355-5.45862875e-02j, -0.10977213+1.11554079e-01j,  0.01783962-2.78702484e-01j, -0.00306595+2.65697928e-02j,  0.1392672 -1.06298955e-01j, -0.10153158-8.24641251e-02j, -0.2693985 +6.47512729e-02j,  0.03236581+4.07523974e-02j, -0.14424473+2.51731413e-01j, -0.01873542-2.47536237e-02j, -0.01356796-8.16033886e-02j, -0.07272296-1.47884513e-01j,  0.01967111-1.46373935e-01j,  0.23073323-8.09684340e-02j,  0.14428689+1.18128043e-01j, -0.06420706+1.47110727e-01j, -0.014288  -1.01535529e-01j,
        0.05505723+1.85830802e-02j,  0.00835002+2.95374366e-02j,  0.00710951+4.40654128e-02j,  0.17926977-3.38549657e-01j]


        u1 = ql.Unitary("testname",matrix)
        u1.decompose()
        k.x(4)
        k.x(1)
        k.x(0)
        k.gate(u1, [0, 1, 2, 3, 4])

        p.add_kernel(k)
        p.compile()
        qx.set(os.path.join(output_dir, p.name+'.qasm'))
        qx.execute()
        c0 = qx.get_state()


        self.assertAlmostEqual(helper_prob(matrix[19+0]),   helper_regex(c0)[0], 5)
        self.assertAlmostEqual(helper_prob(matrix[19+32]),  helper_regex(c0)[1], 5)
        self.assertAlmostEqual(helper_prob(matrix[19+64]),  helper_regex(c0)[2], 5)
        self.assertAlmostEqual(helper_prob(matrix[19+96]),  helper_regex(c0)[3], 5)
        self.assertAlmostEqual(helper_prob(matrix[19+128]), helper_regex(c0)[4], 5)
        self.assertAlmostEqual(helper_prob(matrix[19+160]), helper_regex(c0)[5], 5)
        self.assertAlmostEqual(helper_prob(matrix[19+192]), helper_regex(c0)[6], 5)
        self.assertAlmostEqual(helper_prob(matrix[19+224]), helper_regex(c0)[7], 5)
        self.assertAlmostEqual(helper_prob(matrix[19+256]), helper_regex(c0)[8], 5)
        self.assertAlmostEqual(helper_prob(matrix[19+288]), helper_regex(c0)[9], 5)
        self.assertAlmostEqual(helper_prob(matrix[19+320]), helper_regex(c0)[10], 5)
        self.assertAlmostEqual(helper_prob(matrix[19+352]), helper_regex(c0)[11], 5)
        self.assertAlmostEqual(helper_prob(matrix[19+384]), helper_regex(c0)[12], 5)
        self.assertAlmostEqual(helper_prob(matrix[19+416]), helper_regex(c0)[13], 5)
        self.assertAlmostEqual(helper_prob(matrix[19+448]), helper_regex(c0)[14], 5)
        self.assertAlmostEqual(helper_prob(matrix[19+480]), helper_regex(c0)[15], 5)
        self.assertAlmostEqual(helper_prob(matrix[19+512]), helper_regex(c0)[16], 5)
        self.assertAlmostEqual(helper_prob(matrix[19+544]), helper_regex(c0)[17], 5)
        self.assertAlmostEqual(helper_prob(matrix[19+576]), helper_regex(c0)[18], 5)
        self.assertAlmostEqual(helper_prob(matrix[19+608]), helper_regex(c0)[19], 5)
        self.assertAlmostEqual(helper_prob(matrix[19+640]), helper_regex(c0)[20], 5)
        self.assertAlmostEqual(helper_prob(matrix[19+672]), helper_regex(c0)[21], 5)
        self.assertAlmostEqual(helper_prob(matrix[19+704]), helper_regex(c0)[22], 5)
        self.assertAlmostEqual(helper_prob(matrix[19+736]), helper_regex(c0)[23], 5)
        self.assertAlmostEqual(helper_prob(matrix[19+768]), helper_regex(c0)[24], 5)
        self.assertAlmostEqual(helper_prob(matrix[19+800]), helper_regex(c0)[25], 5)
        self.assertAlmostEqual(helper_prob(matrix[19+832]), helper_regex(c0)[26], 5)
        self.assertAlmostEqual(helper_prob(matrix[19+864]), helper_regex(c0)[27], 5)
        self.assertAlmostEqual(helper_prob(matrix[19+896]), helper_regex(c0)[28], 5)
        self.assertAlmostEqual(helper_prob(matrix[19+928]), helper_regex(c0)[29], 5)
        self.assertAlmostEqual(helper_prob(matrix[19+960]), helper_regex(c0)[30], 5)
        self.assertAlmostEqual(helper_prob(matrix[19+992]), helper_regex(c0)[31], 5)

    def test_adding2tothepower6unitary(self):
        num_qubits = 6
        p = ql.Program('test_6qubitjustadding', platform, num_qubits)
        k = ql.Kernel('akernel', platform, num_qubits)

        matrix = [ 3.66034993e-01+2.16918726e-01j,  4.63119758e-02-1.25284236e-01j,  3.23689480e-01-1.67028180e-02j, -4.01291192e-02-1.53117445e-01j,  1.84825403e-01-3.19144463e-02j,  5.62152150e-03-1.90239447e-02j, -1.37878659e-01-9.85388790e-02j, -4.12560667e-02+2.40591696e-02j,  1.70864304e-02-9.30023497e-02j,  3.56649947e-02-4.69203339e-02j, -3.83183754e-02+5.12024751e-02j, -1.96496357e-02+2.63675889e-02j, -1.43417236e-01+2.87083079e-02j, -7.64191581e-02+8.10632958e-03j, -5.44376216e-02+3.71068995e-02j,  7.89219612e-02+6.00438590e-02j,  2.32485473e-01-2.61279978e-03j,  3.13800342e-02+1.24253833e-02j, -3.80054586e-03-1.73877756e-02j, -7.12322896e-03+9.73966530e-02j, -1.22838320e-02-6.80562063e-02j,  1.04704708e-01+4.17530991e-02j, -1.45411627e-02-4.56578204e-03j, -3.91129910e-02+8.17248378e-02j,  6.36557074e-02+2.21470313e-02j,  1.10349909e-01-1.21376682e-02j, -4.78526826e-02+5.49355237e-02j, -1.20516087e-01+4.01182200e-03j,  9.38275660e-02+2.24713754e-02j,  8.93979520e-02+5.13816205e-03j,
       -4.59396339e-02+1.59721722e-02j, -4.28884340e-03-4.63622637e-02j,  6.12649865e-02-1.22841538e-01j, -5.65913752e-02-6.08971314e-02j, -2.32312060e-02-1.65654059e-02j,  1.41147598e-02+4.73394414e-02j, -4.33854823e-02-1.51947091e-01j, -8.13427974e-02+2.99813808e-02j, -1.17417843e-01+1.90630710e-01j, -1.90231845e-02-8.35713650e-02j, -1.22044267e-01-7.47225397e-02j, -3.60683906e-02+6.78243340e-02j, -7.19769659e-02-2.89274301e-03j,  3.35704664e-02-6.57183304e-02j, -1.77382032e-01-2.84971997e-02j,  1.33681237e-01+1.01754278e-01j,  6.82852215e-02+9.51310080e-03j,  7.78691038e-02+2.00563796e-02j,  1.72520999e-02-4.06637398e-02j, -1.01923775e-02+5.59753174e-03j,  3.10544397e-02+1.61785537e-02j,  8.93474575e-03+6.24199961e-03j,  2.81125596e-02-3.97444226e-02j,  1.51177344e-01+2.05815209e-01j, -5.34831140e-02+1.77651158e-01j, -4.40789322e-02-2.71929907e-02j, -9.66417470e-02-6.07156542e-02j,  1.08161080e-01+1.20076484e-01j,  8.21245796e-02-2.30271819e-02j, -8.84132928e-03-5.48481546e-02j,
       -5.14895797e-02-1.37500021e-02j, -1.51874055e-02+6.32818901e-02j,  4.75397973e-02-7.13568201e-03j,  3.13807800e-02-1.14525719e-02j,  1.01592567e-01+1.60695903e-01j,  1.27305395e-01+1.95561563e-01j,  1.12090718e-01-6.51290458e-02j, -1.01825786e-04-1.81692203e-02j,  1.54924689e-01+2.64139117e-02j,  9.26041281e-02-1.10472514e-01j,  1.14444819e-01-2.07202663e-02j,  6.75494235e-03-7.42645200e-02j,  7.77019424e-02+7.23999587e-03j, -1.97577945e-02-1.21556587e-01j, -1.53460462e-01-6.09952436e-02j,  4.01985175e-02+6.32614480e-02j, -6.20026282e-02-1.35839473e-01j,  1.29200169e-01+3.78332216e-03j, -2.59883114e-02-9.05275193e-03j,  8.17860302e-02-3.48868665e-02j, -1.60053834e-01-2.02916150e-01j,  4.82858885e-02+1.09125088e-02j, -1.24753204e-02-1.22518525e-01j, -5.80857579e-02-4.19993263e-02j,  2.28396669e-02-8.40966881e-02j,  6.32309194e-02-1.34709187e-01j, -9.62511113e-02-7.66908701e-02j, -9.03694102e-02+1.30290490e-01j, -1.46952433e-01+1.04814111e-01j, -2.72713771e-02+9.46774405e-02j,
        6.43591466e-02+4.89870962e-03j,  4.75376946e-04+3.50098669e-02j, -1.49839101e-01+4.44685559e-02j,  3.36051502e-02-1.73310171e-02j,  6.33484348e-02+4.71325034e-02j, -2.17711578e-02+1.31801429e-02j,  1.74906999e-01+1.53006903e-01j,  8.86273947e-02+1.97117753e-01j,  2.80636110e-02-1.05335854e-01j, -2.55400957e-02+3.13987536e-03j,  1.90502728e-01-5.65463143e-02j, -4.83648932e-02+9.12606941e-02j,  5.98563090e-02-7.02341588e-02j, -8.94163863e-03+3.75629282e-02j,  7.28101901e-02+7.14544903e-02j, -6.45875285e-03-3.09883105e-02j,  8.81396901e-03+1.99334329e-01j,  7.90997911e-03-6.52739372e-02j, -6.22843622e-02+1.28419510e-02j, -9.96405825e-02+3.88022827e-02j,  2.56561292e-02-2.23919910e-02j,  6.96886035e-02+3.51775332e-02j, -3.51521748e-02+3.76936524e-02j,  9.75324023e-03+6.65739416e-02j, -6.83377964e-02-6.68801536e-02j,  5.26199194e-03-6.98174024e-02j, -3.18191450e-02-8.73246712e-02j,  7.65668019e-02-2.22700208e-01j,  1.39633338e-01+1.29785687e-01j,  7.76075375e-03+1.07250473e-01j,
        5.43176815e-03+1.07893416e-01j,  8.87283358e-02-2.63757540e-02j,  5.90762011e-03+1.21098716e-01j, -5.31234979e-02-7.38219329e-02j, -6.99676596e-02+1.83616052e-02j, -1.13045510e-01-1.38358191e-01j, -6.40257276e-02-1.33332243e-02j, -5.05842389e-02+1.17327054e-01j,  2.19460719e-01+1.53618876e-01j, -1.98098237e-01-1.83046859e-01j,  2.55895738e-01-1.22287636e-01j,  2.24304159e-02+3.12955813e-02j,  8.10330744e-02+9.09937415e-02j, -8.69829415e-02-1.31156605e-01j,  7.13752017e-02+3.22771841e-02j, -1.09383893e-01+2.78555202e-02j,  6.08603366e-03+1.30630331e-01j, -1.38543382e-01-1.32609733e-01j,  8.11354396e-02-1.56139915e-02j, -2.46721034e-02-6.17177733e-02j,  1.17317415e-01+1.46270026e-01j,  1.57044029e-02+2.81859378e-04j,  7.34343516e-02+1.32166441e-02j, -1.94047136e-02+3.71747664e-03j,  1.67740714e-02+1.55308704e-01j, -2.30281717e-01-1.13574589e-02j,  6.66702774e-02-3.44728569e-02j,  6.64536670e-02-4.11020927e-02j, -5.89556075e-02+1.21648953e-01j, -1.33381018e-01+1.11575280e-01j,
       -2.04297207e-02-1.44361996e-01j, -1.36404986e-01+1.75670409e-02j,  9.04729266e-02-2.05525311e-02j, -8.54217517e-02+8.81964403e-02j,  4.08009950e-02-4.65066696e-02j,  1.28598663e-01-1.26135509e-03j, -5.88955587e-02-2.16365675e-01j, -5.91081138e-02+3.93697458e-02j,  1.25841320e-01-6.59751293e-02j,  6.64938943e-02+5.27045503e-02j,  1.82761546e-01+9.47007410e-02j, -9.47093986e-02-4.42715353e-03j,  2.14782709e-02+1.08386929e-02j, -1.64295853e-03-6.85829980e-02j,  3.15183688e-02-2.83403844e-02j,  1.32010033e-02-1.35335939e-02j,  1.17740974e-01-4.02249320e-02j, -7.12640244e-03+4.92657515e-02j,  6.62000134e-02-6.58330394e-02j, -8.27247973e-02+5.46860857e-03j, -6.85927353e-02+1.60810901e-02j,  5.28608831e-02-4.24129436e-02j,  7.90603356e-02+5.78672352e-02j,  3.73193949e-02-4.61190576e-02j,  4.91693254e-02-1.00191521e-01j, -8.30279823e-02-4.53136772e-02j,  8.52488016e-02-9.43347617e-03j, -3.36847083e-02+4.29105012e-02j,  1.17835217e-01-1.03615646e-02j,  9.03099416e-03+1.23603737e-01j,
       -8.85120477e-02+5.98128080e-02j, -1.07646777e-01-1.06745536e-01j,  2.85703816e-02-1.32915304e-02j,  6.54799867e-02-1.05639352e-01j,  1.56220180e-01-1.94186883e-02j, -8.98506499e-02+1.20794812e-02j, -3.34699800e-02+3.97067051e-02j, -1.13133666e-01+1.39529758e-02j,  3.82233228e-02-6.51392562e-03j,  1.16912282e-01-7.57853365e-02j, -3.02029062e-02+2.03203809e-03j, -7.05833281e-02-3.72791361e-02j,  1.18487986e-01+3.41305694e-02j,  1.90369334e-01-4.23433077e-02j,  1.78329157e-01-2.46079024e-03j,  1.42514615e-02-5.00931284e-02j,  6.22258069e-03+2.71254537e-01j, -1.05493042e-01+4.81011019e-02j,  1.63685462e-01+3.54417709e-02j, -7.92557707e-02+3.37712908e-02j, -6.89388107e-02+9.52917547e-05j, -5.98463661e-02+9.39029791e-02j,  7.83393843e-02+4.48842194e-02j, -4.03889441e-02+1.58463043e-02j,  4.69349327e-02+7.36054463e-02j, -1.23002359e-01+3.56702348e-02j, -8.08745549e-02-9.36340081e-02j, -1.53111649e-01-1.30519031e-01j, -4.55188428e-02-2.63939035e-02j, -5.13289900e-02+3.12841022e-02j,
       -7.22190692e-03+9.84746771e-02j, -4.23305508e-02-4.16100064e-02j,  9.70155024e-02-5.10653722e-02j, -1.52262614e-02+9.70290076e-02j, -4.95065203e-02-4.71260667e-02j,  2.63361010e-01+5.88816734e-02j, -3.58878585e-02-2.50486879e-03j, -2.14856796e-01-7.75726911e-02j, -9.45755082e-02-1.02420516e-01j, -1.45089067e-01-1.73675325e-02j, -1.28612734e-01+2.62396621e-02j, -1.27273026e-01-2.41474716e-02j, -1.08532484e-01-7.70550142e-02j, -6.11361542e-02+5.21576989e-02j, -1.49885849e-01+6.33951444e-02j,  1.39221310e-01+4.74904068e-02j,  7.98734756e-03+1.21834984e-01j, -6.70609516e-02+1.03519222e-01j,  9.84581689e-02-1.90608636e-02j, -4.20985103e-04-5.34460521e-02j,  4.61698910e-02-5.36918570e-02j, -1.80638508e-02-5.52001591e-02j,  3.26902700e-02+1.03032250e-01j,  2.19231364e-01+5.34459046e-03j,  2.02344664e-01+8.05112282e-02j,  1.12050260e-01-9.47627052e-02j,  5.01092383e-02+2.80856107e-02j,  1.54394724e-01-2.02727577e-02j, -1.28311863e-02-1.32251018e-02j,  1.24297206e-01+5.16860223e-02j,
       -2.40603762e-02+2.74847000e-03j, -7.65154347e-02-6.29854246e-02j,  1.90751859e-01-5.75903679e-02j,  1.31040663e-02-6.01812829e-02j,  3.98490344e-02-8.53582487e-03j, -9.87057147e-03+8.47414791e-02j,  4.96880859e-02+1.52103456e-02j, -2.34893852e-02+2.41581495e-02j, -6.29676487e-02+8.60622743e-02j,  3.33756353e-03+2.80774648e-02j, -6.00395642e-02-1.05197812e-01j,  8.12528420e-02+7.93631414e-02j, -2.07420548e-02+1.38374365e-01j,  5.36463694e-02-3.94927621e-03j, -1.96804868e-02+7.52059881e-02j, -3.71609399e-02+2.89964329e-02j,  1.54225234e-01+1.66400526e-01j, -4.28794858e-02+3.77165370e-02j, -4.72161861e-02+1.05193382e-01j,  1.88483774e-02+7.86924100e-02j, -3.05826462e-02+1.63053493e-01j,  6.64422544e-02+1.30615029e-01j,  1.19393774e-01+1.07635386e-01j,  1.15236137e-01+4.54248967e-02j,  8.36065560e-02-1.16369074e-01j, -7.31696969e-02+3.27398301e-02j,  1.04035711e-02+6.94251001e-02j, -1.12999867e-02-3.91635996e-02j, -6.25688782e-02+1.10951995e-01j,  6.06383062e-02+1.30678577e-01j,
        3.87713914e-02-2.56890752e-02j, -2.12352952e-02+7.46470611e-02j,  3.81665661e-02-4.66485675e-03j,  9.83793872e-02-5.47848105e-02j, -7.89298421e-02+9.89548326e-03j,  8.09874389e-02+2.88991618e-02j, -2.84479799e-02-3.95612619e-02j,  5.96981585e-02-1.18286529e-01j,  6.23627334e-02+1.26721441e-01j, -3.10124832e-02-1.84640556e-02j,  1.03597375e-01-1.34393931e-01j, -1.17812622e-02-6.89229414e-02j, -1.44678634e-01+2.03634696e-02j, -1.92499694e-02+3.83736754e-02j, -1.54185344e-02+1.54256354e-02j, -6.22128278e-02-6.06971077e-02j, -1.54442796e-01-4.55965667e-02j, -1.17280933e-02-2.96076710e-02j,  3.61636242e-01-3.68311909e-02j,  1.62232374e-03-2.46211943e-01j, -4.92385971e-02-8.23015443e-03j, -1.45369403e-01-1.29391236e-02j, -5.95406360e-02-2.40696609e-02j,  1.05369118e-01+3.57907856e-02j,  1.54655257e-01-4.92247169e-02j, -1.92329144e-01+2.80700834e-02j,  7.34014412e-02-6.83985774e-02j,  1.59129968e-01+2.24675645e-02j,  4.50009905e-02+8.28684110e-02j, -1.55862846e-01+1.38581516e-01j,
       -5.75055241e-02+2.76394588e-03j, -2.51204024e-01-1.28699277e-01j, -4.06946336e-02+7.17282849e-03j, -5.60177740e-02-2.97766068e-02j,  5.20270958e-02-1.24980073e-01j, -2.70809079e-02-7.62240935e-02j, -9.13895753e-02+3.09052839e-02j, -2.58735369e-02+5.60249825e-02j, -1.47477861e-02+6.08183848e-02j,  4.89102367e-02-2.32083741e-04j,  4.26959278e-02+7.86994426e-02j, -5.37737809e-02+4.29931841e-02j, -7.51527527e-02+1.05047426e-02j, -7.92092909e-02-6.76905017e-02j, -1.53434244e-02+5.39639561e-02j,  1.02041990e-01+1.20369368e-01j,  3.16941445e-03+9.84779820e-02j,  4.50698004e-02+8.95480278e-03j, -3.06674727e-02-3.23950934e-02j, -7.67166598e-02+5.80876145e-03j,  4.03226297e-02+5.02470157e-02j,  9.20435646e-02+6.16208640e-02j,  4.53349090e-02-6.97415185e-02j, -1.43676770e-01+3.16896179e-02j, -5.70956416e-03+1.14461269e-01j,  3.36727523e-02+3.46621803e-02j, -1.28349306e-01+6.15831113e-02j,  2.92131791e-02-1.11494758e-01j,  6.78039246e-02+5.74136771e-02j, -7.37065550e-02-1.55476166e-02j,
        2.56255331e-02-4.43527889e-02j,  3.60829775e-02+1.48859744e-02j, -3.26999034e-02+1.41339655e-01j,  7.33897143e-02-5.13214915e-02j, -3.90630444e-02+7.98559328e-02j,  2.68903082e-02+5.83120382e-02j,  1.14205526e-01-3.53133007e-02j,  1.67581969e-01-2.17414475e-02j, -8.47370959e-02-1.03605182e-01j, -1.02980585e-01+1.87241569e-01j,  4.88560456e-02-1.87154362e-02j,  9.14764037e-04+6.72307848e-02j,  1.54753913e-02+6.22097288e-02j, -2.66815276e-02+7.33037713e-02j,  7.26024183e-03-1.57891621e-01j,  1.61465938e-02-6.23853508e-02j, -1.58906775e-02+9.22861340e-02j,  1.09187430e-01+1.22305747e-03j,  2.04879819e-02+6.30194559e-02j, -4.91774816e-02+1.98730477e-01j,  7.71514495e-02-8.70875038e-03j,  1.57479965e-01+1.04217181e-01j,  4.23884657e-02+9.41224038e-02j,  1.23866904e-01+2.54965114e-01j, -4.96733375e-03-4.34545446e-02j,  1.66863425e-01+1.03216014e-01j,  1.22161732e-02+1.74410123e-01j,  7.56016604e-02+1.14041093e-02j,  1.29416658e-01-1.07683681e-01j,  8.49692580e-03-9.98225347e-02j,
       -2.53682806e-03-1.99317517e-01j,  1.32776926e-01+6.57308336e-02j,  2.48792835e-02-2.64841453e-01j,  1.03428049e-01+9.65965570e-02j, -1.51241974e-01-5.72183934e-03j,  1.88201884e-02+9.78051235e-02j, -3.52125401e-03-9.10550190e-02j, -1.21382845e-02-5.61863132e-02j, -2.72633341e-03+3.66383373e-02j,  1.31161516e-01-1.26913372e-02j,  3.46525801e-02-8.46938669e-02j,  1.17489600e-01-1.27053502e-01j, -7.75126317e-02+3.30270148e-02j, -7.85367649e-03-1.59325461e-02j, -2.04527534e-02-1.50973103e-01j,  1.19070061e-01+2.74181755e-02j, -1.24892455e-01-2.71536222e-02j, -8.38973196e-02-2.83671293e-03j, -9.23824862e-02-1.17795710e-01j, -2.12548944e-02+5.61017801e-02j,  1.45278490e-01-9.56859548e-03j, -1.82863761e-03+4.39439680e-02j, -5.97226268e-02-3.11260187e-02j, -4.76076983e-02-2.13141614e-02j, -4.54603071e-02-1.87085243e-02j,  1.40969780e-01-1.09754704e-01j, -1.01455639e-01+7.71777346e-02j,  1.48752797e-02-1.32375476e-01j, -9.40626155e-02+7.11577906e-02j, -2.12131838e-02-7.11116567e-02j,
        1.11184697e-01+1.35340762e-01j, -1.67713585e-01-2.00603909e-02j,  6.69071941e-02+4.20345476e-02j, -6.05717732e-03-9.68103657e-02j, -4.66999484e-02-8.16202516e-02j, -8.45240798e-02+5.60371406e-02j,  1.26572662e-01+4.52015693e-02j,  1.15084298e-01+1.80329121e-02j, -8.19534585e-02+9.92455558e-02j,  5.82837845e-02-9.60191435e-02j,  1.40466777e-01-9.50202996e-02j,  5.11287771e-02-2.53675878e-02j,  2.47544744e-02+2.06020479e-02j, -3.65720656e-03-4.94804816e-02j, -1.41559984e-02+1.67809139e-01j,  3.40859417e-02+1.36896363e-02j, -4.29813547e-02+1.10736905e-01j,  1.27523603e-01+2.07169069e-02j,  1.47572639e-01-4.16243755e-02j,  5.26379357e-02+6.25793319e-02j,  1.49109608e-01+1.47242665e-01j, -6.65329123e-02+3.26931942e-02j,  1.91342477e-01+6.13849533e-02j, -3.48863201e-02+1.16221687e-01j, -1.84725165e-02+8.88063853e-02j, -1.46651117e-01-8.03755334e-02j,  3.18322341e-01+1.28855771e-01j, -7.34176530e-02+6.28676759e-02j,  6.59855007e-02-4.02354168e-03j, -1.14723570e-02-1.41192022e-03j,
        1.31986821e-02+8.66384236e-02j, -2.06143698e-02-1.06385925e-01j, -1.29887505e-01-5.82215934e-02j,  3.55333019e-02+1.72976962e-01j, -4.43318809e-02+7.50029900e-02j,  2.42008445e-02+3.81880311e-02j,  9.24309092e-02+2.08369499e-02j, -1.73024821e-01+5.21566745e-03j,  2.19700726e-01+1.17537260e-01j,  1.63993038e-01+7.53919556e-02j,  7.07529823e-02+3.32108419e-02j,  8.06363236e-02+5.69884702e-02j,  6.53884989e-02-6.06009350e-02j,  1.02796574e-01+7.41913070e-02j,  3.21527509e-02-3.68525050e-02j,  5.23724001e-02+6.69483928e-02j,  1.40581761e-02+7.70120060e-02j,  7.42821790e-02+7.73759929e-02j,  1.03095422e-01-7.83293200e-02j, -5.98649602e-02-9.76718764e-02j, -8.58898366e-02-1.20654270e-01j,  9.76478031e-02-2.49354014e-02j, -3.33481195e-02+1.59120400e-02j, -1.04779024e-02-9.02867422e-02j, -3.68342490e-02+6.83856814e-03j, -5.03744003e-02+2.55842171e-02j, -1.72672267e-02+2.47688542e-02j, -1.81408876e-02-7.60577495e-03j,  6.15478341e-02+9.21537711e-03j, -9.54714639e-02+4.91095211e-02j,
       -3.28772457e-02-1.07419632e-02j,  7.91895326e-02-1.16514331e-01j, -3.28981168e-02+6.24741888e-03j, -8.74370047e-02-1.34809532e-02j, -3.78068572e-02-2.85542044e-01j,  2.27133052e-02+8.76888751e-02j,  9.11283724e-03+1.95425832e-02j,  6.35536375e-02-6.45304136e-02j, -6.05664346e-02-8.13326951e-02j,  9.51930052e-02-6.52573951e-02j, -5.72679924e-02+8.44254015e-02j, -3.90623628e-02-3.15090263e-02j,  1.02656566e-01-1.53783548e-02j, -2.90858731e-02+3.83758991e-02j,  4.00015008e-02+9.29419353e-03j,  2.16293344e-02-6.15080785e-02j, -3.94283903e-02-2.86725847e-02j, -9.82554318e-02-3.05356109e-02j,  1.43678779e-01+1.66757227e-01j,  4.33669816e-02+2.09725691e-02j,  6.09968565e-02+1.35896205e-01j,  1.86428835e-02-1.07078161e-02j,  3.59210771e-02-4.96728908e-02j, -3.09977809e-02+4.20063574e-02j, -3.36483096e-02+4.06648103e-03j, -6.78492085e-02+3.60589692e-02j, -4.57256782e-02+2.29382640e-02j,  8.41604265e-02-1.21627615e-01j,  1.23159551e-02-4.33185856e-02j,  2.92901980e-02+7.68246203e-02j,
        3.57537713e-02+1.13757740e-01j,  6.61076239e-03+2.35766830e-03j, -2.50170508e-02+2.02216570e-01j,  1.90226197e-02-9.30319796e-02j,  3.52294231e-02+1.98930350e-01j,  6.47480042e-02-7.71035915e-02j,  1.06576071e-01+1.60636469e-01j, -1.84106675e-01+1.48367027e-02j, -2.26634183e-02-6.05001248e-02j, -4.09795810e-02+4.28997415e-02j,  1.18647136e-01+2.84135141e-02j,  1.05658964e-01+1.11901959e-01j,  1.55385384e-01+1.23916786e-01j,  2.50437539e-02+2.39415647e-01j, -3.10447452e-02+7.27368239e-02j,  2.03285504e-02-9.59860836e-02j, -3.39711391e-03-8.06167738e-02j, -1.36708865e-01+1.58365544e-01j, -7.63968104e-02-4.54852761e-02j, -1.34493163e-01-1.62060048e-01j,  1.37630857e-01-5.97073184e-02j,  1.26456118e-01-8.53282313e-02j, -1.44090748e-01-6.48238553e-02j,  1.14527429e-02-8.90366168e-02j,  1.26772282e-01+1.50468306e-01j,  1.09521345e-01+5.79354942e-03j, -6.36493029e-03+1.04114079e-01j, -3.46448312e-02+1.55499200e-01j, -1.91801782e-02-5.55803895e-02j, -1.09747875e-01+1.16908867e-01j,
       -5.98737364e-02+6.92798355e-02j, -7.70045009e-02+8.69529376e-02j,  1.41838471e-01+1.57609972e-01j,  1.05064988e-01+1.27448858e-01j, -5.58481223e-02+1.18970561e-01j,  3.11104760e-02+7.09377243e-03j,  1.17401170e-01-1.29171507e-01j, -9.23143689e-02-6.32936915e-02j, -1.57360238e-02-3.48087654e-03j, -3.68085794e-02-6.90240362e-02j,  3.72129634e-01+1.05885225e-01j,  1.47539078e-01-1.02019375e-01j,  9.10168034e-02-9.45885626e-02j, -6.41230394e-02+5.23466780e-02j,  2.09042766e-01-5.04017256e-02j, -8.18752152e-02-1.67729197e-01j,  2.43479176e-02-3.15791155e-02j, -3.54894844e-02-2.06208641e-01j,  9.79597853e-02+1.27225349e-01j, -3.87549084e-02-2.32046371e-02j, -6.29155767e-02+1.93572123e-02j,  2.80668515e-02+5.94461601e-02j, -6.79156216e-03+3.34189314e-02j, -8.17558519e-02-9.72531723e-02j, -6.93388764e-02+9.75765597e-02j,  1.13872557e-01+5.43605204e-02j,  4.98782889e-03-1.43108491e-01j,  2.50693943e-02-1.18651968e-01j, -8.42094212e-02+3.40094201e-02j, -1.93122748e-03+1.26278033e-01j,
        7.21626311e-02+4.14478810e-02j,  1.30623663e-01+2.44967192e-04j,  9.43901041e-03-9.97110706e-02j, -2.92711792e-03+1.03468344e-01j, -2.26707613e-01+3.41895256e-04j, -1.29510144e-01+1.87631777e-02j, -1.89888081e-04-2.55952955e-02j, -8.26946001e-02+6.13660772e-02j, -1.11228037e-02-1.58220779e-01j,  7.61250434e-02+2.67080152e-02j,  9.25782693e-02+6.13248807e-03j,  2.59736432e-02-2.37141043e-02j, -2.91932014e-02-2.25217711e-02j, -5.16699426e-03-1.36738487e-01j, -2.79601717e-02-4.16723747e-02j, -7.06047980e-02+1.66551658e-02j,  3.72096502e-02+3.30948726e-02j, -1.61133828e-02-1.31037806e-01j,  2.48425980e-02+1.25040461e-01j,  1.24479311e-02+4.85744755e-03j, -6.81173401e-02+7.38703130e-02j, -4.93754225e-02+1.06791006e-01j, -7.68461360e-02-4.01290044e-02j,  1.51095443e-02+6.82241087e-02j, -1.36934200e-01+1.03824672e-01j,  2.64272936e-02-2.80269677e-02j, -8.63367945e-02-3.13888054e-02j, -2.25404421e-04+7.88537649e-02j,  6.92346834e-03+3.17886627e-02j,  5.86120252e-02-2.00444048e-01j,
       -6.88131527e-02+3.50491144e-02j,  2.62520589e-02+1.58407829e-02j,  1.66265278e-03-1.03532297e-02j,  7.69386002e-02-2.67185303e-02j,  1.01084098e-02+3.79223130e-03j, -7.29521430e-02+9.08482588e-02j, -4.61024420e-02+1.94411467e-01j,  1.53468094e-03+1.86350862e-02j,  7.27775150e-02-2.59153001e-02j, -2.05928168e-02-4.88869881e-02j,  1.26185139e-01+3.10908628e-02j,  5.16540196e-02-4.63561741e-02j, -4.00987002e-02-4.88168980e-02j,  2.74709517e-02+1.12099875e-01j,  7.06457783e-02+5.46480806e-02j,  3.46431872e-02+7.06736324e-03j,  5.62921199e-03+1.52252805e-01j, -1.05318693e-01-6.42034872e-02j,  2.61399114e-01+3.12581452e-02j,  1.42549889e-01-7.20539802e-03j, -4.01513611e-02-1.09584788e-01j,  2.32103459e-02-3.08901316e-02j, -8.87718807e-02-4.81598900e-02j,  3.88078001e-02-1.48971115e-03j,  8.20382929e-03-1.60908451e-01j,  1.53171161e-01+9.34285397e-02j, -9.84955476e-02-3.77925732e-04j,  1.15359466e-01+1.53988904e-01j, -3.53067842e-02+2.22448115e-02j, -2.76010476e-02-1.02685489e-01j,
        6.47338872e-02+3.21032179e-02j, -6.25630438e-02-9.57348589e-02j, -1.06676085e-01-6.02328785e-02j,  6.32336003e-02+9.18568053e-02j,  4.22806111e-02+3.67386778e-02j,  3.34450227e-02-9.49873539e-02j, -9.25500440e-02+7.10428902e-02j,  5.83735994e-02-1.98170082e-01j, -9.31369416e-02-6.00026885e-02j, -4.19679158e-02+6.04049319e-02j,  2.94658418e-02-2.07003282e-02j,  8.51945088e-02+8.19217414e-02j, -1.40734017e-01+7.52647297e-02j,  2.42289626e-01-1.27759422e-02j, -9.38154491e-02-1.26465660e-01j, -8.95541629e-03-1.42613529e-02j,  8.03314780e-03+1.60930516e-01j, -4.88653862e-02+1.17749379e-01j, -7.39379399e-02+8.62428712e-02j, -2.59414228e-03-7.84326485e-03j,  2.00269020e-01+9.58856720e-03j, -1.19645441e-02+4.34306329e-02j,  6.09717160e-03-1.54242927e-01j,  1.08644840e-01-7.83434868e-02j, -1.45600371e-01-5.53383550e-02j,  6.95876462e-02-3.75146138e-02j, -1.01317383e-01-3.44359913e-03j, -4.61204619e-02-9.77882682e-02j,  7.60823182e-02-1.37389279e-01j,  4.87149823e-02+9.94095931e-03j,
       -4.76778208e-02+3.06934398e-02j,  1.34616868e-01-3.35001866e-02j,  9.42819618e-03-7.70997443e-02j, -1.58872569e-01-8.51718385e-02j, -1.08323340e-02-3.58830573e-02j, -9.44807628e-02+1.04462576e-01j, -5.12024697e-02+8.95696029e-02j, -1.23263290e-01-2.11365137e-02j, -6.23407814e-02+5.27129163e-02j,  1.84350076e-01-6.48454604e-02j,  4.47048643e-02+9.55835619e-02j,  3.04136984e-02-1.61047348e-01j,  5.35742906e-02-8.89771961e-02j,  5.21798913e-03-1.01652683e-01j,  9.81082670e-02+6.78883376e-02j,  4.51160517e-02+4.08974261e-02j,  7.24547188e-02+1.43396898e-02j,  1.34520732e-01+7.64023876e-02j, -2.44439308e-02+1.93019446e-01j,  4.70632798e-02+5.40126182e-02j,  1.44082445e-01-6.94027082e-02j,  1.10724758e-01-4.81811556e-02j,  1.75840970e-01+1.16857338e-01j, -1.38125661e-01-1.28426487e-01j,  1.67653981e-01+6.10401648e-02j, -9.27944162e-02-4.45898489e-02j, -6.36024967e-02-1.32504351e-01j, -3.83547731e-02-4.80351172e-02j,  2.87039348e-03+1.86993497e-01j, -8.06227067e-02-8.55853307e-02j,
        1.98050483e-01-5.65883695e-02j, -6.61066167e-02-1.71061740e-01j, -3.88695750e-02+1.09338522e-01j, -7.49700094e-02+9.33229878e-02j, -1.57544410e-02+5.63796550e-02j,  9.53506257e-02-6.37602365e-02j,  2.75211340e-03+7.79446442e-02j,  1.39113602e-01-1.36189229e-01j,  1.17220189e-01+9.46405945e-02j, -1.81301425e-02-2.48995711e-02j, -9.81095323e-02+1.42902852e-01j,  1.41212310e-01-1.26684189e-01j, -5.05751207e-02+9.51904702e-02j,  1.04859322e-01-2.04087816e-02j, -1.21826474e-01-1.07626674e-02j, -9.57058985e-03+1.37576990e-03j, -4.74540116e-02+1.96568999e-01j, -1.38789725e-01+5.29123729e-02j, -1.06381314e-02+7.45998022e-02j,  7.06297513e-02-7.13617157e-03j, -1.75797263e-03+1.12692942e-02j, -1.08418056e-01+7.80141453e-02j,  7.95831370e-02+5.43897927e-02j, -4.77081927e-02-1.65112693e-02j, -3.03381357e-02+8.08941075e-03j, -8.23004681e-02+1.05759705e-02j, -6.56969750e-02-1.79256431e-01j, -3.76059369e-02+1.78724732e-01j, -3.13850703e-02+9.23421858e-03j, -3.23711459e-02+8.23655512e-02j,
       -5.52695718e-02-2.86257551e-02j, -8.21930608e-02+2.84104439e-02j,  6.88034715e-02+7.91090633e-02j, -9.58878856e-02+1.62110597e-02j,  1.85489072e-02+7.72518030e-02j, -8.47308705e-02+1.15419402e-01j, -1.27165710e-02-1.47844195e-01j, -3.31225492e-02-1.12256225e-01j,  4.38928408e-02+3.01083140e-02j,  1.49664011e-02-3.23579402e-02j,  6.18136699e-02-3.20311892e-02j, -2.00715491e-02+3.36952165e-02j,  3.84726438e-02-1.47570105e-01j,  5.66270826e-02+1.60052582e-03j, -4.30699454e-03+7.36300298e-02j,  1.17987394e-01+4.71108745e-02j, -6.17743138e-02-7.33753780e-02j, -2.90245523e-02-2.72610689e-02j,  1.91805045e-02+7.28153549e-02j, -2.42663866e-02+1.76970283e-01j,  1.34153427e-01-6.33407243e-02j,  2.80424444e-02+7.13200467e-02j, -1.89628021e-01+3.08308998e-02j,  8.30657938e-02-1.92419259e-01j,  7.56007039e-02+1.32593910e-01j,  4.03658952e-02+9.28689978e-02j,  2.32661487e-01+5.37620195e-02j,  9.71324362e-03+1.55793479e-01j, -7.90003403e-02-9.01055799e-02j, -7.19042248e-03+4.98522998e-02j,
       -7.89912325e-03-9.22331480e-02j, -1.52218111e-01-2.70308556e-02j,  1.52204585e-01-1.04355797e-01j, -9.58268719e-02-2.66039347e-02j, -5.85839839e-02-8.45115184e-02j, -7.67554648e-02+4.09816179e-02j, -1.23737409e-02+6.27494390e-02j,  1.76544689e-02+2.91020457e-02j,  7.88970104e-02-3.84634885e-02j,  1.46944340e-01+4.22569979e-02j,  9.45664458e-02+1.75073953e-01j, -7.43530291e-02+5.39858529e-02j,  5.30273947e-02-2.04301186e-02j,  9.01691091e-02-1.37635743e-01j,  5.17323825e-02-4.42395728e-02j,  4.51065835e-02+1.55587162e-01j, -4.32070922e-02-8.47367622e-03j,  8.89766181e-02+7.27808224e-03j, -3.05321402e-02+4.01867117e-02j,  9.02139365e-02-1.02921926e-01j, -9.68996356e-02-5.57902239e-02j,  1.86882008e-01-3.65138998e-03j, -3.56735640e-02-5.38393441e-03j,  1.03631981e-02+6.62353454e-02j,  1.39301908e-02-3.82579866e-02j,  1.29806843e-02+1.15547571e-01j, -7.78767617e-02+1.17577632e-01j, -6.55156854e-02-3.75568378e-02j, -1.16185770e-01-9.97472455e-02j,  1.13027024e-01-1.71188329e-02j,
       -6.23765584e-02+9.71995127e-02j,  1.98853369e-02+4.41168144e-02j,  5.16372259e-02+1.04020291e-01j,  2.45487579e-02-7.07505804e-02j, -5.66916759e-03+1.79107188e-01j,  1.34919009e-01-3.38433266e-02j, -9.26335780e-02-2.41450966e-02j, -5.93110207e-02-1.13009956e-01j, -1.12473035e-01-1.02705305e-01j, -4.54029970e-02-8.46187294e-02j, -6.23708588e-02+4.57479972e-02j,  1.03992622e-02+1.00980439e-01j,  1.23694781e-01+1.22523548e-01j,  1.75959124e-01+6.68614671e-02j,  1.27580207e-01-7.10524199e-02j, -2.21112630e-02-3.48999085e-02j, -2.89686819e-02-1.38112449e-01j, -4.03154317e-02+1.30278763e-01j, -9.87558643e-03+2.11808797e-01j,  5.51166437e-02+7.28637045e-02j, -1.08116027e-01-1.25165016e-01j, -3.86238981e-02+8.93334399e-02j,  1.05216928e-01+6.44215494e-02j,  8.30120047e-02+1.60173664e-01j, -1.61665814e-01+1.33604609e-01j, -1.07929918e-01-2.27824193e-02j,  7.55794397e-02+5.35699856e-02j,  6.46640729e-02+6.77207472e-02j, -7.58619365e-02-1.36147809e-02j,  1.13368566e-01+1.41535454e-02j,
        1.16562600e-02+1.40637360e-01j, -3.48865452e-02-3.87334493e-02j,  2.84836458e-02+4.90837454e-02j, -3.44567796e-02+7.63272004e-02j,  1.61636226e-01-1.05180549e-01j, -4.75623594e-02+6.52991698e-02j,  4.99185253e-02+1.14465223e-01j, -5.40773631e-02-8.42919215e-02j, -1.14147601e-01+7.66188244e-02j, -1.40176445e-02+1.81528191e-01j,  4.35191919e-03+4.15940186e-02j,  1.27180295e-01+1.33144935e-01j,  9.06497753e-02-8.52809907e-03j, -1.26885003e-01-1.51045190e-01j,  1.02171723e-01-2.10818775e-02j, -2.98553434e-02+5.76845178e-03j, -1.60706438e-01-2.00370478e-01j,  1.51384923e-02-1.26679865e-01j,  7.55024672e-02+1.00895062e-01j, -7.45007080e-03-7.05098188e-02j, -2.88191949e-04-3.68919966e-02j,  5.62265696e-02+3.61613198e-02j,  4.24155733e-02-1.35428909e-01j, -7.53735593e-02-1.13686296e-02j, -1.16745754e-01+7.74921732e-03j, -6.94457624e-02-7.72663319e-02j,  3.74783331e-02+4.85062269e-02j,  6.52243264e-02-1.18436957e-01j,  1.17192626e-01+2.15233561e-02j,  1.02189810e-02-2.93259136e-01j,
        1.20140155e-02-6.63128933e-02j, -7.59536948e-02+6.71887612e-02j,  1.03744562e-02-1.21868885e-01j,  2.44016980e-02+6.89084519e-02j, -4.65470072e-02+1.22685320e-01j, -4.86962360e-02-4.88161112e-02j, -2.34017454e-02-5.77271045e-02j,  1.13502743e-02+4.16363888e-02j, -1.23278700e-01+1.12353134e-01j, -1.00881756e-01+1.17893431e-02j,  1.43470161e-01-3.29644042e-02j, -6.02563150e-02-4.45108227e-02j,  2.08219274e-01-1.67238564e-02j,  1.17121095e-02-6.33598388e-02j,  1.02538269e-01+3.22547888e-02j,  2.06012703e-03+1.07778634e-01j,  2.65464923e-02-3.46848652e-03j,  1.26264949e-02-7.16205403e-02j,  1.88694092e-02+9.58433324e-03j, -1.35215533e-01+2.75775658e-02j,  8.38743345e-02-3.94921696e-03j,  7.32804883e-02+4.21473224e-02j,  9.07604721e-02+1.45926883e-02j, -6.18641521e-02+7.52679281e-02j,  3.77612246e-02+2.95629420e-02j, -1.09369090e-01+3.64775541e-02j, -8.08080517e-02-8.08179068e-02j,  1.25920478e-01+1.02020075e-01j,  1.48976545e-01-3.02271686e-02j, -6.94345873e-02-9.09390945e-03j,
        1.31215815e-01-4.81602495e-03j, -2.05577715e-01+1.85648890e-01j, -4.20038791e-02-6.79461932e-03j, -4.21802223e-02+3.20414345e-02j, -3.87813853e-02+8.90865396e-02j, -4.21262137e-03-2.67890619e-02j,  6.79636786e-02-1.01093511e-01j,  1.40097339e-01+1.13907091e-02j, -1.88534819e-02+1.86465325e-01j,  2.71881881e-02-3.96160387e-02j, -6.42435188e-02-5.92185512e-03j, -4.89365496e-02-5.43585428e-02j, -4.57530970e-02-4.59613671e-02j, -3.61926650e-02+6.52346667e-02j, -9.61553968e-02-1.04451040e-02j,  9.04112402e-02-1.17533502e-01j,  1.74545444e-02-4.57240195e-02j, -4.89015824e-02+7.55148764e-02j, -1.42395993e-01-2.07951667e-02j, -1.04312418e-01+1.84020305e-02j,  1.04123907e-01-2.12641676e-02j,  1.81621294e-02-1.38150470e-01j,  6.90749445e-02-5.41908577e-02j,  1.25242285e-01+6.69590747e-02j, -1.12044943e-01+7.01709827e-02j,  1.18559009e-01+6.36320518e-03j, -4.09721959e-02-7.48052629e-02j,  2.11912964e-01+6.98119674e-03j, -5.53338736e-02+3.84548615e-02j, -1.73475512e-02+4.66847305e-03j,
       -1.39453707e-01+3.59052683e-02j,  5.65519475e-02+1.33581775e-01j,  9.24610673e-02+9.71550192e-04j,  1.01271681e-01+2.91580010e-02j,  5.65757574e-02-5.63073301e-02j, -6.89138190e-02+8.79709939e-02j,  1.32780848e-01+1.15842424e-01j,  5.74757429e-02+4.60581605e-02j,  1.30747705e-01-1.09382867e-01j, -6.81587412e-02+7.23700700e-02j, -4.77640537e-02-8.69425608e-02j,  5.41468987e-02+4.70866972e-02j, -5.45249768e-02+1.28327565e-01j,  3.88602977e-03+1.11946956e-01j,  2.58870901e-02-1.91503109e-01j, -3.66945777e-02-1.10208009e-01j, -2.56790612e-02+1.58065038e-01j, -1.90786566e-01+1.14107636e-01j, -5.60322067e-02-1.21127263e-02j,  1.08860290e-02+1.24596622e-01j, -1.48768362e-01+2.37695281e-04j, -1.47649036e-02-1.64313895e-01j, -4.79489441e-02-2.08483452e-01j, -8.71341644e-03-2.32533010e-02j,  7.23197740e-04-1.45104996e-01j, -1.03649950e-01+5.46849449e-02j, -1.40039869e-02+1.54117586e-02j,  1.34270729e-01+7.10254799e-02j, -1.62944885e-02-6.79478047e-02j,  3.02355044e-02+9.34069481e-02j,
       -4.19572996e-02-2.36984771e-03j,  5.19461879e-02+1.37769363e-02j, -1.56996645e-01-1.34660692e-01j, -4.48308181e-02-3.38817723e-02j,  5.73788104e-02+1.42412043e-01j, -1.15377475e-01-3.01131028e-02j, -7.77514856e-03+8.05802903e-02j, -1.80121894e-02-4.97689832e-02j,  2.74606415e-02+1.48966719e-01j, -1.17119892e-01+8.81288551e-02j,  7.95268357e-02-3.87759152e-02j, -1.66297510e-02-7.62547866e-02j,  4.37512230e-02-4.07918510e-03j, -7.00709466e-02-1.80395743e-02j,  1.17198057e-01-7.79683242e-03j,  1.75402728e-01-1.75827182e-02j,  5.42956130e-02-3.91441862e-04j,  1.01465093e-01+9.25475158e-02j,  5.36093332e-02+8.63715906e-02j,  3.05460006e-03+4.52463764e-02j, -1.46059785e-01-1.29686001e-02j,  1.80571255e-02+1.12753039e-01j, -8.21657514e-02-4.73360574e-02j, -1.10948811e-02-8.29924942e-02j,  8.52411894e-02+5.18029751e-02j, -8.02700769e-02+1.90210044e-01j, -2.34714253e-02-2.31241051e-02j, -3.19869719e-01+4.57770182e-02j, -4.75928400e-02+1.12895685e-02j, -3.75738567e-02+6.21213961e-02j,
       -1.14920013e-01-2.12107989e-03j,  4.77491260e-02-5.19463759e-02j,  1.03806006e-01+7.16366977e-03j, -8.27636909e-02-5.08812397e-02j,  2.22322180e-02+1.43361348e-02j,  3.14670279e-02+1.39911830e-02j,  8.48418379e-02+1.81601566e-01j, -7.46942903e-02+7.89752532e-02j,  7.08970727e-02-6.91124197e-02j, -1.13741776e-02+1.23825472e-01j, -6.59798634e-02+1.24967591e-01j, -6.33113532e-02-1.64307450e-01j,  1.14099231e-01+3.41222046e-03j, -4.61570543e-03-2.19001396e-01j,  2.84759810e-02-7.17797193e-02j, -5.39314225e-02-1.64591007e-01j, -1.16438285e-01+6.52426253e-02j,  5.86854248e-02-4.05905194e-02j,  1.47438624e-02+5.30606235e-02j, -1.90230877e-02+4.63842878e-02j,  8.00044760e-03+1.58689396e-01j,  4.42361550e-02+5.13313063e-02j,  2.55319954e-02-9.75583568e-02j,  3.01793771e-02+1.11081312e-01j,  1.27308954e-01+1.69452356e-01j, -4.94643829e-02-3.50989191e-02j,  2.42630119e-01-3.22834423e-02j, -9.43879468e-03+1.54569386e-02j, -1.26451530e-01+1.07634561e-02j,  6.53072639e-02+1.64004515e-01j,
        4.73037239e-02+6.72621484e-02j,  7.86355859e-02+1.37091046e-01j,  3.48468450e-02+5.01002189e-03j, -4.96409372e-02-1.79874187e-02j, -8.08977821e-03-1.60325464e-02j, -6.18986724e-02-9.03950614e-02j, -8.57358123e-03-1.05211425e-01j, -1.19635684e-01-9.84001742e-04j, -9.29477188e-02-8.10903125e-02j, -6.60847827e-02+1.07380011e-01j,  6.65522847e-02+2.25450350e-01j, -8.20410767e-04-1.28445752e-01j,  6.14568901e-02-4.79238345e-02j, -1.06893529e-01+1.36044214e-01j,  1.15138674e-02-4.90252547e-02j, -2.76216857e-02+6.76507128e-03j, -1.09382658e-01-5.30334315e-02j,  7.08950419e-02+3.44793514e-02j, -7.60439516e-03-1.03572965e-01j, -5.88875512e-02-1.16117780e-02j, -1.76441304e-02+1.51394952e-01j,  1.31994928e-01+4.65974993e-02j, -5.68107027e-02-3.38323177e-02j, -1.00658392e-01+9.28555205e-04j, -7.05082372e-02-1.20465449e-01j,  6.60549798e-02-1.08205211e-01j,  1.56747849e-01+1.21419663e-01j, -1.92613366e-01-1.52102819e-01j,  1.13592085e-02+2.83411765e-02j,  5.99946763e-02+1.22827499e-02j,
        6.29184081e-02+1.00151916e-01j,  2.10898432e-03-1.59215144e-02j,  1.66631398e-02+1.18954419e-01j,  8.45398912e-02-1.10186016e-01j, -1.19940961e-01+9.68249560e-02j,  4.70304208e-02+7.70342680e-02j, -1.22124543e-01+4.39697557e-02j, -9.96175775e-02+1.38251492e-01j,  6.97555402e-02+1.33210859e-02j,  2.56989939e-01+1.26134536e-01j, -3.26770849e-02-1.47901277e-02j,  2.42109734e-02-1.35712836e-01j, -1.47972097e-01+4.14172508e-02j, -5.11927028e-02+1.22448971e-01j,  6.62366501e-02+5.95513442e-02j, -5.32415834e-02+3.72035869e-02j, -1.52278830e-03+1.27320604e-01j,  6.50991303e-02-1.55212986e-01j,  4.34075807e-02+1.02149453e-02j,  4.30717490e-02+9.20139462e-02j,  1.16886206e-02-1.87138194e-01j, -6.64847366e-02+1.09856866e-01j, -3.67840574e-02+6.02029631e-02j, -9.81671194e-02-7.20999183e-02j, -6.04451121e-02+7.52633656e-02j,  2.81976119e-03+1.06633536e-01j,  5.26129880e-02-3.98349053e-02j, -2.87998493e-02-7.12947090e-03j, -3.94344408e-02-1.20091657e-01j,  8.38152379e-02+5.55474575e-02j,
        9.41964471e-02+2.98721048e-02j,  8.16069823e-02-2.59474209e-02j,  1.55022162e-01+1.57407270e-01j, -5.67424378e-02-1.25923094e-01j, -2.06889103e-02+1.23693612e-01j,  8.49745423e-02+6.37954576e-02j,  7.18887232e-02+1.75903579e-01j, -6.08588164e-03+2.03191283e-01j,  6.86348025e-02-2.78848587e-01j, -4.14352560e-02-7.92352365e-02j,  1.99800580e-02-1.07011810e-01j, -4.77750533e-02-4.29164779e-02j, -9.80129418e-02+2.26179330e-01j,  2.44211579e-03+8.87315263e-02j, -2.48518186e-04-2.58477922e-02j,  1.46163656e-02+2.66665629e-02j, -1.08518799e-01-1.22626080e-01j, -4.62433206e-02+6.55029395e-02j, -9.33013316e-02-1.22087077e-01j, -1.31539219e-01-2.41402172e-02j,  2.25238757e-01-4.21429135e-02j,  6.27985064e-03-1.07195959e-01j,  4.34640320e-02+5.35187773e-03j,  7.32060940e-02-8.45846618e-02j,  1.83126542e-01-1.06954649e-01j,  2.22212559e-03-1.29315970e-01j,  6.01963053e-03-3.51904776e-02j,  1.02955536e-01-3.85218206e-02j,  9.46926580e-02-7.37961277e-02j, -8.61772814e-02+1.07443737e-01j,
        2.04379335e-03+5.94079303e-02j,  8.23537953e-02-1.41231083e-02j, -8.33955001e-02-7.33002238e-02j, -4.63586972e-02-7.49582675e-02j,  1.44258446e-01+5.05833339e-02j, -7.26785087e-02+7.99070748e-03j,  9.65967758e-02+2.55373847e-03j,  4.56168318e-02-6.98355136e-02j,  1.34466635e-01+9.33168643e-03j,  1.35620410e-01-3.63893984e-02j, -2.18310783e-01-9.07642078e-03j,  2.41046715e-02+8.68133750e-02j,  8.46941220e-02-9.87087721e-02j,  2.32699075e-02+7.70324105e-02j, -6.83066191e-02+6.26029815e-02j,  3.96917401e-03+3.19047344e-02j, -1.93167112e-02-5.87602583e-02j,  6.40947853e-02+3.49325001e-02j, -8.48481277e-02+1.00080104e-01j,  1.43703227e-01-2.70626163e-02j, -1.56896525e-01-4.98153190e-02j, -1.06529060e-01+1.09204216e-01j,  1.34564483e-02-2.22232431e-01j, -5.20802942e-02-5.67593041e-02j,  6.29414647e-02-1.10240712e-01j, -1.54707424e-02-6.35737164e-02j,  8.94634583e-03-1.06767329e-02j, -4.21134431e-02-3.53658066e-02j,  2.41746297e-02-3.68212148e-02j, -9.21559781e-02-2.99118897e-02j,
       -5.89654642e-02-2.28852318e-02j, -5.20985361e-02-1.16771130e-01j,  5.35843151e-02-1.32214156e-02j,  4.58936445e-03+3.34103858e-02j, -1.46128686e-03-7.50434059e-03j, -1.37118260e-01-2.96979592e-02j, -4.28133426e-02+3.29193671e-02j,  1.22565726e-01-6.37325294e-02j, -5.96435200e-02+6.19807323e-02j,  8.28472218e-02+1.93521885e-02j, -8.88354299e-03-1.16922356e-01j,  1.17046722e-01+7.55087525e-02j,  1.11458497e-01-4.97578367e-03j,  3.03775840e-02-6.26278785e-02j,  9.34835767e-02-8.35671209e-02j,  1.92817644e-01+6.09168886e-02j,  1.55965719e-02-3.49551218e-02j, -9.23258088e-02+2.02094665e-01j,  7.07259110e-02+6.29610500e-02j,  2.65355792e-02-1.45360966e-01j, -8.38484133e-02-2.25673551e-02j, -5.99920073e-02+1.19442828e-01j,  3.08096147e-02+2.05383373e-01j,  1.46680157e-01-1.34303106e-01j,  1.78416469e-01-2.56761475e-02j, -7.04073547e-03-4.26410412e-02j,  5.92741489e-03+2.73194162e-02j,  3.28330300e-02+1.38975663e-01j,  1.36088703e-01-7.00656141e-02j, -7.16088585e-02-5.01751384e-02j,
        1.07261982e-01-6.08676344e-02j,  3.29984881e-02-4.11527154e-02j, -1.22180830e-01-1.79219988e-01j,  1.52540838e-01-3.98900946e-02j, -6.59340891e-02-1.13945351e-01j,  3.26860116e-02+7.75988849e-02j,  1.84680188e-03-7.15874930e-02j,  5.75188825e-02-1.59627565e-01j, -1.45799524e-02+1.40916322e-02j, -1.89600216e-02+2.20952202e-01j,  3.67478575e-02+1.30282642e-01j, -9.75462649e-02+1.15756251e-01j,  1.16468796e-02+1.17487817e-01j, -2.76444195e-03+7.54195399e-02j,  1.01904254e-01+1.43117381e-02j,  6.03665982e-02-8.65537954e-03j, -1.32384893e-01-5.91759860e-02j, -1.26867457e-01-1.99641121e-02j, -7.35718904e-02+7.37721086e-02j, -4.37046524e-02-5.73821921e-02j, -4.88920306e-02+4.85533481e-02j, -2.58084890e-01-4.52105731e-02j,  8.83268017e-02-3.24794517e-02j,  3.97109460e-02+2.31464221e-02j, -8.74443503e-02+5.05459657e-02j,  3.49357665e-02+5.74013249e-02j,  7.76023292e-02+1.24157281e-01j,  4.67657992e-02+8.54501007e-02j, -2.73903139e-02+3.20710850e-02j, -7.22744938e-02+2.98218349e-02j,
       -1.66836536e-02-5.55139728e-02j, -1.00817364e-01+1.98738235e-02j,  1.06000799e-01-4.17864489e-02j,  2.47089963e-02-6.89030059e-02j,  1.02071912e-02+1.40803829e-03j, -4.29064727e-02+1.48247071e-01j,  8.86549012e-02-1.49688762e-01j, -1.37487726e-01-6.88685909e-02j,  9.74446290e-02+3.34782789e-02j,  3.09376006e-02-8.18948969e-03j, -6.07821142e-02+2.05709303e-02j,  6.20301019e-03-1.07661389e-01j,  2.66360798e-02-1.95200123e-02j, -8.70121352e-03+6.27192787e-03j,  5.67467889e-03-4.71688645e-03j, -3.96568337e-02+2.60335168e-03j,  1.26272851e-01-7.88612171e-02j,  2.39896969e-02-1.94449288e-01j,  1.01939995e-01+5.40381344e-02j,  3.38010121e-02-1.07535239e-01j,  1.01694129e-01+1.58769881e-01j, -1.60525228e-01+1.61402777e-02j,  2.46147533e-02+1.84478112e-01j, -2.24323874e-02+1.44766330e-01j, -7.44818491e-02+1.10475796e-01j, -7.03647417e-03-7.89877995e-03j, -7.12605582e-02-6.00421868e-02j,  8.19735532e-02+3.10232290e-02j,  1.29199728e-01+8.64579318e-03j,  1.04093008e-01-2.16880641e-02j,
        2.62815057e-02-3.82377454e-02j, -1.62557224e-01-1.01320720e-01j, -5.68005830e-02-6.94736514e-03j, -1.49055391e-01+5.18207090e-02j,  6.27178782e-02+1.01489809e-02j,  1.25981379e-01-8.58405608e-02j, -8.20377419e-02+8.24372614e-02j,  6.37946588e-02+8.57050701e-03j,  6.54024538e-02+4.63840214e-02j,  6.18224834e-02-2.22489702e-02j,  1.17291795e-01-7.38374742e-03j,  1.48954910e-02-3.86650743e-02j, -3.77589928e-02-1.39936721e-01j,  4.52316221e-02-1.57936117e-01j, -6.23943129e-02+4.81748404e-02j, -1.92620084e-02-2.15996053e-02j,  1.71279612e-01+7.73412683e-02j,  2.23232062e-02-5.61065685e-02j,  1.57779087e-01+7.85148897e-02j,  1.40230327e-03+6.35368648e-02j,  8.38305347e-02+1.63118880e-02j, -9.03278369e-02+1.01139458e-02j,  3.54020248e-02+1.78314505e-02j, -1.70735336e-01-1.07777517e-01j, -3.34738370e-02+7.44591431e-02j, -2.16874208e-02-2.16863527e-02j,  1.15546757e-01-1.47632133e-01j, -1.64688932e-01+4.66255948e-02j,  5.97823744e-04-2.02720511e-02j,  2.98168578e-02-9.41198035e-02j,
        6.21270773e-02+6.40834784e-02j,  2.58933539e-02-2.42667870e-01j,  5.70136258e-02-3.77350337e-02j,  1.98579632e-01-9.20701075e-02j,  9.55640150e-02+3.54843433e-02j,  7.45928560e-02+4.26588872e-02j,  6.09447733e-02+3.16539556e-02j, -1.80171093e-01+5.60133264e-02j,  9.40854193e-02+4.31389113e-02j,  3.03850816e-04-2.85530728e-02j,  5.41410801e-02-6.13161714e-02j,  9.12148494e-02+1.80281943e-01j,  5.09948266e-02-3.86997536e-02j, -1.91113131e-02+1.94498984e-01j,  2.08692927e-01+6.43547140e-03j,  5.20633056e-03-1.90022093e-03j,  3.62398834e-02+1.08762806e-01j,  1.33590408e-01-1.61775350e-02j, -9.28966408e-02-5.91354052e-02j,  3.90924936e-02+3.49619784e-02j, -1.10597598e-02+1.62593761e-01j, -1.19019882e-01+1.11694210e-01j,  7.64944486e-02+8.29783985e-02j,  3.19470164e-02-2.07941529e-01j, -6.16504642e-02-5.17749153e-02j, -9.19641302e-02+1.26001118e-01j,  7.48796227e-02-5.35446337e-02j,  8.61395046e-02+7.66827772e-02j,  5.67446200e-02-1.44510489e-01j, -3.64513131e-02-9.95038225e-02j,
       -5.23694938e-02+1.68673670e-01j, -9.98489889e-02+4.49214566e-02j, -2.62137478e-03+5.31844467e-02j, -1.97293197e-02-1.01779130e-02j,  6.56118297e-02-1.39819961e-01j,  1.07078888e-01-9.60455951e-02j,  4.04114157e-02-2.75185153e-02j, -1.06731160e-01+2.45576423e-02j,  3.97224538e-02+4.80422979e-02j, -1.41155662e-01+5.37056668e-02j,  3.94804097e-02-5.91553615e-02j,  1.02421169e-01+1.97936296e-01j, -1.01728301e-01-1.38399695e-02j,  8.76776513e-02+1.40003856e-02j, -1.53801875e-02+2.04027278e-02j, -8.81370764e-02+7.21575549e-02j,  2.88433274e-02-1.42749089e-01j, -6.16490703e-02-2.20430462e-01j, -1.98880578e-02-7.67322432e-02j,  5.79127107e-02-1.00419699e-02j, -1.09662385e-02+1.26745293e-01j,  7.36261795e-03-1.62045074e-02j, -1.45951390e-01-8.98045669e-02j,  1.04518882e-01+7.87311257e-02j,  1.84677017e-01+2.57397149e-02j,  9.20387231e-02+4.51211489e-02j, -8.11699179e-02+6.13269405e-02j, -7.54782810e-02-1.53812051e-02j, -2.27439779e-02+5.40599197e-02j,  5.27509423e-02+4.94672673e-02j,
        6.01908515e-02+7.08980366e-02j,  7.29386254e-03-4.35495079e-02j,  2.59865264e-02+5.23991312e-03j, -2.80790961e-02-3.17079888e-02j,  3.06137050e-02+7.10525322e-02j,  1.98725682e-01+1.63806997e-01j, -3.85808044e-02+3.17497500e-02j, -8.38991266e-02-8.29634220e-02j,  1.14568111e-01-2.51524437e-01j,  1.89161580e-01+1.22405055e-01j,  2.67369898e-02-6.29372223e-02j, -5.55112811e-03+8.24912321e-02j,  3.06911272e-02+1.24362478e-01j,  6.90052196e-02+4.78403406e-02j,  3.32511878e-02-5.54954803e-02j, -6.83065225e-02-9.83139293e-02j, -2.24205878e-03-1.08330998e-01j,  1.32158911e-01+6.21895108e-02j,  6.04041228e-02+2.26998786e-02j,  1.19910251e-03+2.40133006e-02j,  1.95384495e-02+7.25223079e-02j, -7.51114128e-02+7.50023940e-03j, -6.91753048e-02-1.84426984e-01j,  8.93361350e-02+6.14020197e-02j, -1.11027212e-02-2.73006053e-02j, -1.58027594e-01+1.12196684e-01j,  6.13191254e-02+2.98921562e-02j,  9.36940475e-02+1.74832860e-01j,  3.54488913e-02-1.86012865e-02j,  1.18886717e-01-5.60421606e-03j,
       -5.08023225e-02+1.58187433e-01j,  2.05543121e-02+2.36229478e-02j,  7.56607990e-02-9.20635412e-03j,  3.04571802e-02-5.32735635e-02j,  1.05681520e-02+7.99310770e-02j,  1.00900200e-01+3.39036295e-02j,  6.00254366e-02+1.29251252e-01j,  1.73747341e-01-3.50700834e-02j,  8.68889594e-02+4.32124921e-02j,  1.20809063e-01-9.57225859e-02j, -5.08827469e-02-1.56682461e-01j, -5.51578706e-02-1.60825072e-01j,  7.75492052e-03+3.62310778e-02j,  2.70830573e-02+3.42352710e-03j, -3.35299324e-02-3.07678678e-03j, -2.09542652e-02-8.28876236e-02j,  1.67136215e-03+9.77623412e-02j, -8.33024445e-02+9.68319454e-03j, -1.27694754e-01-3.43756377e-02j, -1.05215118e-03+1.09484097e-01j,  5.77644996e-02+2.26376332e-02j, -2.27042043e-02+4.64923429e-02j, -7.52065556e-03+8.32320454e-02j, -3.45650834e-02-9.36227416e-02j,  1.58893895e-02-5.88622612e-02j,  6.75234381e-02+1.17750436e-01j,  9.91136713e-02-4.90548054e-02j,  1.14744296e-03-2.03805309e-01j,  1.26681748e-01+7.20946491e-02j,  1.72091738e-01+9.45687729e-02j,
       -2.23781476e-02-3.05658385e-02j, -4.79846553e-02-9.53644599e-02j,  8.98978713e-02+3.73340639e-02j,  9.71904756e-02-5.07401492e-03j,  4.70042328e-03+2.98136201e-02j,  3.20602243e-02+8.90046751e-03j,  2.91870381e-01+1.56310942e-02j,  1.47571000e-02+1.74082211e-01j,  1.98727480e-01+1.81545936e-02j,  5.76140047e-02-1.37512019e-01j,  2.84157891e-02-8.90169341e-02j,  2.58144712e-02-8.66533513e-02j,  1.26007423e-01-1.42353852e-04j,  1.22011887e-02-1.50599335e-01j, -1.03678605e-01+1.77343389e-02j,  1.01880582e-01+1.02053135e-01j, -6.89911020e-04-1.34757213e-01j, -4.67669409e-02+5.62588217e-02j,  5.42306518e-02-4.70868860e-02j,  1.87819392e-02+5.88306140e-02j, -1.39180077e-01-5.74473723e-02j, -7.71319085e-02+1.05052464e-01j, -1.76084377e-01+4.53754001e-02j,  5.78987630e-02-1.39030598e-01j, -1.36379204e-03+1.03197101e-01j, -1.29842079e-01+6.46203124e-02j,  3.63331025e-03-1.52651696e-03j, -9.63370297e-02-5.23631453e-02j,  7.11523907e-02-7.71438142e-02j, -3.33129359e-02+4.04006262e-02j,
        2.49142373e-02+1.48066271e-01j, -2.26183059e-02+7.66734818e-02j, -5.45495074e-03-1.19942778e-01j,  9.13909578e-02+4.79952618e-02j, -5.35918216e-02-1.60654037e-02j,  5.98871312e-02-1.87532022e-02j, -1.12910440e-01-2.78610231e-02j,  1.46838755e-01+9.92565647e-02j, -4.13814974e-02+7.56087587e-02j, -4.88973691e-02+3.13114227e-02j,  7.56532879e-03+9.34644447e-02j, -2.67290794e-02-3.42410745e-02j,  8.46376955e-02-1.22432585e-01j, -3.13201588e-02-3.59657622e-02j,  1.52126152e-01+9.91205244e-02j, -9.45677226e-02-8.05276033e-03j,  5.46044728e-02-2.63756752e-01j,  1.95345701e-01+1.33669072e-02j, -3.25932513e-03-5.43829049e-02j,  1.53950498e-01-1.42848111e-01j,  1.48524300e-01+8.79890137e-03j,  1.17396603e-01+2.64318281e-02j, -2.62384138e-02+1.38751108e-01j, -9.52598923e-02+7.62648401e-02j, -1.80631676e-01-1.35669199e-01j,  8.51086298e-03-1.10183317e-01j, -8.83923576e-02+1.37131346e-01j, -3.89492392e-02-8.90366047e-02j,  1.90119483e-02+4.19438222e-02j,  1.12797080e-01+7.86580908e-02j,
        7.80716814e-02-9.09945481e-02j,  8.64121030e-02-7.22343210e-02j, -6.86716918e-02+1.03223441e-01j,  9.38556460e-02+7.85191865e-02j,  3.12389310e-02-8.62865526e-02j,  1.19593208e-02+6.74798173e-02j, -9.97752646e-02+4.73740307e-02j,  8.50920494e-02+1.21420677e-01j, -3.36867683e-02+7.31079808e-02j,  7.80684211e-02-1.11358256e-01j, -1.03606051e-01-1.00017901e-01j,  7.46662567e-02-4.58176146e-02j, -1.77215554e-01+5.14045000e-03j, -7.35851406e-02-5.88169005e-02j, -2.34091949e-02+5.56784850e-02j, -1.92734776e-01+5.23357550e-02j,  1.20534005e-01+6.92412538e-02j, -3.03040274e-02-2.43467856e-02j,  1.60451360e-01-1.02238503e-02j,  6.15204165e-02-1.94133488e-03j, -2.33847089e-02-8.66114982e-02j,  4.19745705e-02-4.99401327e-02j, -1.03928808e-02+8.20517312e-02j, -1.72244615e-03-4.42743317e-02j,  1.67168913e-01-3.07829598e-02j, -9.92849889e-02+4.65088178e-02j, -5.21084550e-02+2.70599004e-02j, -1.44242685e-01+1.42099698e-01j,  1.31564015e-01+6.03947617e-02j,  7.79295470e-02+1.16599297e-01j,
       -1.44508140e-01+3.90326398e-02j, -1.63189060e-01-4.80102773e-02j, -4.23892524e-02+7.24586719e-02j, -1.08354291e-01+2.46926730e-02j, -3.31000925e-02-2.02469979e-02j, -1.06440911e-01+5.74076867e-02j,  2.98674990e-02-2.56460099e-02j,  3.27784317e-02-1.09277698e-01j, -1.11743589e-01-5.16542702e-02j, -4.38449856e-03+1.25111334e-02j,  4.20069024e-02+6.10391773e-02j, -8.31398473e-02-3.81548729e-02j,  1.09141869e-01+1.50926344e-01j, -4.57654467e-02-6.46747717e-02j, -8.54458173e-02+1.86067568e-01j,  6.94294608e-02-8.10150126e-02j, -5.70344424e-02+7.83521518e-04j,  3.21090931e-02+9.47319893e-03j, -6.02959548e-02-1.01689715e-01j,  1.25328049e-01+1.14694700e-02j, -3.84840481e-02-8.31927703e-02j, -6.76446553e-02-1.86440782e-01j, -9.65863639e-02+4.30146576e-02j,  1.86429327e-01+1.77285461e-02j, -2.79125197e-02+1.55599615e-02j,  9.66942708e-02+5.05933675e-02j, -1.96844343e-01-9.05549515e-02j, -1.06330695e-01-1.29754415e-01j,  9.69592860e-02+7.87237719e-02j, -3.20763173e-03-6.01830255e-03j,
       -5.75846436e-02-3.07527027e-02j, -2.15090340e-02-1.13335909e-01j,  2.05999323e-02-1.64092718e-02j,  6.20217109e-02-7.21962400e-02j,  2.52569512e-02+8.67133723e-02j,  6.73469570e-02+5.06801423e-02j, -4.47799880e-02-1.27433227e-01j, -2.43147056e-02-4.19481682e-02j, -3.99145646e-02+1.36650217e-01j, -1.17600891e-02-3.67255385e-02j, -2.41082251e-02-1.48703792e-01j,  1.45917987e-01-2.09551629e-02j,  1.12455567e-01+4.32809709e-02j, -1.41748964e-01+8.50598353e-02j, -3.11770116e-02-5.18172723e-02j,  1.83579810e-01-7.06905345e-03j,  9.71116808e-02-7.25783826e-03j, -6.06413346e-02+1.83309774e-02j,  1.32067152e-01-8.04329623e-02j, -3.70830182e-02+3.25924415e-02j,  5.99945661e-02+1.39581640e-01j,  4.95057896e-02+1.41585780e-03j,  1.27163533e-01-2.79494886e-02j, -1.12806297e-01-1.38597417e-01j, -1.70331848e-01-6.41623156e-02j,  1.13320890e-01+4.80817353e-02j,  2.35832464e-01-5.92082391e-02j,  8.74837897e-02-2.86661849e-02j,  6.11698809e-02+6.06877492e-02j,  4.24028809e-02-6.62392146e-02j,
        4.30315505e-02-2.17789106e-01j, -2.72931426e-02+1.52670758e-02j, -3.82654806e-02+9.56802873e-02j,  2.43401149e-02-1.78124583e-02j,  9.02901686e-03-1.65710546e-01j,  8.04870293e-02+5.50553769e-02j, -4.69870057e-02+3.41343271e-02j, -3.50577896e-02+7.23143438e-02j,  1.10225646e-01-2.15752004e-03j, -9.61299965e-02-4.18347593e-02j,  8.68479349e-02+4.70126112e-02j, -1.07545082e-01+2.49148829e-02j, -4.53040726e-02-1.06734716e-01j, -1.59436033e-02+8.10286351e-02j,  2.02915812e-02-6.81132744e-02j, -6.08982393e-02+8.40773887e-02j,  1.50191978e-01-6.42097008e-02j,  1.35274591e-01+1.57431907e-01j,  3.53273620e-02+7.14369281e-02j,  1.81320909e-02+7.20914621e-02j,  6.03721972e-02+1.96226127e-01j, -3.18042660e-02-3.39904414e-02j, -1.19303840e-01+6.14160382e-02j, -1.76524156e-02-8.04986522e-02j,  1.76157967e-02-1.36875912e-02j, -4.49031451e-02-1.89495355e-01j,  1.26341022e-02-4.84818365e-02j,  7.93666935e-02-1.20859425e-01j, -9.00060617e-02+7.53951193e-02j,  6.15384669e-02+1.81702718e-01j,
       -1.96562939e-02+6.02270326e-02j, -1.53637264e-01-3.42216746e-02j,  5.82845366e-02+1.35589726e-01j, -9.69854953e-02+7.81307161e-02j,  9.47424015e-03-7.67839223e-02j, -5.21777176e-02+1.18463908e-01j,  1.08597440e-02+7.34720754e-02j,  1.25753709e-02-4.59227875e-02j,  4.66065304e-03+3.16832178e-02j, -1.70642799e-01+1.09175443e-01j, -1.07616067e-01-6.77062304e-02j, -4.94690131e-02-1.33454471e-02j, -1.13671465e-01-1.13124774e-02j,  1.79311462e-01+1.36206373e-02j, -5.44546549e-02+2.39954931e-01j,  1.53720058e-01+8.17470362e-02j, -1.65785647e-01+4.99276923e-02j,  1.24051085e-01+2.08575029e-02j, -1.27344695e-02-1.00323698e-01j,  1.26323841e-01-1.75232843e-01j,  1.42829963e-02+9.51676108e-02j, -1.36837621e-02-5.10102254e-02j, -1.09035252e-01+3.04699753e-02j, -5.73977327e-02-9.67364763e-02j, -4.18955867e-02-7.26363284e-03j, -1.11110425e-01+9.11072318e-02j, -3.33402440e-03-9.81640668e-02j,  5.00153545e-02+2.96772715e-02j,  8.34464143e-02-1.07141828e-01j,  1.66627515e-02-1.76768451e-01j,
        3.01046178e-03+4.29920679e-02j,  3.90382225e-02+6.48627085e-02j, -1.29633542e-01-1.21639020e-01j, -1.81455901e-02+1.81332342e-02j,  1.70046497e-01+9.18523538e-02j, -7.49819636e-04+2.57010470e-02j,  6.14006255e-03-7.49534371e-02j,  5.73079131e-02+6.45021320e-02j, -1.25906199e-01+3.61504866e-02j, -4.14229989e-02-2.91283203e-02j,  3.22455405e-02+8.63451878e-02j,  1.23237528e-01-1.89968509e-01j, -3.05088062e-02+1.73009137e-02j,  2.14194747e-03+5.29985319e-02j,  6.96329432e-02+2.60627319e-02j,  1.57695179e-01+8.59454119e-03j,  7.26680354e-02+7.21484481e-02j,  7.00651142e-03-2.90372350e-03j,  2.90015482e-02-5.68542701e-02j, -1.27693027e-01-1.14964625e-01j,  1.50753348e-02-1.36610446e-01j, -1.47676083e-01+8.13290718e-02j,  9.24954421e-02+3.54388660e-02j,  1.81954041e-01+1.07582710e-01j,  1.15127848e-01+5.98445734e-02j, -6.47767365e-02+1.22084642e-01j,  5.54779597e-02-2.11336155e-01j,  2.33200724e-02+3.44422083e-02j, -3.99506520e-02-1.46402335e-01j, -3.39094996e-02-2.32080319e-02j,
        6.38032383e-02+4.58309555e-02j,  9.80627186e-02+6.74309177e-02j,  2.09243817e-01-9.21844569e-02j,  1.16068394e-01+3.58375950e-02j,  2.34470731e-02+1.34662899e-02j, -4.87953052e-02-3.39548474e-02j,  1.77551778e-02-7.33967642e-02j, -4.28970865e-02+2.14591815e-02j,  1.36029417e-01-9.66574708e-02j, -3.16822200e-02+1.06454016e-01j, -1.93320117e-01+1.76505036e-02j,  1.09916758e-01+1.27398554e-01j, -1.65850940e-01+7.04898942e-03j, -6.53608621e-02-1.87281647e-01j, -1.57179762e-02+3.65905427e-02j, -4.77377390e-02+5.01770606e-02j,  7.38286054e-02+5.86402502e-02j,  2.00220886e-01+2.14556567e-02j, -7.99321902e-02+1.20661845e-01j,  2.25130739e-02-7.92813095e-02j,  9.39404226e-02+4.23514982e-02j,  5.59759953e-02+5.29013915e-03j,  1.42272381e-01-2.03889918e-02j, -7.70692925e-02+2.18788591e-02j,  9.46416245e-02-5.40567083e-02j, -1.16864668e-01-1.17453747e-02j, -6.80245475e-02-8.85472661e-03j, -5.07648557e-02-1.41856238e-02j, -6.57054741e-02+4.87753999e-02j,  6.79140250e-02+7.57999993e-02j,
        8.31558858e-02+4.88031390e-02j,  7.04245127e-02+5.11799739e-02j,  1.92019820e-01+1.63346011e-01j, -1.08430098e-01+2.88148366e-02j,  2.15534804e-02-2.40090002e-02j, -2.83413932e-02+7.28009270e-02j,  2.77128550e-02-7.17129530e-02j,  4.43451848e-02+4.62015889e-02j,  2.29868386e-02-1.58564488e-01j, -1.80745027e-01-3.09626852e-02j, -5.25473622e-02-5.09485093e-02j,  4.13635441e-02+2.95240616e-03j,  3.46266625e-02+3.40501706e-02j, -1.36957085e-01+5.93795536e-02j,  1.71716569e-01+6.62369454e-02j,  2.00789628e-02+1.02579168e-01j, -2.49268976e-04+4.05793649e-03j, -8.70948659e-02+2.67855934e-02j,  9.58058704e-02-3.92343326e-02j,  6.37344353e-02+2.91954856e-02j, -1.62573041e-02+1.44050769e-01j, -2.12950381e-01+1.05380057e-01j,  1.54420103e-02-1.58531345e-02j,  4.62275644e-02-1.44131724e-02j,  9.63943826e-03+1.43294871e-01j, -6.82715621e-02+7.72801050e-02j,  1.19392259e-01-4.32006940e-02j, -9.25898691e-02+1.73642675e-01j,  7.87258333e-02-5.00345685e-02j,  6.74252100e-02-3.22021492e-03j,
       -1.05938764e-02-4.80682200e-02j, -3.79109088e-02-7.18862647e-02j, -7.85329094e-03+5.08832953e-02j, -1.22207965e-01-2.08985568e-01j,  3.86004176e-02+9.75474606e-02j,  5.47838699e-02-2.51857867e-03j,  7.32135146e-02-2.02847686e-02j, -9.54663596e-02-1.51302713e-01j,  1.17232603e-01-8.67695572e-04j,  1.13627781e-02-2.45710953e-01j,  1.21005570e-01-1.25331270e-01j, -4.93568188e-02-6.26654349e-02j, -1.10411812e-01+1.72674386e-02j,  7.51406502e-02+1.35621387e-01j, -9.34619657e-03+4.06835362e-02j, -1.69436966e-01+2.11265428e-02j,  9.44593750e-02+9.45454295e-02j,  6.29973849e-02+7.81106789e-03j, -2.31884880e-02-6.08022409e-02j, -1.28506085e-02+4.57155954e-02j, -1.78724877e-01-8.40636436e-02j, -2.74516446e-03+6.87567686e-02j,  1.66154851e-01+2.97763171e-03j,  8.62945953e-02+1.33624387e-01j, -1.23971712e-01+1.06764084e-01j, -8.40377309e-02-4.45037883e-02j, -3.26308419e-02-1.03329380e-01j, -1.06682028e-01+9.15113745e-02j,  1.13743906e-01-7.02685477e-02j, -1.06680734e-01-1.29465759e-02j,
       -6.97277691e-02-1.31064309e-01j, -1.10091153e-02+2.23611713e-01j, -4.61136052e-02-8.17268905e-03j, -1.69987303e-02-9.10722748e-02j,  2.79034389e-02-4.16172595e-02j,  1.24405356e-01-5.92755001e-02j,  3.02072420e-02+3.43359378e-02j,  1.10650997e-01+6.44841711e-02j, -9.59093007e-02-1.11423798e-02j, -5.96787368e-02-7.04578949e-02j,  3.28918399e-03+1.32284061e-01j, -3.72764227e-02+1.55273833e-01j,  2.06921775e-02+1.00442523e-01j,  7.21325549e-03+5.71288626e-02j, -2.14453527e-02-4.68398310e-02j, -6.82026389e-02-1.12497749e-02j,  4.92381808e-02+6.02284521e-03j,  7.83207240e-02+2.07448764e-02j, -5.05939363e-02-3.85781833e-02j, -3.35242535e-03-2.19084108e-02j, -3.49651101e-02+1.07847538e-01j, -2.45290850e-03-6.24826573e-02j, -1.51023459e-02+5.37010336e-02j,  6.37016602e-02-1.23760494e-01j,  2.91215038e-02-2.81493758e-02j, -3.39612485e-02+8.42272097e-02j,  7.35061854e-03+4.18985182e-02j,  1.12694901e-01+3.44174546e-02j,  2.75030448e-02-1.15362720e-02j, -6.56825803e-02+7.69914664e-02j,
        2.45522893e-01+1.94445453e-01j,  1.95565169e-02-6.02333458e-02j, -4.23341023e-02-4.08553715e-02j, -1.65720107e-02+1.11636285e-01j,  6.43254764e-02+5.20975294e-02j,  1.16324230e-01+4.30064237e-02j, -2.35493523e-04+7.94067955e-02j, -3.13234546e-02+3.31928019e-03j, -1.21907959e-01+8.94697709e-03j, -5.86229364e-02+5.00083180e-02j,  1.68440179e-01+1.58008999e-01j, -1.09277389e-02-5.63942582e-02j, -4.33874746e-02-9.18770776e-02j, -3.70318230e-02+2.10558944e-02j,  7.28110664e-02-4.99081181e-02j,  1.31921880e-01-4.54760418e-03j,  3.94635300e-02-8.20549821e-02j,  1.48897886e-01-1.45917759e-01j, -1.18265542e-01+3.70289451e-03j,  3.85086358e-03+1.91440435e-01j,  7.03135105e-02+2.60050811e-02j, -4.21039425e-02+3.01867113e-02j, -1.39706071e-01-2.85504702e-02j,  1.81921714e-03-1.40860236e-01j,  4.20339153e-02+1.95430738e-01j, -2.30135850e-02+3.09767448e-02j,  1.29258195e-01+9.14976478e-02j, -1.58756793e-01+1.28822164e-01j, -3.50036492e-02+8.00893734e-02j,  3.02776094e-02+1.42172163e-03j,
        2.57023140e-03-1.85433098e-02j,  3.72525284e-02+1.05139165e-01j, -9.12678942e-02+2.18607649e-01j, -1.46180909e-01-5.56793105e-02j, -6.56707703e-02-3.19916670e-02j, -7.38160928e-02-1.34669909e-02j,  3.23396348e-02-2.36520392e-01j,  3.30483293e-02-1.02266954e-01j, -4.19639544e-03-6.59678771e-02j, -8.19179151e-02+4.51704171e-02j,  3.09570371e-02-5.32005536e-02j, -5.22790586e-02-3.26500699e-02j,  1.56455025e-01-1.25231125e-01j, -5.05761174e-02+8.32852024e-02j, -2.59554971e-02+4.37847037e-02j,  2.16226296e-01-1.45471321e-01j,  2.65695033e-02+1.26666001e-01j,  4.68381033e-02+1.99629530e-01j, -1.50825778e-02+3.50747365e-02j,  1.26681152e-01-9.75019945e-02j, -1.71499088e-02-2.09581248e-02j,  1.61066501e-01+9.30902661e-02j,  2.60354119e-02+4.88141211e-02j, -6.21761636e-03-4.91509192e-03j,  1.70671987e-03-1.58639200e-02j,  9.95150572e-02-1.68503093e-02j, -3.98240701e-02-2.10542532e-02j, -7.76479029e-02+7.39797509e-03j, -5.77613183e-02+1.49932253e-01j, -1.67637878e-01+2.32658756e-01j,
        4.10173784e-02-4.51688638e-02j,  8.65002182e-02-1.06406855e-01j,  8.46830254e-02-1.72799554e-01j,  1.45422299e-02+2.59367720e-02j,  4.41673701e-02+1.11838918e-01j,  3.26970392e-02+4.00980128e-02j, -5.24772172e-02-7.49030751e-03j, -8.86683859e-02+1.79684474e-02j, -7.43158293e-02+1.44772160e-01j, -1.15935800e-01+6.86790801e-02j,  1.00946401e-02-5.68803310e-02j,  1.40177229e-01-1.19713358e-01j, -1.95130289e-01+5.29709085e-02j,  1.35467559e-01-2.15847353e-02j,  7.09552640e-02-7.66496576e-02j, -8.57302586e-02-1.37409094e-01j, -5.84144286e-02+4.22725006e-02j, -1.13523127e-01-2.08960999e-02j, -1.27785798e-01+2.27385357e-01j,  1.56444767e-03+1.48328917e-02j,  1.82904744e-02+9.01221738e-03j, -4.38128497e-03+4.96827820e-03j, -6.21944033e-02-7.90469632e-02j,  3.44125896e-04-4.79139098e-02j,  8.43927403e-03+4.11354177e-02j,  5.77038887e-02+8.29289688e-02j,  1.22039812e-01+1.12839590e-01j, -1.05530964e-02-1.15582561e-01j, -8.10699111e-02-1.07817350e-01j, -1.20022792e-01+6.78991377e-02j,
        6.48498574e-02-1.02145011e-01j,  2.90657255e-02+3.23596981e-02j, -1.43699949e-02+3.12920727e-02j,  1.71904194e-01-1.42590729e-01j, -5.09140335e-02+3.29766258e-02j, -3.01653038e-02+6.95227171e-02j, -3.76048324e-02-7.94865380e-02j,  6.51687233e-02+1.42580112e-01j, -1.39256310e-01-1.85980964e-02j, -1.10957689e-01-8.32129945e-02j, -1.22937408e-01+1.05433745e-01j,  2.32827863e-01-7.84040766e-02j,  9.68891514e-02+5.50536106e-02j, -7.66146565e-02-1.07485097e-01j,  3.75854655e-02+1.78256175e-02j, -1.05510057e-01+5.39334433e-02j,  2.59293770e-02+5.25056351e-02j,  4.17658585e-02+2.16550525e-02j,  1.79434516e-02-3.79639158e-02j,  4.54758271e-02-1.62169263e-01j, -9.15537346e-02-4.55546358e-02j, -2.87057950e-02-1.51850321e-01j, -4.73431627e-02-4.27936002e-02j, -7.34456848e-02+1.14503191e-01j,  3.21215086e-02-1.16222983e-01j, -1.86452641e-03-1.97148035e-02j,  9.35736392e-02+6.43359360e-03j,  9.90720039e-02+6.49407638e-02j, -1.30861723e-01-1.10397435e-01j, -6.06187143e-02+4.34780133e-03j,
       -2.59141536e-02-8.36660746e-03j,  7.99764395e-02+2.05827485e-03j, -6.79710115e-02+2.19632538e-01j, -8.17267130e-03+1.30090034e-01j,  2.42203491e-02-1.18482312e-01j, -9.65525880e-03-7.27160937e-02j,  1.71710961e-02+3.97131195e-02j,  2.44032297e-02+1.55142147e-01j, -6.95618609e-02+1.14777484e-01j, -3.04082242e-02-2.56808488e-01j, -3.31333938e-02-4.42257523e-02j,  9.09829518e-02-6.41355665e-02j,  1.86082355e-01+8.21534305e-02j,  4.98683358e-02+2.30843433e-02j,  8.69232620e-02-5.72734258e-02j,  9.65089746e-02+4.63738692e-02j,  1.29934009e-01+6.98672714e-03j, -9.55106837e-02-2.11435206e-02j, -1.07437846e-01-4.34196605e-02j,  5.95822062e-02-1.60023155e-01j,  1.45094000e-01+3.62098607e-02j, -4.08020893e-02-3.54469118e-02j, -6.41922431e-02-1.50165876e-01j,  1.37778604e-01+1.57148644e-01j,  1.29772868e-01+3.87413012e-03j, -3.85445235e-04+9.15633467e-02j,  1.46217004e-02-7.02722233e-02j, -9.12225513e-02-1.34467186e-01j, -3.51468484e-02+4.68272859e-02j, -5.28148474e-02-7.05527841e-03j,
        9.85502435e-02-1.44725584e-01j,  4.52535985e-04-4.44306972e-02j,  1.03723988e-01+1.65607843e-01j, -4.21749097e-02-5.54886967e-02j, -9.10799975e-02+7.58681664e-02j, -2.17843207e-02+9.52639644e-02j, -3.30284395e-02+5.94376825e-02j,  9.26534994e-02+2.66802719e-02j,  1.03979128e-02-1.37706931e-02j,  2.33812820e-01+8.59946927e-02j,  6.98746888e-02+9.14639956e-02j, -1.08516120e-01+8.58234842e-02j,  4.21850071e-03+8.71853039e-02j,  6.36273247e-02+7.74673226e-02j, -3.91881255e-02+1.50528746e-01j,  1.09723495e-01+1.04344141e-01j,  7.43803264e-02-1.19685892e-01j,  2.19504862e-02+1.67527045e-01j, -1.60452930e-02-2.00978073e-02j,  6.55173044e-02+2.93255757e-02j, -8.85404950e-02-1.20757531e-02j,  1.14761440e-01-4.57840469e-02j, -5.41528486e-02-5.53916169e-02j,  4.21003566e-02-1.93226229e-02j,  3.45660820e-02-3.52093150e-02j,  1.17626157e-01-1.16988622e-01j,  8.56072498e-02-3.37517615e-02j, -5.90244905e-02-5.24335807e-02j, -5.65206254e-02-2.43446279e-02j,  1.20279495e-01+3.42123716e-03j,
       -3.05654890e-02+1.98042481e-01j,  1.08387964e-02+2.84799289e-03j, -9.91462670e-02-3.82499265e-02j,  5.61732300e-02-2.25129588e-02j,  8.25340529e-02-1.45756514e-02j, -4.43017854e-02-1.98422787e-02j, -2.75902371e-02+2.17979919e-02j, -1.90295706e-02+4.21368188e-02j, -8.80691660e-02+1.15852296e-01j, -3.90388495e-02-2.17922247e-02j,  8.14551313e-02-6.28102543e-03j,  2.79352673e-02+6.35843316e-02j, -3.36457227e-02+1.90578786e-02j, -1.45100657e-02+3.63295744e-02j, -7.00631811e-03-7.40466939e-02j, -2.90520577e-02+7.81154295e-02j, -7.99312633e-02-5.37780863e-02j,  2.30177660e-01-1.83888371e-01j, -2.98717843e-02-1.13899232e-01j, -2.05273534e-02-1.81141663e-02j,  2.96813771e-02+5.94044596e-02j, -3.18971173e-02+7.22385762e-02j,  9.83167960e-02-8.92023743e-02j,  4.25608743e-03+9.09134290e-02j,  1.56893561e-01-4.23935738e-02j,  9.05556547e-02+3.59624971e-02j,  3.15980257e-02-5.46643490e-02j, -1.15196957e-01-8.82593135e-02j, -1.61272928e-01+1.78448561e-01j,  4.00567109e-03+1.76993307e-02j,
        1.17540437e-01-1.36239393e-01j, -6.69383706e-04+5.79926839e-02j, -1.12770659e-01-5.62223146e-02j, -6.21953015e-03-5.98234743e-02j, -4.10342219e-02-5.59744623e-02j, -5.66841235e-02+4.25833208e-03j, -8.23118814e-02+5.33394692e-02j, -1.34412900e-01-1.45272792e-01j,  1.15702329e-01+1.02297961e-01j, -3.19323625e-02+1.39788468e-01j, -2.24923180e-02-6.37168336e-02j,  1.39975574e-01+2.96315967e-02j, -2.05526833e-01-8.05348577e-02j,  1.20419312e-02+1.04877206e-01j, -5.36581838e-03-2.21242391e-01j,  1.43312392e-01+9.03085637e-02j,  1.50230560e-01+9.50231348e-02j, -4.59445185e-03+1.12399647e-02j,  5.16547174e-02+8.01461708e-02j, -9.43895891e-02-3.75397968e-03j, -4.73242216e-02-1.01945370e-01j,  3.44673025e-02+1.96628490e-01j, -9.88840012e-02+8.85464110e-02j, -7.92955362e-02-2.28998407e-02j,  1.69410571e-01+7.44974887e-02j,  6.20173928e-02+1.38106734e-01j,  8.52039046e-02-1.21666670e-01j,  6.84100368e-02-1.00904176e-01j, -1.42596295e-03+1.64268960e-01j, -1.24100632e-01-1.61368165e-02j,
        1.35519012e-02-2.59310892e-02j,  9.05490250e-02+4.84747479e-02j, -4.58662366e-02+1.34930496e-01j, -1.40646551e-01-1.69831134e-01j, -3.16239770e-02-1.48493455e-01j,  1.29115221e-02-4.76191420e-02j,  1.02846452e-01-1.15790746e-01j,  1.59952958e-01+3.50029230e-02j,  5.52164979e-02+5.58300761e-02j, -2.34098143e-02+1.20508448e-01j, -4.62940339e-02-1.16252123e-01j,  7.14945735e-02+8.50853849e-02j,  1.90523893e-02+1.82220632e-01j,  8.47593047e-02-3.87876272e-02j, -1.55367396e-01+9.11388648e-02j, -8.67503920e-02+3.87326858e-02j, -7.47073348e-02+1.13616928e-01j, -8.05356534e-02+7.50838943e-02j, -4.42508043e-02+1.04776551e-04j,  1.91365405e-02+4.59602654e-03j, -5.53849081e-02+9.81402899e-02j,  5.65523118e-02-1.28923219e-01j,  1.09708674e-02-7.17405169e-02j,  5.10870804e-02-4.18029754e-02j, -2.35923190e-02-3.54344675e-02j,  1.05632412e-01-1.52723227e-01j,  2.29356400e-02+1.72003014e-01j, -7.77818089e-02-8.58774571e-02j, -4.12300311e-02-1.40778154e-02j,  3.85331791e-02+1.34956847e-01j,
        9.08599023e-02-2.22765047e-02j, -6.47141773e-02+5.39631501e-02j,  3.78096842e-02+1.20010133e-02j,  6.56218844e-02-6.51650219e-02j,  9.31690801e-04-8.33640650e-02j, -1.79665558e-01+5.36828027e-02j, -7.18696825e-02+5.98082781e-02j, -7.60979310e-02+2.96533884e-02j,  8.38090767e-02+4.64776836e-02j, -1.90640176e-01-2.67445829e-03j, -3.33244747e-02+1.46650575e-01j, -2.87164842e-02-2.38484574e-02j, -1.31368281e-01+8.97392419e-02j,  1.70551482e-01-3.06077010e-02j, -1.23763743e-02-7.87055709e-02j,  1.03033690e-01-1.32308263e-01j,  4.89975988e-02-1.35921611e-01j,  9.42844960e-02+4.92092484e-02j,  1.25975887e-01-6.46248221e-02j,  2.04355365e-02+1.39811567e-01j, -8.13697343e-02+1.30030354e-01j,  1.44452639e-01-4.59256835e-02j,  8.30262490e-02-1.07582941e-01j, -1.01352710e-01+2.50968168e-02j,  3.80275778e-02+7.06296634e-02j,  1.01240183e-01-1.09841276e-01j,  7.93654229e-02+4.00721967e-02j, -1.27232823e-01+1.45125955e-01j, -1.63935684e-01-5.06866305e-02j, -3.33110180e-02+6.38573847e-02j,
        8.29906205e-02+2.45941583e-02j,  7.72777568e-02-2.18729918e-02j, -2.52670750e-02-1.33657547e-02j,  1.23713405e-01+6.60009234e-02j,  1.76105250e-03-5.13327294e-02j,  4.14606615e-02-1.08656071e-01j, -6.28912224e-02-1.07107702e-01j,  1.78069853e-01-3.95319222e-03j, -1.45753269e-03+7.84868919e-02j,  2.06813821e-02-4.02431446e-02j,  9.90237621e-02+1.96007825e-02j,  5.95901068e-02-2.92985389e-02j,  8.53920666e-02+9.47685100e-02j, -9.07144863e-02-2.48034646e-02j, -7.01707091e-02+1.05577150e-01j, -1.42941920e-01+1.06365145e-02j, -1.07249613e-01+2.31065721e-02j, -2.88007686e-05+1.70984097e-01j, -1.33993560e-01+4.60814975e-02j,  6.46602337e-02-1.73007798e-01j,  1.06092053e-02+1.77117336e-01j, -7.41235355e-02+6.72405741e-02j, -4.45379901e-02-2.75973792e-02j,  4.40770486e-02-4.07231141e-02j,  7.43855652e-02+3.34408901e-02j,  2.40780172e-02+7.63837816e-02j, -1.23431650e-02-2.96942615e-02j, -5.36958281e-02-9.09460148e-03j,  4.83507887e-02-3.64673587e-02j, -3.15949181e-03-6.78532281e-02j,
        1.49790019e-01+7.14330627e-02j, -2.59849469e-03+1.08797910e-02j,  2.39996357e-02-2.49046686e-02j, -1.70855065e-02-1.66572246e-01j,  2.50384745e-02-1.73555395e-03j, -8.79488783e-02-4.43380740e-02j,  4.04368200e-02-1.43535828e-01j,  6.99699704e-02+5.53061802e-02j,  1.06809669e-01+3.35034661e-03j,  2.35091058e-01+1.76495810e-01j, -1.04838058e-01+1.73042549e-02j, -1.79707883e-01-4.28597418e-02j,  1.06487618e-01+6.53314431e-02j, -7.12396824e-02-1.91412368e-03j,  1.75034759e-01+7.74238869e-02j, -2.80836905e-02+6.22152256e-02j, -9.72723299e-02-6.42131837e-02j,  1.28855782e-01-8.94346738e-02j, -6.34807923e-02-8.95849148e-02j, -1.76639575e-01+4.70857161e-02j, -8.66457034e-02+8.75818452e-02j, -9.89288233e-05+9.88579156e-02j, -8.42255352e-02+2.32390595e-02j,  5.68654338e-02+9.42482354e-02j, -7.55075952e-02-3.11797796e-02j, -1.77843576e-01+3.45449165e-03j,  5.22159678e-02+1.35185694e-01j, -4.83698045e-02+3.87913053e-03j, -1.51332382e-01-5.08730574e-02j,  9.19052718e-02+8.92932328e-02j,
       -1.04864609e-01+1.59277075e-02j,  1.96860199e-01-1.54596648e-02j, -1.57519029e-01+8.28875556e-02j,  5.32027166e-02+1.80537329e-02j, -5.28489142e-02+4.64273257e-02j,  9.67687293e-02+5.45402170e-02j, -1.09668159e-01-1.19911498e-01j,  1.22173698e-01+7.19935237e-02j, -7.36173466e-02+1.19121401e-01j,  2.12661940e-01+8.30574676e-02j,  1.23393312e-01-1.96849020e-01j,  1.12986422e-01-1.66096746e-01j, -1.11544289e-01-9.50296086e-03j,  1.18875205e-01-9.74865066e-03j, -2.66898723e-02-6.04765594e-02j,  4.87295234e-02-3.00176126e-02j, -1.83928516e-02-1.56550584e-01j, -8.42099875e-02-1.94351327e-02j,  1.64034367e-01-1.73681939e-01j,  1.10112511e-01-7.38060195e-02j,  5.57965855e-02+1.27055482e-02j,  2.16746832e-02+3.08951640e-02j, -1.42130558e-02-2.70500110e-02j, -6.24735154e-02+1.00099685e-01j, -1.50431500e-03+1.77847451e-01j,  1.79109657e-01+4.53034157e-02j,  2.48493244e-01-1.15072362e-01j,  7.02539989e-02-8.21793869e-02j,  5.26929496e-02-4.78438510e-03j,  1.32560795e-02-2.04357332e-02j,
       -1.65817978e-02-3.27041521e-02j,  1.45683239e-02+5.08346595e-02j, -2.16494395e-02-4.55596860e-02j, -1.19823818e-01-7.82211212e-02j,  5.69451958e-02-1.04748797e-01j, -7.35412255e-02-1.04581354e-01j, -1.12094932e-02+9.56866013e-03j,  1.07333598e-01-5.03504846e-02j,  3.98839186e-02+4.91837612e-02j,  1.24111729e-03+5.80244038e-02j, -8.12136032e-02-1.25906033e-01j, -1.47795068e-02-1.41661383e-01j,  7.28039351e-02-1.53666294e-01j, -1.44006427e-02-1.07568591e-01j,  2.89558773e-02+5.20172567e-02j, -3.64643948e-02-1.80813430e-01j, -6.77284194e-02-2.19340382e-02j, -1.51800732e-01+1.26354418e-01j, -2.73948963e-02-8.30809951e-02j, -1.23354114e-01+7.76964520e-02j, -2.26546949e-02-1.00472928e-01j, -4.60740997e-02+2.85918430e-02j,  4.98523677e-02+2.73595143e-02j,  6.84538848e-02-4.23015752e-02j, -1.23786500e-01-1.03759657e-01j, -4.63580185e-02-4.20827278e-02j,  5.00822284e-02+2.97891211e-02j,  1.20486086e-01-1.05685993e-01j, -4.83546445e-02-1.50608054e-01j,  4.41121465e-02+6.52452726e-02j,
       -8.48704931e-02+1.00099989e-01j,  3.64618144e-02-5.84049288e-02j,  1.22257358e-01-2.46209538e-02j, -3.55664948e-02+3.00069552e-02j, -3.74424330e-02+1.03840630e-01j,  6.62161956e-02-1.24088728e-01j, -1.74591936e-02-6.08432847e-02j, -4.30903776e-02+2.28280325e-02j, -1.32783585e-01+2.54915665e-02j,  7.19465639e-02+7.60218114e-02j,  6.47195234e-02-6.25988897e-02j,  4.43633032e-02+1.13628071e-01j, -9.61062592e-02-1.10626660e-01j,  5.02086545e-03+1.71097394e-01j,  2.79457756e-01+3.62937257e-02j,  7.00690199e-02-7.77637776e-02j,  1.79011229e-01-8.68805824e-03j,  1.11655273e-01+1.15277003e-01j,  6.95789332e-02+7.61278729e-02j,  5.46339890e-02+4.89518665e-02j,  4.17643112e-02-9.89158287e-03j,  4.33563805e-02+8.13926965e-02j, -1.21887314e-01-2.12994079e-01j,  6.62822033e-02-9.28126086e-02j,  1.19584184e-01+5.96549891e-02j, -8.66323535e-02-6.73417091e-02j, -1.10895596e-01+1.63722633e-02j,  1.07332452e-01-2.05194027e-02j,  2.33547227e-02+2.92104674e-02j,  1.63241181e-01-3.72177572e-02j,
       -8.50373601e-02-4.58981785e-02j,  1.30399262e-01+6.79769702e-02j, -3.88839698e-02-2.80027695e-02j, -1.42123694e-02-1.13992336e-02j, -3.91191271e-03-8.38363973e-02j, -3.00525620e-02-4.84482749e-02j, -7.07061618e-02+3.40883522e-02j, -2.65451649e-02+7.82186705e-02j,  7.06263189e-02+1.90006089e-01j, -1.06830313e-01+3.57336561e-02j, -1.83318787e-02-2.98391334e-02j, -1.26094265e-01-2.95955671e-02j,  6.69916734e-03-3.29056342e-02j, -4.56437407e-02-3.00362417e-02j, -1.97638423e-02-4.53695081e-02j,  5.62720421e-03+6.57464963e-02j,  1.62152741e-02+5.08849146e-02j,  7.15579817e-03-1.46367176e-01j, -3.66457634e-02-7.32904103e-02j,  1.47588967e-02+1.42117835e-01j, -4.15077413e-02+5.32471543e-02j,  1.81074012e-01-2.15788297e-01j,  1.37761481e-01+1.98205227e-02j, -5.49664338e-02+1.98201091e-01j,  1.06526105e-01+4.25454460e-02j, -1.11587932e-01-1.46278311e-02j, -5.20473894e-03+3.93591568e-02j, -3.98053192e-03-2.77757585e-02j,  8.01758751e-02+5.52518513e-02j, -1.89902375e-02-7.50263130e-02j,
        1.02340693e-01-1.07484115e-01j,  5.97194024e-02+2.99637326e-02j,  4.97079433e-02+9.42118947e-02j, -1.21749063e-01+5.12413635e-02j,  8.86514824e-02+2.02168637e-01j, -6.29512938e-02-5.10894134e-02j,  1.72194219e-02-1.39583523e-01j, -5.59939033e-02-7.47071549e-02j,  5.46002846e-02+1.44836673e-01j, -2.74093589e-02-4.62528590e-02j, -2.06132176e-02+2.54445042e-02j,  5.49369216e-02+1.13085624e-01j,  5.31450092e-02-5.77602131e-02j,  9.76193562e-02+1.59068016e-01j, -2.98195751e-02+4.85882645e-02j,  1.73318274e-02-9.95944267e-02j,  4.46831226e-02-8.05748385e-02j,  1.55069124e-01+1.85506946e-03j,  1.74618570e-01+7.88387737e-02j,  4.88827187e-02-6.57641635e-02j, -4.83705302e-04-3.66495970e-02j, -6.84625471e-03+9.00931899e-03j,  5.99085139e-02+9.55878693e-02j,  4.53412056e-03+4.82660251e-02j,  3.32400505e-02-7.24208945e-02j,  6.90343685e-02+5.16744895e-02j,  2.55541118e-02+2.68896192e-02j, -7.11313208e-03+5.40454970e-02j, -7.37568243e-02+3.56452181e-02j, -1.51159026e-01-3.32725818e-02j,
        2.76447397e-01-9.71246013e-02j,  1.40445833e-01-1.63819214e-01j, -7.47529467e-02+3.29702300e-02j, -5.24187826e-02+1.18661825e-01j,  8.31870820e-02-1.44188935e-01j, -9.53324502e-02+1.03506147e-01j,  3.50388013e-02-2.05399091e-02j,  5.87599572e-02-1.22481738e-01j,  1.06554427e-01+1.09594224e-02j, -2.37403722e-02-4.05386383e-02j,  1.30906014e-01+2.30293496e-02j,  3.42586203e-03+7.21644220e-02j,  4.28718087e-02-1.63183726e-03j, -9.74581387e-02-2.17171279e-02j,  4.79794062e-03+9.69996568e-02j, -2.81152594e-02+3.64640848e-02j,  1.41057044e-01-8.55610176e-02j,  1.13579799e-01-6.86894625e-02j,  3.26039112e-02-3.18036182e-02j, -1.03442985e-01+7.34067502e-03j,  2.45074328e-01-8.02551876e-02j, -9.95980284e-03-1.25597027e-01j, -1.13072351e-01+5.84716931e-02j, -2.91466791e-02-1.12346923e-01j,  1.39940920e-01+4.33022175e-02j,  9.62152966e-02+6.44762437e-02j, -1.78502974e-02+3.61510360e-02j,  1.58364304e-01-4.44919730e-02j,  5.56087485e-02-7.78226361e-02j, -1.28642124e-01+9.25199650e-02j,
       -1.37827615e-02+8.63485029e-03j, -5.90489222e-02-6.21678061e-02j, -8.97089270e-02-5.55332554e-02j,  4.60473118e-04-7.44873712e-03j, -3.01789780e-02+2.62505348e-01j, -5.24922624e-02-1.16998291e-01j, -4.48619660e-02+7.75534691e-03j, -2.16942247e-02-1.32529990e-01j,  2.06845106e-01-3.99952263e-02j,  4.21013041e-03+3.75442281e-02j,  2.08877999e-02+4.65784759e-03j,  1.10316161e-01-1.85375840e-01j,  1.10518071e-01-5.97296649e-02j,  6.06210473e-02+4.13367885e-02j, -3.90943334e-02-8.60356454e-04j,  3.38772694e-02+2.12803762e-03j,  5.50896120e-02-2.37097732e-02j, -9.23532584e-03-2.15778018e-01j, -5.35860008e-02-4.10930146e-02j,  5.10150050e-02-9.19891264e-02j, -3.12179156e-02+5.16581942e-02j, -4.79608112e-02-9.76871529e-02j, -2.24134346e-02+1.11195765e-01j,  1.61216262e-01-9.33898111e-02j,  7.84476912e-03-1.25747019e-01j, -1.28241441e-02+1.46830948e-01j,  1.66002493e-01+6.78124856e-02j,  2.38609868e-01-1.74669970e-01j,  1.00056091e-01-1.91285012e-02j,  4.36951377e-02+3.92636206e-02j,
       -6.33700025e-02+1.00688791e-01j, -9.01457033e-02-5.55666105e-02j, -2.74920713e-02-5.60945304e-02j, -1.12460738e-01+2.08926976e-01j,  1.47746830e-01+3.73544016e-02j, -1.18480516e-01+7.28623868e-02j,  4.33540648e-02+6.69355465e-02j,  4.12853021e-02-5.74198610e-02j, -5.08905281e-02-2.89177495e-02j,  4.39371436e-02+3.64541436e-02j, -1.86339121e-02+2.31353377e-02j,  9.01016810e-02+5.74020539e-02j, -1.46629405e-01-8.14734300e-02j,  8.38912755e-03+8.15995264e-02j,  4.11653085e-02-2.59878024e-02j, -5.12357023e-02+2.53150165e-04j,  1.36426369e-02+2.64118687e-03j, -1.95895595e-02+5.62617047e-02j,  2.47361159e-02-1.29518847e-01j,  3.88829366e-02+2.03908544e-02j,  3.41960599e-03-5.46266792e-02j,  7.46357033e-02-7.37336342e-02j,  2.42879656e-02-8.25850576e-02j, -1.30434368e-02+5.12158348e-02j, -5.86799503e-02-1.20936502e-01j, -3.64627451e-02-1.14700770e-01j,  8.79969190e-02+1.08108855e-01j,  2.86271051e-03+1.42659207e-02j, -4.16354582e-02+8.06660296e-02j,  4.15301680e-02+1.79575574e-02j,
       -1.86461656e-01-2.42490106e-02j,  8.35776870e-02+5.65352490e-02j,  5.17627809e-02-2.14815383e-03j,  6.94949108e-02-1.12716819e-01j,  1.24952171e-01+5.87906220e-02j,  7.17725463e-02-1.19516974e-02j,  1.38777023e-01+1.91666559e-02j, -7.70714927e-02-8.45921195e-02j, -5.50990910e-02+1.88816346e-02j, -4.52069047e-02+9.92559466e-02j,  1.52931927e-01+2.18224356e-03j,  6.36110833e-02-1.49446746e-01j, -1.04306099e-01+2.55936479e-02j, -2.43190570e-02-1.24642498e-01j,  1.07908919e-01-5.24542844e-02j,  1.18023420e-01+1.82251949e-01j, -9.87945471e-04+2.24670038e-02j,  6.88747033e-02+3.88424142e-02j,  4.65620995e-02+6.84585535e-02j,  4.55186370e-02-1.10679652e-02j,  2.45232445e-01+1.21957784e-01j, -1.46216991e-01-6.73000332e-03j,  1.88431703e-01-7.71881296e-02j,  1.64320240e-01+5.78847160e-04j,  9.79316716e-02+4.54526962e-02j,  3.20288139e-03+1.55014950e-01j,  5.47467470e-02+4.35107341e-03j, -2.08130010e-01-5.64307911e-03j, -1.10232566e-01+7.32351151e-02j,  1.35941251e-01+1.38078108e-02j,
       -7.56788025e-02+3.73517234e-02j,  2.04679302e-01+3.90147242e-02j,  1.08785662e-01-6.94594148e-02j,  7.20045941e-02+2.78459276e-02j,  1.92400736e-01-1.50979597e-01j, -1.58869068e-01-8.22425262e-02j, -2.98171534e-02+1.71665472e-03j, -3.06886027e-02-8.02964429e-03j, -4.25762489e-02+3.23637065e-02j, -2.79618445e-02-2.07485498e-02j,  1.18857251e-02-1.75486789e-02j, -4.36645926e-02+6.80482134e-02j,  7.37583273e-03+1.26666694e-01j,  3.95626865e-02-9.77189985e-02j, -1.48716204e-01+2.68786987e-01j, -3.72800597e-02+5.71237210e-02j,  7.41890650e-02-7.52430682e-02j,  3.90385347e-02-2.07079317e-02j,  3.84685342e-02+5.01376534e-02j, -5.27350224e-03+1.47858994e-01j,  3.90452488e-02-8.37384564e-02j, -7.98233712e-02-1.79529336e-02j,  1.33565706e-01+4.92479091e-02j,  3.72930254e-02+1.73346346e-02j, -8.85513745e-02-3.44914310e-02j, -2.55952093e-03+1.10740235e-01j,  1.22295953e-01-6.26298383e-02j,  1.07479323e-01+4.08024277e-02j,  1.94989868e-02+4.87243296e-02j,  1.43449277e-01-1.01261533e-01j,
       -2.16939069e-02-4.18289269e-02j,  6.07804273e-02+9.37183963e-02j, -4.85826992e-02+4.34844713e-02j, -2.86068223e-02+1.03414084e-02j, -6.82958979e-02+7.45791847e-02j,  9.92759338e-03-1.01013890e-01j,  4.46668242e-02+5.34278176e-02j,  2.05957081e-01-8.43994442e-02j, -3.17105069e-02-2.64039322e-02j, -1.23137159e-02-1.67162172e-02j, -7.10187538e-02+1.11568931e-01j,  4.83956551e-02+1.38122479e-01j,  1.06745393e-01+3.55201390e-02j,  5.36566000e-02-1.64529636e-01j, -3.22813703e-02-4.03551165e-02j, -8.16829634e-02-1.59310035e-02j,  9.19194280e-02-1.87254021e-03j,  5.39877612e-02-1.41864131e-01j, -9.76301436e-02-1.21547018e-01j, -9.20678092e-02+1.74387849e-01j,  1.40685348e-01-1.27548000e-01j,  2.96123871e-02-1.02922618e-01j,  1.11352902e-01-8.62467830e-02j, -1.09712727e-01-3.89440732e-02j,  1.18222416e-01+5.77362689e-02j, -6.58226250e-02-1.00553320e-01j, -7.08216359e-02-3.82394234e-02j,  5.22392245e-02-9.90544241e-02j,  1.04343923e-01-2.21400076e-02j, -1.50119333e-01-2.08365776e-03j,
       -2.00031377e-03+5.81542663e-02j,  3.83065634e-02+1.08315594e-01j, -1.52024028e-01+1.13697356e-01j,  5.98038406e-02-4.82362581e-02j,  8.65636860e-02-6.35096970e-02j,  7.72958756e-02-2.56093728e-02j,  2.29512769e-02+9.62400436e-03j,  4.45398156e-02+8.80887704e-02j,  9.49028759e-03-2.35771406e-02j, -2.36791919e-03-7.62382381e-02j,  1.19090719e-01-8.93309279e-02j,  5.19314568e-02+3.85447831e-02j, -3.87628280e-02+7.77506760e-02j, -3.26884566e-02+1.91066264e-02j,  1.43129957e-01+5.06655538e-02j, -8.14216372e-02-1.24857868e-01j,  4.08149916e-03+3.39964438e-02j, -3.18000514e-02+4.94502870e-02j, -8.59753841e-02-6.05242613e-02j, -7.02375587e-03-7.23709227e-02j, -5.90808602e-02-5.10396991e-03j, -3.99358436e-02+1.54029042e-01j, -1.48939165e-01-2.30986046e-02j, -1.36109697e-02+6.09298717e-02j,  3.36241127e-02-2.08250661e-01j, -6.48593373e-02-2.60921265e-02j,  4.78423392e-02+1.56280882e-01j, -3.67274730e-02-8.41030956e-02j, -1.44975443e-02+7.80523865e-02j, -1.13469394e-01-6.74622693e-02j,
        4.70147160e-02+3.56008433e-02j,  1.80886023e-01+6.94462782e-02j,  2.01441028e-01-2.41069795e-02j, -1.77832245e-01+1.67730057e-01j, -1.72926391e-01+8.72396028e-02j,  1.05716876e-01+7.93440128e-02j, -7.17398366e-02+2.39715722e-02j,  9.40991385e-02+9.48572465e-02j,  7.56374392e-02+1.26422068e-02j, -3.42567574e-02+6.57554524e-03j, -9.80583679e-02-2.48742400e-01j,  1.94821099e-03-4.61412351e-02j, -1.15171892e-01+4.23163024e-02j,  9.55046504e-02-2.85351271e-02j,  6.87501834e-02-9.09245253e-02j,  8.69873372e-02-1.36599865e-01j,  8.72279542e-03+1.71002218e-01j, -1.47382708e-01+1.48146962e-02j, -4.20312282e-03+1.48338430e-01j,  9.18441565e-02-1.67967981e-03j,  1.16613728e-01+1.50842236e-01j,  3.91945313e-02-1.54576297e-01j, -1.04976187e-02+6.29468802e-02j,  1.25177984e-02-8.77763898e-02j,  4.89867798e-02-5.41877145e-03j,  1.09219986e-01+4.12706256e-03j,  4.09641507e-02-2.21297934e-02j, -2.48745776e-02+1.70792444e-01j, -1.24622984e-01-5.47605581e-02j, -6.76718048e-02+5.83777721e-02j,
        1.08185457e-01-1.27066585e-02j, -4.14109909e-02-3.11627607e-03j,  9.79107799e-03+1.12951585e-01j, -4.09954965e-02+4.97455619e-02j, -1.01938277e-01-1.20867638e-01j,  5.94627844e-02+5.46678428e-02j, -2.22617431e-02-1.00320800e-01j, -5.28175862e-02-8.33668982e-02j,  5.18586133e-02+1.16558179e-02j, -6.29728280e-02+1.05923649e-01j, -5.99212755e-02+1.94190572e-02j,  6.94771130e-02-1.55683386e-01j,  5.19897813e-02+6.27034419e-02j,  7.90956993e-02-9.01446300e-02j,  1.88130641e-02-1.49956110e-02j, -4.23517166e-02-8.81984045e-02j, -8.63174858e-03-1.60373942e-01j, -8.74400710e-02+1.35436105e-01j,  1.05708057e-01-4.90293166e-02j, -8.38220286e-02+6.93173211e-02j, -9.44596245e-02+1.41396701e-02j,  1.44384840e-01-5.37706430e-03j, -1.41297717e-02+2.07240685e-01j, -8.91091924e-03+2.76415024e-02j, -5.69003223e-02-1.87856438e-02j, -3.10382650e-02-9.69041662e-02j, -8.46753743e-02-4.51121121e-02j,  2.13287292e-01+2.96795365e-02j, -6.78000993e-02-7.27014711e-02j, -5.24643121e-02+1.97358995e-02j,
        8.62181411e-03+1.79870726e-01j,  1.52898138e-01-9.88048051e-02j,  4.74521169e-02-3.56121574e-02j,  9.22896695e-02+1.40223845e-02j,  4.68319387e-02-7.26391162e-03j,  5.92394125e-03-4.79254400e-02j,  2.16269967e-01+5.54358310e-03j, -4.27515151e-02-1.23614667e-02j,  9.83446073e-02+8.83227942e-02j, -1.09197854e-01+4.99396436e-02j, -8.15404308e-02+2.55541009e-02j,  7.39865612e-02+6.55508075e-02j,  6.63286555e-04+1.54249963e-01j,  9.27110573e-02+3.41628715e-02j,  1.45527728e-01-1.41080682e-01j, -1.41359130e-02-6.02124272e-02j, -1.70089668e-01-9.50888884e-03j, -5.90907847e-02+9.90858589e-02j,  1.20282993e-01+1.63169966e-02j, -6.01831403e-02+4.99282270e-02j,  1.48186487e-02-1.19425802e-02j,  1.58460014e-01-4.80879586e-02j,  1.74692437e-01+1.32829541e-01j, -7.74148211e-02-3.37385643e-02j,  9.17695643e-02-3.98154413e-02j,  2.37156746e-02+9.59269352e-02j,  2.12142259e-01+2.79772767e-02j,  1.07204688e-01+7.30996249e-02j, -1.02045757e-01+1.90798954e-01j, -1.41858107e-01-1.07587269e-01j,
       -1.43454403e-01+3.73650281e-02j,  2.32833516e-02-1.36078422e-02j,  8.83472839e-02+2.71852498e-02j,  1.19513218e-01+5.01626287e-03j, -5.92568345e-02+4.09442957e-04j, -8.14238995e-03+1.83183640e-02j, -6.61149944e-03+7.80984936e-02j, -1.12162965e-01-1.19962828e-01j,  5.48458766e-03+1.00469491e-01j,  2.73708801e-02-5.82827636e-03j, -1.19046015e-02-3.82564125e-02j,  7.29885631e-02-9.01451280e-03j, -4.94120509e-02+8.26467259e-02j, -5.51635770e-02+5.15768294e-02j,  3.20117208e-02+5.85584915e-03j,  1.06872987e-01-4.31353939e-02j,  4.40304662e-02+2.91857939e-02j,  1.68500095e-01-2.82738129e-02j, -2.03371967e-02-1.26195722e-01j,  2.83846022e-02+8.17355207e-02j, -1.51547265e-02+1.12602067e-01j,  7.81649864e-02-2.38189548e-01j, -1.35224365e-02+3.19631898e-02j,  1.17490585e-01+4.01554664e-02j, -4.72704834e-03-3.99565213e-02j, -8.84904826e-02-4.28727274e-02j, -1.22198482e-01-2.42786041e-02j, -1.12897707e-03-6.94497623e-03j, -2.19884514e-02+8.55486354e-02j,  8.70638696e-02-1.07127798e-01j,
       -2.29371995e-02+4.63320598e-02j,  1.93873091e-02+1.86337449e-01j, -1.25468921e-02-5.18952165e-02j, -7.45890764e-03-8.03295372e-02j, -3.56745327e-02-6.25638362e-02j, -4.75510121e-02-7.83464611e-02j, -1.39534894e-01+8.16425405e-02j,  7.70005031e-02+8.69423266e-02j,  1.43911987e-02-1.33546977e-01j, -1.47102056e-01+1.15681347e-01j, -1.33126375e-01+1.02235868e-02j, -1.45122855e-01-2.89314423e-02j,  1.11563142e-02+3.69740758e-02j,  1.00779242e-01-3.90064124e-02j,  1.26325368e-01+6.71845381e-02j,  3.24794315e-02+1.38737288e-02j,  6.81836122e-02+1.40750598e-01j,  3.44546719e-02-5.21553657e-02j, -4.57601524e-02-3.27555405e-02j,  8.24373398e-02-8.90390790e-02j, -1.16860314e-01+1.25653018e-01j,  1.46493260e-01+1.41194112e-01j, -1.45396010e-01-1.32885959e-01j, -7.73293855e-02-6.72724685e-02j,  1.49309687e-01+6.97139456e-02j,  1.91464376e-01+3.82211888e-03j, -3.63957571e-02-1.31754413e-01j, -2.46974881e-02+1.39206134e-01j,  5.83769856e-02-3.77826253e-02j, -6.72768215e-02+2.28114923e-02j,
        2.55957644e-02+4.21858672e-02j, -5.88391848e-02+1.42670909e-01j, -7.05483919e-02-3.98423804e-02j, -7.02652206e-02+1.00856608e-01j, -1.41224511e-01+4.66217961e-02j,  1.07977589e-01+2.05966808e-01j,  1.27139331e-01-9.33860861e-02j, -5.36280638e-04-3.59555623e-02j,  2.38548900e-03+1.08390515e-01j,  5.57620979e-02+2.13967089e-01j,  2.14044445e-02-6.81976237e-02j,  1.33050862e-01+9.78079190e-02j,  9.59093734e-02-1.86488160e-02j,  6.43843748e-02-1.00325395e-01j, -3.78896000e-02-5.56830122e-02j, -9.49734363e-02+1.78783106e-01j,  8.92016398e-02-5.73330404e-02j, -1.49081326e-01-8.02418770e-02j,  1.29000520e-02+2.00740628e-01j,  9.56015376e-02-7.84317559e-02j,  7.74367443e-02+1.73620000e-02j,  6.30112118e-02-2.32571601e-01j,  3.00303803e-02-9.83705381e-02j,  1.61844259e-01+1.05485097e-01j,  2.40291349e-01-4.01070715e-02j,  1.18754377e-01+4.31127547e-02j, -2.18267030e-03+2.88200832e-02j,  1.17227459e-01+1.26467873e-01j, -8.49409061e-02+4.97160270e-02j, -1.63005455e-03-4.99396218e-03j,
        8.44483610e-02+7.05728728e-02j,  1.24861205e-02-2.58181667e-02j, -5.50187440e-02-4.48743334e-02j,  2.80015656e-03-4.82029670e-02j,  4.06373407e-02+1.17659005e-01j,  6.92546145e-02+5.26651932e-02j,  4.41971443e-02-7.53547930e-03j,  1.58836470e-01+1.43051517e-01j,  1.75841588e-01-9.63643389e-02j, -8.17547660e-02-3.31679339e-02j, -3.90876419e-02-9.54602863e-02j, -5.27660299e-02-4.52618847e-02j, -3.29940397e-02-7.32188040e-04j,  1.33182711e-01-3.30125326e-03j, -1.73800247e-01+7.37850658e-02j, -9.03044053e-03-5.62976198e-02j, -7.23873677e-03-5.29735141e-02j,  2.81516307e-03+5.96141674e-03j,  3.40919441e-02-4.95963107e-02j, -5.85547763e-02+1.19710083e-01j,  3.71410507e-02-1.14337865e-01j, -3.28786435e-02-1.31582104e-01j, -3.11008227e-02-1.06387141e-01j,  5.70725503e-02-5.01012877e-02j, -5.67037266e-02-1.45846038e-01j,  5.12987918e-02-5.27321657e-02j, -2.91698456e-02+7.36045193e-02j, -1.78732183e-03+3.56761928e-02j, -4.22088087e-02-2.10621398e-02j, -4.93624568e-02+1.57929069e-02j,
       -7.57986052e-02+1.05462121e-01j,  9.12211212e-02-2.55913922e-02j,  1.65492733e-01+1.11378639e-01j,  1.23643907e-01-6.70361505e-02j,  8.91758326e-02+8.21455575e-02j, -5.05051969e-02+5.59456241e-03j, -1.26290743e-01-5.67854069e-03j, -1.18931736e-01+1.53843733e-01j, -7.50296683e-02-1.07846045e-01j, -1.21333537e-01+4.07381039e-02j, -8.52090624e-02+1.29604666e-01j,  8.09572029e-02+5.23771628e-04j, -5.44002980e-02+1.27113581e-01j,  5.79705919e-02-4.22805852e-02j,  1.26732210e-01+1.50354241e-03j, -4.77950234e-02+1.99087102e-02j,  1.03424607e-01+3.40027024e-02j, -5.83845975e-02+1.59746122e-01j, -2.17388369e-01+6.74992723e-03j,  1.42500131e-01+5.05071268e-02j,  5.45543560e-02+3.92456147e-02j,  2.18488301e-02-6.14267392e-03j, -5.57776866e-02+8.26003743e-02j,  3.49894774e-02+1.41336726e-01j,  6.69241650e-03-1.04471824e-01j,  1.21772946e-01+1.91210500e-01j,  1.19217589e-01-5.38115139e-02j,  1.88246371e-02-1.83004726e-02j,  2.08866104e-03-7.33113093e-02j, -8.12529253e-02+4.61430645e-02j,
       -4.46184815e-02-3.01757956e-02j, -1.35634714e-02+3.38777008e-02j,  4.94210365e-02-2.07529226e-01j,  8.74104130e-02-5.08970513e-02j,  1.53550582e-02-9.11243595e-02j,  8.84523103e-02-4.03101881e-02j, -2.23589887e-01-7.04136609e-02j,  1.23249442e-01+7.55507497e-03j, -1.36224452e-02+1.24659565e-01j,  2.28284552e-01-1.13277060e-01j, -5.96268468e-02-5.73745561e-03j,  1.29530326e-01-8.61904688e-02j,  8.54578487e-02-1.00686862e-01j,  2.70928472e-02-4.63967514e-03j,  1.48161107e-02-5.46569087e-02j,  2.85271880e-02+3.26964639e-02j,  7.96782967e-02+1.10891607e-02j, -1.34176030e-01+6.96277781e-02j, -3.29195127e-02+9.64297617e-02j,  3.03391090e-02+9.71757423e-02j,  5.29277338e-02+3.47785677e-03j, -6.26896146e-02+4.62296383e-02j, -7.85231220e-03+3.56225075e-02j, -1.59413042e-01-7.66033618e-02j, -8.52104081e-03-3.79146418e-02j, -8.48098551e-02-1.08488438e-01j,  6.84002684e-02-9.27612240e-02j, -6.64407275e-02+4.15485835e-02j,  6.08387005e-02+8.19913120e-02j,  1.00446088e-01+1.55852725e-01j,
        3.97609833e-02+1.00146008e-01j, -5.22778987e-02-8.21009007e-02j, -3.77077303e-02+5.13288886e-02j,  9.21346640e-02-1.02972511e-01j, -5.81472043e-02+9.22815026e-03j, -6.72925561e-02-4.11932180e-02j,  6.44503140e-02+1.29261896e-02j, -1.32762347e-01+9.23711163e-03j, -1.03599757e-01-4.56192544e-03j,  3.24505339e-02-2.45398116e-03j,  6.58694205e-02-3.21227618e-02j,  1.24036462e-01+1.47475323e-02j,  1.30152316e-02+1.35707105e-01j,  1.56039110e-01-5.18164633e-02j, -7.03152690e-02+3.12506934e-01j,  8.76117464e-02+1.17448220e-02j, -4.68448704e-02+4.10314253e-02j, -3.22028182e-03-1.22678955e-01j, -1.14178360e-01-6.71304826e-02j,  4.55256387e-02+3.82766596e-02j,  1.48401693e-01-5.47951986e-02j, -7.56378950e-02+1.20320511e-01j,  6.30532615e-02+1.01337916e-01j, -4.99127404e-02+1.37739974e-02j,  2.35931263e-02-3.96687088e-02j,  2.67108253e-02+6.13453731e-02j,  1.62891049e-01+7.84529123e-02j,  7.49086743e-02+1.10381126e-01j,  7.63873637e-02-1.19047306e-02j,  6.77266410e-02-3.28310781e-02j,
       -8.53110808e-02-2.50184384e-02j,  3.83933170e-02+1.43491474e-01j, -8.86989164e-02+7.99727750e-02j, -7.37362920e-02-1.49473969e-01j,  6.43959171e-02+1.99276397e-02j,  6.27697475e-02-2.27116899e-02j,  8.40868521e-02-5.22837391e-02j, -1.12648545e-02-1.12680946e-01j, -1.32392949e-01+4.13293428e-02j,  1.14842189e-01+3.80617047e-02j,  4.67782370e-02+1.54746874e-01j,  8.23224982e-02+4.56644140e-02j, -2.82310492e-02+1.47896780e-01j,  6.71171984e-02+3.63367239e-03j, -1.30820895e-02-1.66233584e-02j,  1.56041973e-01-5.91053002e-02j,  9.72049576e-03+1.57604493e-01j,  3.99164787e-02-1.39203421e-01j,  1.33473022e-02-2.20465140e-02j,  8.28185122e-02-6.79543638e-02j,  3.82282405e-02-5.77487822e-02j, -1.55532854e-02-1.04539428e-01j, -5.91946563e-02-1.84369274e-02j, -3.82486186e-02+4.42172266e-02j,  1.18801889e-01-8.57833822e-02j, -2.51752531e-01-1.52096140e-01j,  1.55380879e-01+7.23105262e-03j, -1.01032533e-02+1.06962879e-02j, -1.31703458e-01+3.76881824e-02j,  1.60460085e-02+6.30850547e-02j,
        4.19255549e-02+3.92629725e-02j,  4.93357915e-02+4.62217751e-02j, -2.46551328e-02+9.18461625e-02j,  7.84824861e-03+3.42026930e-02j,  2.27842145e-02+1.02988705e-01j,  8.89143553e-02-1.32777071e-01j, -1.53255753e-01-5.64168441e-02j,  2.60864735e-02+6.83070308e-02j,  1.24089898e-01+1.46531300e-01j,  8.03861657e-02+8.44347212e-02j, -3.99675456e-02-4.13219148e-02j,  1.99496563e-02-1.68741956e-01j, -9.61656011e-02-9.85036805e-02j,  3.81645661e-02-1.58403405e-01j, -1.22222597e-01+2.27927369e-02j, -1.95541768e-01-3.26566021e-02j,  3.06440615e-02+3.05364521e-02j, -8.76411203e-02-1.79424422e-02j, -2.13722727e-02-1.46393081e-01j,  1.81898579e-02-1.17003644e-01j,  1.36627407e-01-1.57758323e-01j, -5.03748616e-02-1.47579680e-01j, -3.40991053e-02+2.60350260e-02j,  6.61586084e-02+1.13051859e-01j,  9.27199042e-03+6.76019455e-03j,  1.97908095e-01-7.26532762e-02j,  8.82218775e-03+6.06572679e-03j, -7.52454053e-02-6.42541289e-02j, -8.99883870e-02+4.02263836e-02j, -1.59753697e-02+1.71002481e-01j,
       -2.61136199e-02-1.03271017e-01j,  1.56419876e-02+3.02378993e-02j, -1.06994379e-02-4.42494478e-02j,  7.02703110e-02+5.64923383e-02j,  5.07122277e-02+4.74081563e-02j,  7.67303013e-02-1.56906009e-02j,  9.16318724e-02-3.07214532e-02j, -5.18225692e-02+1.08506378e-02j, -7.67455403e-02+5.55639645e-02j,  6.64535546e-02-9.01565151e-03j, -9.14990010e-02+3.73538098e-02j, -7.28625453e-02-1.19186740e-01j, -2.77589055e-02-8.22322098e-02j, -3.22296245e-02+1.31757376e-01j, -3.50339775e-02+1.37913873e-01j,  3.84813010e-02+1.67884657e-01j, -7.00195023e-02-6.19230041e-03j,  8.02979751e-03+1.25547652e-01j,  3.81418938e-02-8.66550141e-02j,  6.66673422e-02-2.80517890e-02j,  1.47446194e-01-1.21608923e-01j, -1.06983782e-01+1.61685673e-02j,  1.45128417e-01-9.97579672e-02j,  4.78343409e-02-3.43103884e-02j,  5.46402843e-02+3.32975311e-02j, -1.17712449e-01+3.53043129e-02j,  6.17783098e-02+7.68043303e-02j, -4.12278356e-02+2.20803550e-01j, -1.27049166e-01+5.43901474e-02j, -3.76806434e-02+1.87060854e-01j,
       -8.90334008e-02+4.81197426e-02j, -1.61805869e-01-5.83124248e-03j, -2.35021073e-01-1.36839690e-02j,  5.95583000e-02-8.53455811e-03j, -1.36475568e-02+1.02072032e-02j,  6.57874310e-04-5.54357406e-02j, -1.16075063e-02-1.88043201e-01j, -4.10593532e-02-1.25295385e-01j, -1.01181308e-02-1.83976177e-02j,  1.22655279e-01-1.17069920e-01j,  4.31446899e-02-6.57352962e-02j, -1.00119330e-01-1.39878527e-01j,  9.98658335e-02-4.26160789e-02j, -1.34536488e-01+9.74478087e-02j, -6.92874025e-02-8.13138795e-02j,  2.45163582e-02-1.76548089e-01j,  3.22174804e-02-5.65589817e-03j,  8.07679515e-03+6.33986691e-03j, -6.31184637e-02-4.41509650e-02j,  2.05374550e-02+4.76306825e-03j, -1.36800160e-01+3.66858887e-02j,  1.47490170e-01-1.20606391e-01j,  8.14807620e-02-2.11446690e-01j, -1.45202365e-02+8.19600273e-02j, -1.59974770e-01+4.89558755e-02j,  1.40694468e-01-3.31038214e-02j,  1.41222361e-02+1.88783167e-02j, -1.39964930e-04-7.90888896e-03j,  4.02400089e-02-1.91496498e-02j, -2.10975834e-02+2.55743635e-02j,
       -1.24830235e-02-1.04506988e-01j, -7.83991128e-02+7.24495291e-02j, -7.67997026e-02+1.15793120e-01j,  7.71299789e-02+1.54784598e-02j,  1.00227939e-01+1.64355312e-01j,  8.14056190e-02+1.20896109e-01j, -5.07211511e-02-4.31177626e-03j, -5.98801754e-02+1.87757032e-01j, -5.58581009e-02+1.11035307e-01j,  8.64549886e-02+9.22623166e-02j,  1.33102211e-01-8.68358502e-02j, -4.38973116e-02+3.47998547e-02j, -1.51802312e-01-9.79580479e-02j,  2.32964551e-01-2.62447131e-01j,  5.48006171e-02-8.11579033e-02j, -1.74315936e-02+9.52579508e-03j,  1.17761070e-01+1.11200729e-01j, -8.00284541e-02+1.32515499e-01j, -1.49555446e-02+1.88069542e-01j, -7.57884206e-03-4.58967652e-02j, -9.50078367e-02-1.00631395e-01j,  1.72491605e-02+4.47670026e-02j, -2.03781472e-02+3.26250698e-02j, -9.59715607e-02-4.37484169e-03j, -9.58454167e-03+2.95792410e-02j,  1.05438691e-01-1.33092379e-01j,  3.09937517e-02-2.42140206e-04j, -1.44752997e-01-1.10462732e-01j,  3.59606533e-02-3.84511090e-02j, -3.56750817e-02+5.75093775e-02j,
       -4.15723422e-02+5.09095742e-02j, -2.50051531e-02+3.74719107e-02j, -3.34371919e-02-2.69992518e-02j,  2.17278805e-02-1.59555220e-01j, -1.18390615e-01+3.06546257e-02j,  5.93851132e-04+6.14587651e-02j,  1.40201832e-01-1.17018742e-01j,  6.30594857e-03+5.45456872e-03j, -3.76289949e-02-1.52282039e-02j,  1.32512888e-01-1.48351173e-01j, -7.54453520e-02-4.61426234e-02j, -3.03461225e-02-5.67156398e-02j, -8.36462232e-02-1.37701819e-01j,  1.70123787e-02-2.07730468e-02j,  2.02734494e-01+6.52242080e-02j,  1.92557330e-02+1.00734613e-01j,  1.75343958e-01+8.81780749e-02j, -1.19044611e-02+5.91177580e-02j,  1.67539379e-02+4.06442164e-02j, -8.00636857e-02+7.89439667e-02j, -4.75474374e-02+2.83892572e-02j,  2.61253074e-02+1.39692416e-02j,  1.27684922e-01+1.17156954e-01j,  2.73419985e-03-8.15456779e-02j,  1.30767101e-01-7.98744725e-02j, -1.17162715e-02-1.65840395e-01j, -5.76548732e-02+5.87985325e-02j,  7.07963445e-02-1.79886324e-01j, -4.20474415e-02+1.08883511e-01j, -5.75618414e-02+5.21190462e-03j,
       -7.21987996e-03-3.10182665e-02j,  5.50605465e-02-7.25079912e-02j, -5.35629253e-03-3.15994255e-02j,  1.45261311e-02+1.13362595e-01j, -2.76125254e-02-3.24502000e-02j,  3.91132852e-02+7.99084254e-02j,  1.36671884e-01-4.22167609e-02j, -2.78745524e-02+1.66604820e-01j, -5.79322711e-02+1.89892952e-01j,  5.80213545e-02-5.63300106e-02j,  4.21450402e-02-2.34111878e-01j, -9.04896655e-02+8.57572295e-02j, -1.03359661e-01-7.96324957e-02j,  6.02369786e-02-6.27833326e-02j,  1.19673196e-01+1.10677207e-02j,  8.30081547e-02-3.58778391e-02j,  6.11783668e-02+6.11887674e-02j, -7.68605931e-02-5.39948682e-03j,  1.21343778e-01-2.97186254e-02j,  1.29116081e-01+1.87106712e-01j,  5.15873038e-03+6.61919344e-02j, -1.83282876e-03-3.68731936e-02j,  1.17016771e-01+1.58222912e-01j, -3.87226282e-03-9.70371654e-04j, -1.79300200e-01-2.83260393e-02j, -1.41400483e-03+1.35331847e-02j,  5.72087458e-02-7.75475006e-02j, -1.58108785e-01-7.97071215e-02j, -3.31189056e-03-5.73357007e-02j,  4.13786706e-03+1.22780539e-01j,
       -5.36931375e-02-6.72722822e-02j,  1.11920724e-01+1.29862093e-02j, -5.24987149e-02+4.76643575e-02j,  1.05637164e-01+8.27861165e-02j, -1.18864055e-01-1.74568296e-01j,  5.81192411e-03-4.04070185e-02j, -1.03430483e-01-1.69035282e-01j, -1.13502716e-02+8.34191056e-02j,  1.05325163e-01-4.76216714e-03j, -9.25772817e-02+1.50670237e-01j,  4.28140700e-02+3.46151756e-02j,  9.73338934e-02+7.53926315e-02j, -1.25555176e-01+4.50327447e-02j,  2.83269738e-02+2.14864449e-02j,  1.03088528e-01-1.14373265e-01j, -7.31401450e-02+1.77769898e-01j, -1.12535149e-02+4.78161974e-03j,  8.18570896e-02-3.81132366e-02j, -1.33295296e-01-9.10001207e-02j,  8.58276762e-02-1.41863647e-02j,  1.67874702e-01-1.33988434e-02j, -8.67651467e-02+1.32689953e-01j, -3.90355992e-02-3.43351624e-02j,  7.26507704e-02+2.64829915e-02j,  1.11678187e-01+3.90653261e-02j,  1.15906135e-01+1.76724059e-02j,  9.82619413e-02-6.25174727e-02j, -7.37921876e-03+2.82544110e-02j,  1.03018820e-01-6.28247854e-02j,  1.87003733e-01+3.37938203e-02j,
        4.85530302e-03+7.88810000e-02j, -9.36602683e-02+2.03654036e-01j,  6.70298724e-02-1.28682263e-02j, -2.54909646e-02-1.98646445e-02j, -1.98543086e-02+6.61216538e-02j, -8.48576080e-02-2.49620592e-02j, -6.71496603e-02+7.22095879e-02j,  4.76262959e-02+1.21009164e-01j, -4.58951514e-02-5.17531968e-02j, -6.47960010e-02-1.20728072e-01j,  1.60295792e-01-1.40473647e-01j, -1.63756748e-03+1.76260693e-01j, -2.14187214e-02-1.44486405e-01j,  1.38935677e-01+1.82433112e-02j,  2.72551076e-02-8.98792607e-02j, -1.71326545e-01-1.10489461e-01j,  2.59713517e-02+1.80210728e-01j, -1.13356997e-01-5.39655073e-02j, -1.15002327e-01+2.11468680e-02j,  1.22206680e-01+2.00740144e-01j,  5.31251355e-02+3.41911572e-02j, -2.85470406e-02+3.40588721e-02j,  1.62612378e-02-1.25619786e-01j, -5.56722710e-02+8.35975795e-02j,  9.53800031e-02-8.43482473e-02j, -1.09105819e-01+9.48804730e-02j,  4.77081838e-02+9.49219343e-02j,  8.11850606e-02+1.08680074e-01j,  1.22006741e-01+5.82741535e-02j, -2.29701893e-02+6.62232347e-02j,
        3.92315193e-02+6.55030120e-02j,  2.03376796e-01+9.27366361e-02j,  6.35392164e-02+3.23680200e-02j, -5.31622978e-02-2.88908372e-02j, -7.02426374e-03+6.75379556e-02j, -2.12237232e-02-1.83434699e-02j, -6.78967104e-02-5.80275574e-02j,  1.69828044e-01+9.49136570e-02j, -5.08448568e-02-5.19598149e-02j, -1.06691250e-01-9.62889583e-02j,  5.32046688e-02-1.90509591e-02j, -2.50194692e-02-9.29642044e-02j,  1.88093417e-02+7.48436505e-02j, -6.10298150e-02-3.20064291e-02j,  4.40991893e-02+8.70526883e-02j, -3.11373800e-02-7.99283258e-03j, -1.21129358e-03+6.04760979e-02j,  1.23181705e-01+3.86186523e-02j, -3.45111027e-02+4.90467351e-02j, -6.92065655e-02-3.02708167e-02j,  5.84411839e-02-4.52250729e-03j, -2.94612333e-02-1.11594151e-01j, -4.35060760e-02+3.08939714e-04j,  1.65268661e-01+1.78101296e-02j, -9.71783036e-02-9.50097168e-02j,  3.12533928e-02-1.60026912e-02j, -7.79738476e-03+4.88586564e-02j, -8.40499527e-02-6.37198817e-02j,  9.61353831e-02+1.26583069e-01j, -5.53354967e-02-2.40176823e-02j,
       -1.15968617e-01+3.30895067e-02j,  1.62856623e-01-3.47484132e-02j,  1.63934966e-02-1.56508626e-01j,  1.64466893e-01-1.15467354e-01j,  3.63366559e-02-1.20918496e-02j,  5.57051607e-02-2.18319816e-01j,  6.95284985e-03+1.83828937e-01j,  4.11729400e-03-1.49751410e-01j,  1.16899770e-01+5.20485323e-02j, -5.34538311e-02-1.12893252e-02j,  2.25104422e-02+1.11501556e-01j, -4.55768609e-02+2.15601916e-02j, -1.94808270e-02-1.27154235e-02j,  2.91608554e-03-5.58722359e-03j, -1.03179399e-01+6.99009222e-02j,  1.13405865e-01-2.56399009e-02j,  3.13735506e-02-5.75109182e-02j,  3.65624418e-03+1.07627557e-01j, -2.24465012e-02+9.87669291e-02j, -9.35607749e-02-5.07987181e-02j,  5.79961983e-02+3.09445598e-02j,  2.97307432e-03+3.93021054e-02j,  2.11226275e-02-9.88357430e-03j,  3.13416632e-02+1.03456485e-01j, -1.01917192e-02+6.18006060e-02j,  2.71549373e-02+1.35561825e-01j, -7.68074153e-02-6.85512247e-02j, -1.17581867e-02-2.53829160e-02j,  9.28375780e-02-6.14954707e-02j, -1.41596614e-01+1.07188921e-02j,
       -7.28860525e-03+2.43651634e-01j, -8.80650359e-02+1.43082304e-01j, -1.26341313e-03+8.32647482e-02j,  2.03115842e-01+6.89095479e-02j,  1.67115866e-01-4.60730662e-02j, -1.05755428e-01+1.22039239e-01j, -1.43352206e-01-2.39082798e-02j, -4.18043091e-02+8.97024776e-02j, -7.77240171e-02+2.19308414e-01j, -9.11640110e-03+8.65422721e-02j, -1.05457490e-01-1.84014637e-02j,  6.77892119e-02+2.28872910e-02j,  1.16564450e-02+4.25565495e-02j,  1.26894200e-01-1.70513750e-01j, -1.20932866e-02-1.23697146e-01j, -1.05797816e-01-7.67449744e-02j,  5.47589103e-03-1.90378815e-01j, -1.22361425e-01-3.64873506e-04j, -4.92975620e-02+2.03877639e-01j, -1.01416867e-01-2.20615975e-02j, -4.04668442e-02+1.54538885e-02j, -7.31195916e-02+1.47711186e-01j,  1.28543207e-02+3.67988918e-02j,  6.59874297e-02-2.20951499e-02j, -5.69867977e-02-2.77874616e-02j, -1.39863923e-02-3.92723193e-02j,  3.44966375e-02+9.17851631e-02j, -1.35718074e-01+3.35410957e-02j,  2.56668817e-01+3.62517597e-02j,  1.45008246e-03-1.67266042e-02j,
        7.23949189e-02+3.50058311e-02j,  2.95906913e-02-5.11673530e-03j, -2.89210703e-02-7.83260207e-02j, -3.95276055e-02+2.62070530e-02j,  1.89223565e-02-3.73511633e-03j, -1.49815586e-01-3.23024572e-02j, -2.76757428e-01+1.09794058e-01j, -1.55603333e-02-9.26240637e-02j, -8.34324290e-02+1.10717331e-01j, -8.49852684e-02+1.21096192e-01j, -2.34217533e-03-2.20533245e-02j, -4.05124372e-02+4.91544177e-02j,  2.21939474e-01+1.06585662e-02j, -1.14788852e-01-1.45670084e-02j, -2.31864711e-02+8.16787838e-02j,  1.05772073e-01-3.81363629e-02j,  1.06410310e-02+5.56535302e-02j,  7.49474640e-02-2.44079960e-01j, -1.14493374e-01+2.54362093e-03j, -9.10842184e-02+4.58644113e-03j, -1.50764400e-01+9.60854982e-02j,  1.17718269e-01-1.33115252e-02j,  2.88747212e-02+2.37461286e-02j, -1.41759597e-02+1.51999121e-02j, -2.47353248e-02+7.48763400e-02j, -3.83509977e-02-3.50842772e-04j,  2.44222600e-02+5.36360149e-02j, -4.15187177e-02+1.66291792e-02j, -6.98415077e-02+1.55027916e-01j,  1.75717100e-01+4.63332796e-02j,
        1.62118073e-01+1.44290350e-01j,  2.27418566e-01-1.45684285e-01j,  3.06208328e-02+4.27399784e-03j,  7.46468629e-02-2.60921832e-02j,  1.36523904e-01+4.44488001e-02j,  4.42830359e-02-8.76058694e-02j, -5.01048932e-02+1.06328634e-02j,  1.74182387e-02-7.92930529e-02j,  1.30387046e-01-5.77562670e-03j,  3.38661732e-03-2.66393819e-02j, -5.84555746e-02+7.24533139e-03j,  9.24386154e-03-7.38257239e-02j, -2.01353089e-01-1.54900117e-01j, -7.64397545e-02-2.64763460e-02j, -1.05638845e-02+4.79286399e-02j,  3.31547555e-02+1.82840041e-01j,  1.02455279e-01-3.22470454e-02j, -2.02968636e-02-1.20561400e-01j, -1.53201600e-03-4.86412356e-02j, -3.36622849e-02+1.23466127e-01j, -3.17666031e-02-5.30530230e-03j,  7.87856594e-02-1.10488726e-02j, -6.50752335e-02-1.53696600e-01j,  1.12242184e-01+9.63039070e-02j,  7.29070752e-03-6.12939915e-02j, -1.27887554e-01-5.27193958e-02j,  2.67746028e-02+1.70487967e-02j, -7.75381791e-02-8.27737207e-02j, -2.68419314e-02+3.52729473e-02j,  2.08583372e-02-4.61001172e-02j,
        1.35435125e-02+7.17668513e-03j,  7.51284375e-02+1.57659374e-01j, -1.16573329e-01+3.45952874e-02j,  1.16952561e-01-1.92810160e-01j,  8.91983080e-03-1.72847221e-01j, -1.37395165e-01-1.29769519e-01j, -5.57858716e-02-1.29509855e-01j, -3.78818453e-02+8.97323481e-03j, -9.26055477e-02-3.82225179e-02j, -1.02176844e-01+7.72429422e-02j, -2.77767386e-02-7.89175823e-02j,  1.08550789e-01-1.24801337e-01j, -7.51967666e-02+4.42884220e-02j, -3.84132813e-02-2.23818923e-02j,  2.10290591e-02-1.63910071e-02j, -1.55546281e-02+6.83243462e-02j,  7.04925956e-02+1.13334991e-01j, -7.80853320e-02+6.46462801e-02j, -3.21182665e-02+5.84735194e-03j, -1.01335631e-03+2.56732273e-01j,  4.63262263e-02-8.08179095e-02j,  3.58066672e-02+4.41791548e-02j,  6.73367098e-02-7.01266251e-02j, -8.73687243e-02+8.94059276e-02j, -7.39072106e-02-1.65025667e-01j, -7.66981818e-02-3.34616641e-03j, -4.22405675e-02-1.60785801e-01j, -4.05947796e-02-6.73805916e-02j,  2.08188645e-02+6.87961393e-02j,  1.67132980e-01+2.05388800e-02j,
        2.10096943e-02-7.14703997e-02j, -1.96928545e-02-2.18626018e-01j,  3.85125630e-02+7.57691532e-05j,  8.30529385e-02-2.07160668e-02j,  1.38867666e-02+1.41588611e-01j, -6.60239944e-02-2.31858478e-01j, -5.97977022e-02-1.57154782e-01j,  1.12189711e-01-1.18577658e-02j, -3.53731368e-02-7.82586039e-02j,  1.75438251e-02+8.76571977e-04j,  1.41945832e-01+9.28040164e-02j,  3.52110938e-02-1.77822589e-02j,  4.95572709e-02-2.87314727e-02j, -1.30143044e-01+5.82148327e-02j,  1.45779892e-01+5.21651803e-02j, -1.98968827e-02+4.94037736e-02j,  6.58149682e-02-6.44416128e-02j,  9.20102049e-02+3.04692222e-03j, -3.38008186e-02-5.63391322e-02j,  8.37698748e-03-3.59590496e-02j, -4.35261097e-02-3.87390686e-02j,  5.76792009e-02+3.44475463e-02j,  9.59522963e-02+2.15717502e-01j,  1.88552950e-02-4.53059574e-02j,  1.70086117e-01-2.64053284e-02j,  2.22595913e-02-7.30566035e-03j, -1.12054071e-02+1.22101799e-01j, -6.41573620e-02+1.66227099e-01j, -3.58693002e-02-2.83078773e-02j, -6.60688819e-02-2.56512600e-02j,
       -4.86337540e-02-1.57859846e-01j, -1.25599279e-02-4.54959147e-02j, -2.03534357e-02-2.87431987e-02j, -1.27646849e-02-3.90012510e-03j,  3.47598092e-02+7.87069224e-02j, -3.62772635e-03+1.62428369e-01j,  3.62281855e-02+5.50061027e-02j, -5.70765622e-02-1.34000365e-01j, -9.44564719e-02+1.49567942e-01j,  1.21510992e-01+1.17693975e-01j, -6.62911619e-02-6.46359390e-02j,  7.09082700e-02+2.48476467e-02j,  1.57824600e-01+1.17066315e-02j, -8.44151788e-02+5.34910078e-02j,  8.22583660e-02-9.68421318e-02j,  8.59570275e-03-1.91844681e-01j,  3.00350172e-02+6.80688724e-02j,  8.21553079e-03+3.06162281e-02j,  9.23264499e-02+1.94905975e-01j, -4.14970965e-02+1.08064766e-01j,  2.58033773e-02-6.91497742e-02j,  1.55917773e-01-5.09599368e-02j, -3.65483160e-03+1.14380435e-01j,  5.51468816e-02-2.59419769e-02j,  1.70702211e-01-1.99296533e-01j,  1.47724497e-02-1.85515054e-02j,  2.72122021e-02+7.29287795e-02j,  1.49109842e-01+5.93069060e-02j, -1.66613542e-02-8.67137446e-02j, -1.20909604e-01-2.93675341e-02j,
       -9.83917085e-02-1.84204761e-02j,  1.62814792e-01-1.35139306e-01j,  4.73460440e-02+1.31337461e-01j,  2.32713411e-01+2.88388028e-02j, -6.68972821e-02+2.22197631e-02j,  3.92719612e-02+1.52580203e-01j,  2.53776470e-02+7.74832234e-02j, -4.36193830e-02+4.24058557e-02j,  8.23322375e-02+1.76410134e-01j,  1.54890627e-01+5.32375276e-02j,  4.37512275e-02+4.15304821e-02j, -3.49403249e-03+1.52670688e-01j, -9.70650860e-02+1.44844260e-01j, -1.95678640e-03+1.99977290e-02j, -4.80695395e-02+1.94770744e-01j,  2.79827269e-02+9.91437026e-02j, -1.29906563e-02-1.00716455e-01j,  5.77525685e-02-1.30956086e-01j,  2.57358361e-02-6.44845883e-02j,  1.72803438e-02-8.23410272e-02j,  6.55852364e-02+2.73660529e-02j,  1.25689774e-03+5.70941527e-02j, -6.05442115e-02+8.50756711e-02j, -1.20872173e-01+1.94266740e-02j,  1.99440268e-02+6.12317022e-03j, -8.27951261e-02+2.42864309e-02j, -3.47486834e-02+9.66718780e-03j, -9.44474618e-03-1.29324625e-03j,  1.08077651e-01-7.63083493e-02j,  1.77039940e-01+1.34457007e-02j,
        1.02205651e-01-1.32673224e-03j,  2.00202803e-01-4.79903919e-02j, -4.07654990e-02+1.99179948e-02j,  6.65024855e-02+9.63731804e-02j,  6.17223649e-02-2.41440229e-02j,  6.21646020e-02-4.29251282e-04j, -1.19217377e-01-5.11233888e-03j,  5.00452355e-02+9.64116293e-02j,  6.72018105e-02+1.58383035e-02j,  1.04920585e-02-3.75171295e-02j, -5.15836453e-02+4.10383258e-02j, -4.50121432e-02-1.56393689e-01j, -3.69949558e-02-3.49941789e-02j,  4.24813266e-02+4.56446662e-02j, -1.40693525e-01+3.62119110e-02j,  3.54644935e-02+6.55820345e-02j, -1.15036248e-01-6.70995814e-02j,  1.74477980e-01-8.46266302e-02j,  5.63822416e-02+7.09389700e-02j, -6.32001141e-02+5.22351752e-02j,  4.98335744e-02+8.47202636e-02j,  4.63525184e-02-2.59316779e-03j,  1.21744758e-01-2.32443032e-02j, -2.57646251e-01-1.06662161e-01j,  2.17462637e-02+8.80639240e-02j,  9.00883144e-03-1.80037699e-02j, -4.06906387e-02-4.18911845e-02j,  9.21795940e-02-4.20689728e-03j, -6.49150463e-02-8.38487063e-04j,  2.29472275e-02-1.09014529e-01j,
       -1.18350148e-01+4.08252745e-02j,  4.82206235e-02-1.43520408e-02j,  8.95036890e-02+1.93669906e-02j, -1.15319803e-01+1.36806111e-01j, -1.74398823e-02-2.77018578e-02j,  2.58644789e-02+5.38396709e-02j,  9.21745774e-02-7.13300386e-02j,  8.49353400e-02+1.58488850e-01j,  1.05015608e-01-3.77073810e-03j,  9.35912413e-03+1.63510644e-01j, -4.73810170e-03-5.70824705e-02j,  6.57060173e-03-1.96057621e-03j, -7.21801277e-02+1.52720724e-01j,  9.56171753e-02+1.17779754e-01j,  4.28275190e-02-3.45548941e-02j, -4.86463734e-02-1.09920637e-01j, -6.83698564e-02-3.81786068e-02j,  1.08564055e-01-1.99558529e-01j,  2.14653459e-01-4.88992080e-02j, -8.24321086e-02+6.30930070e-02j,  3.37396930e-02+8.09973287e-03j,  3.12656291e-02+2.90758576e-03j,  4.19413119e-02+1.76739590e-01j, -1.78314610e-02+1.95598017e-01j, -1.03324368e-02-5.77691340e-02j,  4.49029815e-02+6.21737213e-02j, -8.40076602e-02-9.83562420e-02j,  1.13326150e-01+9.91253684e-02j,  2.38893291e-02+1.12755456e-01j, -8.37705940e-02+1.55665879e-02j,
       -4.29407233e-02-1.17518245e-02j,  5.13377269e-02-8.93171618e-02j,  1.20001217e-01+2.01425975e-01j,  1.90297138e-01-8.38967425e-02j, -3.87460366e-02+7.49288218e-02j, -1.24320906e-01+6.56743495e-02j, -6.02213153e-02+3.79364325e-02j, -2.18310318e-02+1.88705143e-02j,  4.26490839e-02-7.19299318e-02j, -9.25682258e-02+2.18607895e-01j, -1.01660296e-01+1.12017637e-01j,  1.51315515e-01+1.84926829e-02j,  8.57594723e-02-7.50221664e-02j, -1.98220124e-02-1.20395767e-01j,  1.27485048e-01-6.69181634e-02j, -2.73468470e-02+4.65786430e-02j, -7.72377306e-02-2.68279016e-02j, -4.41939063e-02-1.86501327e-01j,  3.79414469e-02+1.29131092e-01j,  1.93659647e-03-1.92419385e-01j, -4.08341014e-02+6.14073848e-02j, -3.84304585e-02+7.01256300e-02j,  5.60045546e-03+9.27319924e-02j, -2.27222077e-02-8.96927881e-02j, -5.53554102e-02+8.30322612e-02j,  1.00213142e-01-6.38125486e-02j,  5.47423951e-02-9.71559022e-02j,  3.82241837e-03+1.21289781e-01j, -4.04486610e-02+1.79941878e-02j, -1.03559368e-02-4.30334206e-02j,
        5.89578314e-02+4.00524147e-02j,  1.40916511e-01+6.67082696e-02j,  4.40418631e-02+3.70535594e-03j, -2.67797544e-01+8.62091694e-02j, -3.76648865e-02-2.31015760e-02j, -7.67526017e-02-8.08939654e-02j,  9.26572419e-02+2.16789139e-02j, -6.29159288e-02+1.04174588e-01j, -1.11529756e-01+5.62658454e-02j,  1.17951934e-02-1.61302278e-01j, -1.80869076e-03-6.34125495e-02j,  7.62696393e-02-7.90164136e-02j, -7.87952131e-02-1.39323485e-01j,  1.02348156e-02+5.51626383e-02j, -1.67469740e-02+1.83413897e-02j, -2.87239563e-02-5.06066118e-02j,  8.87510480e-02+8.98941416e-02j, -5.76729095e-02-3.50195824e-02j, -8.61756954e-02+3.10618350e-02j,  2.94118856e-01-7.48255388e-02j,  1.58173913e-02-1.09383861e-01j,  3.21889474e-02+1.12228748e-01j, -6.28360404e-02-1.16637975e-01j,  7.93730300e-03-1.09748836e-02j,  1.27800657e-01-9.16258515e-03j,  3.27511712e-02+1.01571073e-01j,  9.85438792e-02-5.31420117e-02j,  1.61275851e-02-2.80605417e-03j, -1.19686377e-01+1.32242066e-01j,  1.87543551e-01+2.38937126e-03j,
        1.16921884e-02+2.62848401e-02j, -9.86694177e-03+7.77981844e-02j, -9.85602566e-02+2.38507308e-02j,  2.35400707e-03-5.43592014e-02j, -7.23858021e-02-5.93465697e-02j, -1.58506446e-02-5.97662787e-02j, -8.75208135e-03+5.09418023e-02j, -3.08916406e-02-3.48724890e-02j, -1.60191352e-02+1.03111217e-01j, -5.68159068e-02+7.46244049e-02j,  9.23995849e-02-1.05567241e-01j,  1.02643630e-01-4.48982087e-02j, -2.37025557e-02+9.91981818e-02j, -1.51080851e-01-6.60829227e-02j, -1.27955345e-01-3.54218582e-02j,  6.68788444e-02+7.48446185e-02j,  1.25809616e-01+8.22238020e-02j,  5.76912904e-02-7.07594599e-02j, -8.78078680e-02+1.75702063e-02j,  2.23150487e-01+1.21618743e-01j,  3.68863819e-02-2.30063462e-01j, -2.78605009e-02+3.00439015e-02j,  4.76710874e-02-4.21785515e-02j, -2.06022674e-02-8.33240131e-02j,  1.53185096e-01+5.43462784e-02j,  1.10483025e-01-9.58798477e-02j, -1.64335314e-02-1.37741584e-02j,  1.26456713e-01-6.49290331e-02j,  7.02876284e-02+8.95977937e-02j,  8.45954555e-03+1.52604418e-01j,
       -1.26458774e-01-1.00591188e-01j, -1.51082804e-01-7.07219691e-02j,  1.35312036e-02+2.42766701e-02j, -2.57820349e-03-5.82206445e-02j,  1.25394937e-02+1.54994794e-01j,  4.34303108e-02+1.67758088e-01j,  2.50193827e-02-1.68203339e-02j, -7.42770942e-03+1.19995245e-01j, -2.14668364e-01-1.80954136e-01j, -7.90535193e-02+7.86162084e-02j,  1.63419690e-02-6.09920610e-02j, -1.35407002e-01-1.56744993e-02j,  1.23914645e-01-4.03721439e-02j,  1.77204484e-03+2.66140087e-02j,  1.25716789e-01-3.78667856e-02j, -2.99746760e-02+4.86935208e-03j,  1.38225290e-02+7.04359534e-02j,  1.72072353e-01+1.31015253e-02j, -6.19101329e-02+1.24807440e-02j,  5.20454040e-02-2.47656886e-01j,  1.05189919e-01-2.88161306e-02j, -1.53992477e-02+7.55199009e-03j,  5.82505243e-03+1.20984659e-01j, -1.42134562e-01+1.12480596e-02j, -1.09111335e-01-1.54002884e-01j,  4.21553065e-02+5.76813852e-03j, -7.58350800e-03+2.26849903e-01j,  4.77762685e-02+3.40790650e-02j,  3.85152600e-03+1.26559590e-01j, -4.06218500e-02+1.26222670e-01j,
       -1.28628510e-01-1.00489682e-02j,  7.45782354e-02+6.90716494e-02j, -1.39804487e-01-5.89119056e-02j, -3.13298452e-02-1.40520253e-01j, -3.01100745e-02-8.12306975e-02j,  9.58783144e-02+8.71888394e-02j,  8.17466446e-02-5.59117276e-02j,  8.32972084e-03+1.02402858e-01j, -5.98910605e-02+1.77552646e-01j, -1.62084309e-01+4.79064238e-02j,  4.56745968e-02+2.37759503e-03j, -1.73401279e-01+2.45332405e-02j, -1.08518076e-02-5.51290615e-02j,  1.61371652e-01+7.02663314e-02j,  5.77371628e-03+3.08382613e-02j, -5.23348517e-02+6.18789064e-02j,  6.05004292e-02-4.83654472e-02j,  1.26452871e-02-1.00891539e-01j,  3.01323647e-02+1.96240326e-01j, -5.92861632e-02+1.08424895e-01j,  1.07222615e-01+1.41348222e-01j, -3.52439375e-02+9.37165441e-03j,  2.68309794e-02-1.10349093e-01j,  1.94493578e-02+1.47608062e-01j,  6.93871662e-02-9.40535516e-02j, -6.68998398e-02+3.72529839e-02j,  7.31071439e-02+4.62749207e-02j,  8.22767275e-02+4.33738687e-03j, -1.27461890e-01+7.16800653e-02j, -2.59615938e-02-4.42029716e-02j,
        5.93603172e-02+8.00663553e-02j, -1.30433729e-01+1.22098338e-01j,  6.08017369e-02-7.05338115e-03j, -4.97809705e-02+1.42873842e-02j, -4.40948151e-03-3.62800038e-02j, -7.23446309e-03-2.28188194e-02j,  7.55706875e-02-1.69914898e-02j,  1.20151025e-01+9.98944293e-02j, -2.80746263e-02-5.48362337e-02j, -1.32263858e-01+9.17482255e-02j, -1.57415802e-03-1.08396653e-02j,  3.69312813e-02-2.91296460e-02j, -3.41443670e-02-7.03052603e-02j, -1.38221692e-02-3.27697123e-02j,  8.95111448e-02-3.25747369e-02j,  9.12661146e-02+7.88395467e-02j,  1.33323387e-02+8.39085279e-02j,  1.03179796e-01-2.57974091e-04j, -4.01998444e-02+1.51487692e-02j, -3.29247221e-02+2.28990460e-01j,  1.72097871e-02+3.03322059e-02j,  3.56471471e-02+3.53238951e-02j, -7.01989588e-02+1.25589194e-01j, -1.27727238e-01-2.56240149e-02j,  1.11420554e-01+1.79185848e-02j, -6.33720285e-02-5.24980027e-02j,  1.69408311e-02+2.10972901e-02j,  4.61900829e-02+2.30128229e-02j,  1.64914084e-01+9.53427779e-02j, -8.10370774e-02-4.02999617e-02j,
       -2.28926173e-02+6.01414298e-03j,  4.15959537e-02-1.62844430e-02j, -1.04015567e-01-2.28197070e-01j,  5.70917647e-02+6.10634039e-02j,  1.86651786e-02-1.32786359e-01j,  3.52815399e-02+2.63096329e-01j, -5.20942911e-03+9.32562283e-02j,  6.02358871e-02-2.03967495e-01j,  3.07765966e-02+4.43833301e-02j, -1.25342748e-01+9.30307464e-02j, -8.02078208e-02-7.48545491e-02j,  4.41479684e-02+4.55319347e-02j, -1.30358039e-02+3.44938629e-02j,  2.59548001e-03+8.55018910e-02j, -7.24207391e-02+7.25371152e-02j,  4.75649287e-02+8.49041681e-03j,  7.98960425e-02-2.09135805e-02j, -8.16200773e-03+8.52944822e-02j,  5.76132389e-02+7.92413488e-02j, -7.86674994e-03-5.18220519e-02j,  1.61866259e-03+4.58226858e-02j,  2.10573498e-02+1.21302913e-02j,  1.50642093e-01+3.18694799e-02j, -5.11976007e-02+3.35902634e-01j,  1.30304319e-02+1.20557949e-02j,  1.93169643e-02-2.04117390e-02j,  4.21485086e-03+4.50653899e-02j, -6.99644728e-02-4.38648373e-02j, -5.57760973e-02+1.03144757e-01j, -2.77981225e-04-6.55103561e-02j,
       -2.47429687e-02-5.56510136e-02j, -1.30487865e-01-2.53886778e-01j, -5.31332551e-02+1.83257653e-01j,  4.49553366e-02-1.27860901e-01j, -1.41877991e-01-9.17018436e-02j,  1.41319426e-01+9.76950302e-02j,  1.69036782e-02+4.32713109e-02j, -2.21736722e-02-1.35943321e-02j,  8.45566133e-02+1.56861643e-01j,  6.06872432e-02+1.10300630e-01j,  8.18810463e-03+4.90937472e-02j,  2.59253095e-02+2.16031285e-02j,  2.91014435e-04-1.31594533e-01j, -1.54684856e-02+2.54619375e-02j, -2.12487171e-02+3.36985235e-02j, -2.13034153e-01-3.81858886e-03j, -2.27912308e-02-3.15100249e-02j,  8.56614800e-02-4.73562392e-02j, -3.21653523e-03-1.00666344e-01j,  4.76758478e-02+1.13788672e-01j,  1.04992660e-01-7.19338205e-03j,  2.61845427e-02-1.95367436e-04j, -5.70545419e-02-8.24770580e-02j,  2.59511921e-02-1.47959999e-01j,  9.90365911e-02-1.07073452e-01j,  4.24238401e-02+6.83240999e-04j, -9.29305355e-03+2.57478648e-02j,  5.09306367e-02+5.17850540e-02j, -3.35412235e-02+5.12389402e-02j,  2.46636595e-02+1.52497496e-01j,
       -3.69885214e-02-1.43051224e-03j, -6.34083688e-02+7.95585662e-02j, -7.63159733e-02-9.59472692e-02j,  1.37701675e-01+1.72715523e-02j,  4.43926374e-03+1.03978366e-01j, -9.76045082e-04+1.16177364e-01j,  9.75501498e-02+1.10711036e-01j, -8.61395371e-02+4.75590898e-02j, -1.86326972e-02-8.72703099e-02j, -3.98222506e-02-2.01927936e-03j, -4.56659343e-02+6.85048391e-03j, -8.52516866e-02-3.58323276e-03j,  6.43698794e-02+9.95628825e-02j, -4.39734955e-02+2.10779642e-02j,  1.14785069e-01+1.09119788e-01j, -1.80570955e-01-6.60318868e-02j,  1.82596842e-02+1.32847306e-01j, -1.25277200e-01+1.85473095e-02j,  3.61733437e-02+1.42379572e-02j, -8.92845204e-03+6.77236053e-02j,  1.58690519e-01+6.45710673e-03j, -1.24921492e-02-9.99208311e-02j, -1.91513019e-01+3.48823651e-02j, -9.84584655e-02+1.76559101e-02j, -8.56099318e-02+1.49766608e-01j, -6.12707229e-02+1.07577597e-02j,  1.16550002e-01-2.40802989e-01j, -1.23620543e-02+4.45149950e-02j, -1.76292863e-01-1.09460504e-01j,  3.85853397e-02-5.33685000e-02j,
       -4.33475627e-02+6.06156460e-02j, -1.87846565e-02+1.14931789e-01j, -1.02013650e-01-2.29362151e-02j,  6.16651648e-02+7.99999963e-03j, -3.24185056e-01-1.48847542e-01j, -1.15048411e-02-5.67954670e-02j, -1.31298592e-01+9.40862751e-02j, -3.38096707e-02-4.96639503e-03j,  1.01303563e-01-1.20893708e-01j,  3.69567985e-02-7.34673617e-02j,  1.17120440e-01+1.46681717e-01j, -1.08104493e-01+4.59797898e-02j,  1.57388412e-02-9.05862553e-02j,  1.28683206e-01+1.36862876e-01j,  1.20579464e-01+4.56797568e-02j,  5.32546371e-03+2.00030811e-03j, -4.47207586e-02+6.84790769e-02j, -4.28425820e-03+7.32050329e-02j, -5.49771829e-02+3.00964687e-02j, -2.52478091e-02+1.04723922e-01j, -3.75356066e-03-1.20885506e-01j,  3.37046879e-02+4.44937988e-02j, -5.52111021e-02+4.18487207e-02j,  6.53305005e-02+2.24424425e-03j, -4.86068425e-02+1.00608085e-03j,  1.60271931e-01+6.63649255e-02j,  4.60478432e-02+3.98226836e-02j,  3.26150010e-02+1.82968788e-01j, -9.10672877e-02-1.11311630e-01j,  1.00893232e-02-1.39737503e-02j,
        2.71754161e-02-2.34544035e-02j,  9.15977387e-02+1.99426961e-02j, -1.97642710e-01+9.12888621e-02j,  4.74121316e-02-5.13504981e-02j,  5.07225943e-02+9.46633787e-02j, -9.53911293e-02-1.80654690e-01j,  1.64115127e-01-6.50378535e-02j,  1.09449061e-01+5.60124725e-02j,  1.53796325e-01+5.18189982e-02j,  5.17206905e-02+1.90530640e-01j,  7.50057667e-02-1.03567448e-01j, -3.82878661e-02-1.80075937e-01j,  4.01978788e-02-1.00698223e-01j,  1.03377853e-03-3.07692277e-02j,  1.20225227e-01+2.07456969e-01j,  1.04309283e-01-9.26611183e-02j, -1.03361361e-01+1.95889640e-02j,  2.58259256e-02+1.58281886e-01j,  7.75239291e-02-1.01311736e-01j,  9.15117078e-02-7.60188931e-03j,  1.14475846e-01-6.79271552e-02j,  5.44144608e-02-1.28520982e-01j, -9.52291620e-02-6.55706207e-02j,  3.43890303e-02-2.76969004e-02j, -1.02611436e-01-1.50417588e-01j,  3.21748176e-02-3.07119412e-02j, -3.86609986e-02+7.35572270e-02j, -5.24141267e-02-2.68636860e-02j,  1.15398753e-02+5.32210898e-02j, -1.73433735e-01-1.20650971e-01j,
        1.59124482e-01-1.31576336e-02j,  1.20225299e-01-1.52593103e-01j, -9.55610838e-02-6.95071254e-02j,  5.40075010e-03+3.80444187e-02j, -1.17110757e-02-3.28355329e-02j, -3.14854352e-02+9.13669332e-02j, -2.11449648e-02-7.11967389e-02j, -1.05950323e-02+1.75284874e-01j,  1.84744416e-02-6.85391690e-02j,  3.46360131e-02+4.62850612e-02j,  8.41213852e-02-1.24326928e-02j,  9.98813964e-02+2.08812494e-02j,  5.76777438e-03-5.35307536e-02j, -9.73557829e-02+2.03322351e-01j, -7.32834396e-03+3.51292305e-02j,  1.97020322e-01-3.69066750e-02j, -4.20662160e-02-1.01225755e-01j,  4.66345435e-02+4.48953960e-02j, -2.96526616e-02+2.70537698e-02j, -1.12505815e-01-4.81118731e-02j,  6.21206830e-02-7.15323882e-02j, -4.47070431e-02+1.45161193e-01j,  5.16437799e-02+1.48994784e-01j, -4.60222096e-02-9.52131423e-02j,  5.53010620e-02-4.16026288e-02j,  3.29589607e-01-8.01715204e-02j,  7.43597136e-02-8.52844592e-02j, -1.50760562e-02-1.18150067e-01j,  2.04861788e-01+5.84783818e-02j,  1.93288780e-02-2.43307354e-02j,
       -1.99745724e-01+3.64362975e-02j, -3.43725281e-02-6.21676876e-02j,  3.63703503e-02-6.52874052e-02j,  8.94751529e-02+4.47611628e-02j, -6.65470145e-02-7.98519549e-02j,  1.01433775e-02-7.72488898e-02j,  1.06307469e-01+7.79035119e-02j, -4.78507257e-02-6.23415087e-02j,  3.78893691e-02+2.62410143e-02j,  1.73318720e-01-3.21495796e-02j, -1.65681634e-01+6.67285599e-02j,  1.94291731e-01-2.89109408e-02j, -4.54548567e-02+5.83158238e-02j,  5.61383335e-02+3.83324113e-02j,  1.21596304e-01+1.70747464e-01j,  1.27954274e-01-1.93444819e-03j, -8.37891724e-02-8.64889585e-02j,  3.80499478e-02+2.01027939e-02j,  9.43965724e-02-8.90623338e-02j, -4.79597835e-02+4.75603864e-02j, -8.61574607e-02+8.32130201e-02j, -1.55412399e-02-1.27006036e-02j, -7.42519286e-02-8.61685534e-02j, -8.10108175e-02-6.16012333e-03j, -7.37177736e-02-4.44450757e-02j,  1.43980441e-01+5.66282763e-02j, -7.16666783e-02-1.66644680e-01j,  1.52513989e-01+5.67736253e-02j, -7.22985818e-02+4.50107611e-03j,  8.57243393e-02+1.44598168e-01j,
        2.61325291e-03-1.17556073e-01j,  7.88519979e-02+6.74275525e-02j,  7.41806885e-02-4.95153472e-02j, -1.84676918e-03+3.29891980e-02j,  3.84952719e-02+1.48034075e-01j,  3.04972039e-02-7.37869287e-02j,  7.01507010e-02-2.44450088e-02j,  3.33009500e-03+1.05487204e-04j, -4.25430453e-02+5.86220817e-02j, -1.05973079e-01+7.39664382e-02j,  2.92878309e-02+5.28859984e-02j,  5.61329338e-02+1.69166909e-01j,  5.28499244e-02+3.45360084e-02j, -6.53900660e-02-8.61522262e-02j,  7.96946190e-02-3.41452511e-02j,  5.22770869e-02-7.40489497e-03j, -1.88772294e-01+3.96780063e-03j, -2.32936723e-02-5.37723356e-03j, -1.24854292e-01-1.40224821e-02j,  3.07904415e-02-3.15381949e-02j, -3.25235318e-02+8.42701028e-03j,  1.66862279e-01+1.19354852e-01j,  1.57978914e-01+3.63014900e-02j,  8.67807480e-02+1.40379161e-01j, -5.07193433e-02-1.40569523e-02j, -5.00888412e-02-2.23435528e-02j, -7.17048440e-02-9.81330153e-02j, -2.12409092e-02+5.45752543e-02j,  6.06474942e-02+5.88358912e-02j,  6.80174213e-02+3.30162415e-02j,
       -6.07282111e-02+1.02939777e-01j, -1.87732930e-02+1.92200565e-02j, -1.16626047e-01+6.45786333e-02j,  1.37742795e-01+8.42180324e-02j, -1.20841908e-01+7.65002919e-02j,  3.46012372e-02-4.84411652e-02j, -1.01213514e-02+1.20822878e-01j,  1.20196281e-02-3.67275266e-02j,  3.92842430e-02+2.04356272e-02j, -3.50278977e-02+1.22347382e-01j,  1.91917372e-01-6.95304549e-02j,  9.29916189e-02+6.32480707e-02j, -1.40237138e-01+4.73312139e-02j, -5.46257907e-03-3.43833080e-02j,  5.25735063e-02+1.22349339e-02j,  5.55091124e-02+4.13346745e-02j,  3.80292721e-02+8.69044043e-02j, -1.57996649e-01-7.65619514e-02j,  1.75325367e-03-1.92975200e-02j,  4.93539327e-02+1.78264744e-01j,  1.02628124e-01-9.66824408e-02j, -3.21786689e-02+4.29915452e-02j, -8.65810365e-02+1.13052509e-02j, -3.85279691e-02+6.40545958e-02j,  1.08229295e-02-8.43402245e-02j,  1.79862749e-01+7.88074870e-03j, -2.87530932e-02+2.46179737e-01j, -1.27639034e-01+1.94348924e-04j,  8.38416209e-02+1.18533050e-01j, -9.80168967e-02-2.75891572e-02j,
       -1.18741656e-01-1.12401688e-01j,  1.73015815e-01+3.17142696e-02j, -1.12689840e-01-6.31993322e-03j, -1.42069958e-01+1.71043064e-01j, -7.41914581e-02-1.51931975e-01j, -3.66012909e-02+2.39026896e-03j,  5.14718439e-02-1.03341673e-01j, -4.72099830e-02+1.26667668e-02j,  1.37512313e-02-2.60230700e-02j, -6.00589937e-02+2.54281176e-03j, -1.55002558e-02+6.29291268e-02j, -1.19615201e-01+6.77959328e-02j,  2.31292548e-02-1.08782093e-01j, -1.09964341e-01+1.25294910e-01j, -7.47861771e-02-5.98894488e-02j,  4.71331416e-02+1.43755876e-01j, -1.38666615e-02+1.01256745e-02j, -1.00179410e-02+1.11863487e-01j,  6.31224250e-02+1.19440718e-01j, -4.16177531e-02-4.36345510e-02j, -3.48014985e-02+8.85623335e-02j,  1.24650686e-01+1.48734266e-01j, -2.24549336e-02-1.00924355e-01j, -1.02751585e-01-2.12847917e-01j, -1.05847008e-01-5.29000688e-02j, -4.67909943e-02-1.36206316e-01j, -4.85632359e-02+1.42473156e-01j, -1.24246749e-01+6.42615904e-02j,  5.81696877e-03-4.96687519e-02j, -2.24424614e-02-3.88848488e-02j,
       -7.74915273e-02+1.54367388e-02j,  4.34592454e-02-1.56021904e-01j, -8.56892132e-02+1.24030217e-01j, -4.89507658e-02+5.90317176e-02j, -1.25697664e-01-2.32861745e-02j, -1.61863374e-01+1.19418496e-01j, -2.90213516e-03-2.82791717e-02j,  4.50605390e-02-1.85114054e-01j,  1.02768328e-01-7.98889081e-02j, -8.15041765e-02+7.49613006e-02j,  7.62836130e-02-1.09942641e-01j, -2.43957305e-02+1.60075781e-02j,  1.25368088e-01-4.97508023e-02j,  4.32447252e-02-6.48555547e-02j, -2.64345140e-02-1.18679637e-01j, -5.49407140e-02+2.89175240e-02j, -4.47502071e-02+7.51395018e-02j, -4.99554424e-02-1.05535549e-01j,  7.47066390e-02+1.23133955e-01j,  1.03069754e-01-1.57446612e-01j, -2.38467760e-01+1.51184192e-01j,  3.55395997e-02-1.03355238e-01j, -2.32302845e-02+4.74672683e-02j,  8.73424064e-02+9.29539222e-03j,  1.37790158e-01-1.37543346e-01j, -9.47568754e-02+4.59808420e-02j,  1.41612817e-01-7.50145993e-02j, -1.21252993e-01+2.11071077e-02j,  5.93864166e-03-1.03518747e-01j,  1.00814386e-01-1.08636130e-01j,
        8.47764700e-02-1.36804644e-01j, -4.06295973e-02-3.17316929e-02j, -1.29388374e-01-1.13022235e-01j,  4.05034297e-02-1.51303513e-01j,  5.35895225e-02-9.81906501e-02j,  1.01307296e-02-6.14659966e-02j,  7.19860763e-03-7.87661099e-02j,  6.87236698e-03+7.01353724e-03j,  1.84117743e-01+1.14586080e-01j,  3.91942011e-02-1.67427533e-01j, -4.34340225e-02-2.41238971e-02j, -4.20038485e-02+5.76875010e-02j,  5.23551179e-02-3.79231514e-02j,  1.17465353e-01+1.19464437e-01j, -9.14539775e-02+9.32704953e-02j, -3.28255936e-02+2.00709680e-01j, -3.47973061e-02-6.96489860e-02j,  5.68759165e-02+4.46913265e-02j, -4.55834517e-02-6.49989637e-03j,  6.92236117e-02+6.31449747e-02j, -1.00908352e-01-7.69839513e-03j,  1.81960295e-02-3.79970705e-03j,  1.68374360e-02+1.05842359e-02j,  1.28460284e-01+6.09000930e-02j, -1.02077544e-01+6.69284634e-02j, -1.53969863e-02-1.52720337e-02j,  1.81218895e-01+3.16794729e-02j, -1.24806065e-01+2.26699104e-02j, -3.03130859e-02-2.23873943e-02j, -6.99493462e-02-7.03904849e-03j,
       -3.67200875e-02+2.84145877e-02j,  9.43878591e-02+7.21251481e-02j,  6.58129601e-02-3.60549923e-02j,  6.61779478e-02-3.04054186e-02j, -6.13842814e-02-6.07921661e-02j,  8.98783698e-03-7.41166890e-02j,  1.16392800e-02-2.36748091e-02j, -1.10768989e-01+4.56420908e-02j,  3.72471227e-03-1.25668424e-03j, -1.29167025e-01+5.63022023e-02j,  1.05866530e-01+1.86155416e-02j, -6.67515779e-02+6.46395437e-02j, -1.03224770e-02-6.42898639e-02j,  6.21606257e-02-5.26347583e-02j,  2.57841919e-02+3.56828380e-02j, -8.55918562e-02+1.63670027e-01j, -4.31423671e-02+7.94041400e-03j,  8.59410574e-03+5.25343549e-02j, -1.33389296e-02+4.52481973e-03j,  7.58536209e-02+7.06748771e-02j, -4.14761871e-02-1.02294821e-01j, -6.50116056e-02-2.21306166e-02j,  2.33077150e-01+5.67852132e-02j,  4.86390845e-02-2.02001648e-03j,  7.53644961e-02-1.89453116e-02j,  2.08862978e-02-1.49855086e-02j,  1.33052297e-02+1.01522608e-01j,  1.51083846e-01+1.12903477e-01j, -1.62667787e-01+6.56239071e-02j,  1.45208158e-01-5.36623261e-02j,
       -6.95826831e-02+1.39196707e-01j,  3.29916774e-02-6.75590243e-02j,  3.95917337e-02+5.93925052e-02j, -2.26219692e-02-2.87050486e-02j, -7.18277375e-02-2.33909540e-01j,  3.23958840e-02+6.33185484e-02j,  6.84165483e-02+1.09982301e-01j, -1.76022868e-02+1.67477409e-01j,  1.23647618e-01+5.72347084e-02j,  1.01034190e-01-2.91422180e-02j, -9.75792719e-02+6.92072056e-02j,  2.24926784e-02+4.92453456e-02j,  9.72881515e-02-1.19536229e-02j, -3.46137229e-02-2.17219106e-01j,  1.18110854e-01-2.25904054e-02j,  7.03378796e-02+3.42026266e-02j, -8.74713404e-02+8.22715157e-03j, -1.03981674e-01-1.24708617e-01j,  3.55529717e-02-3.88864430e-02j, -1.37342148e-01-9.40956813e-02j,  4.45450338e-02+2.39754202e-01j,  2.41743357e-02+2.06386810e-01j,  1.08491505e-01+8.89560185e-02j,  5.47927931e-02+4.36730443e-02j, -1.77677037e-01-1.15306877e-01j,  2.38957141e-02+1.47875042e-01j,  4.55000869e-02-2.62193008e-02j,  2.12153347e-01+1.42025329e-01j, -5.30709017e-02-7.21771704e-02j, -5.46350714e-02+5.94590117e-02j,
        4.20227987e-02+5.33808606e-02j, -5.44950519e-03-1.02152548e-01j, -8.52371557e-02+6.87526175e-02j, -9.88849806e-02+8.63653429e-02j, -3.29535337e-02+1.11468767e-01j, -8.91525767e-02-1.10460050e-01j,  8.65861644e-02+6.93102010e-04j, -9.60730971e-02+1.56945079e-01j, -5.14109278e-03-2.81513903e-02j, -1.40068665e-01+2.86562459e-02j, -9.83017425e-02+5.21354581e-02j,  4.69469551e-02-8.89922402e-02j,  2.21941752e-02-5.57626726e-03j,  1.66024586e-01-7.95503934e-02j, -1.38920743e-01+6.04827362e-02j,  8.25451438e-02-6.28766108e-02j,  4.25038037e-02+1.82529097e-02j,  2.62973135e-02-4.18821635e-02j, -1.45761903e-01-1.47286365e-01j,  9.89672045e-02-1.24387645e-01j, -5.78704218e-02-1.62675064e-02j, -4.71981727e-02+2.44048888e-01j,  3.99979719e-02-5.61776567e-02j, -5.90057528e-03-2.06685934e-02j,  1.00743108e-01+1.26113923e-01j, -8.30394273e-02-8.67700959e-02j, -2.42292161e-02+5.45664588e-02j,  1.03200164e-01+4.38793374e-03j, -1.29760046e-01-1.16149074e-01j, -4.06013934e-02-2.91615196e-02j,
       -7.66643814e-02-2.82129167e-02j,  5.18827442e-02-9.09744909e-03j,  1.74289199e-02+1.30371358e-01j, -2.12748239e-01+8.60445151e-02j, -4.50027204e-02-4.68921603e-03j, -1.65936974e-02+9.74970666e-03j,  1.89314727e-02-1.52142208e-02j,  1.40710985e-03-7.24579715e-02j, -2.85195855e-02-4.64302487e-02j,  9.28808388e-02-5.38966974e-02j, -8.37647013e-02+3.87783304e-02j, -7.93612636e-02+5.52106771e-02j, -8.24627154e-02-7.29275080e-02j, -9.95466967e-02-2.55863384e-02j, -1.11330369e-01-1.07026510e-02j,  9.03801665e-02+7.48566039e-02j, -1.57610909e-02-2.11373355e-03j, -1.29136461e-01-1.57473543e-02j,  1.44036925e-01-1.17830341e-01j, -1.67628907e-02+1.06263346e-01j, -1.78654592e-02-1.01880945e-01j, -2.49916456e-02+4.60896420e-02j,  2.10042132e-01-7.70664131e-02j, -7.63588341e-02-3.73507331e-02j, -7.46108448e-02+1.98266460e-02j,  7.96343195e-03+3.98058310e-02j,  7.08286705e-02+5.89683331e-02j,  9.06570839e-03-1.68675820e-01j, -7.58415198e-02-1.61708205e-02j,  1.63594232e-01+1.06080863e-01j,
       -5.27043428e-02+1.97776646e-01j,  1.68672907e-01+1.72692116e-01j,  1.20883554e-01-4.19624503e-02j, -4.06781283e-02-3.62701240e-02j, -1.81279165e-02-5.02802312e-02j,  1.06586105e-01-6.34529844e-02j,  2.24092969e-01-1.34142134e-02j,  4.64319738e-02-5.98185270e-03j, -7.72761980e-02+6.83240697e-02j, -2.47197188e-02-6.64142633e-02j,  6.12047543e-02-5.67342772e-02j,  6.89583775e-02-7.32779423e-02j, -9.73665566e-02-1.38092077e-01j,  9.42299578e-02-7.83134489e-02j,  4.66576304e-02+5.01211578e-02j, -5.84347124e-03+2.76985347e-02j,  2.32720197e-02-4.52208360e-03j,  1.03896607e-01-4.81745519e-02j, -2.53632308e-02-9.39062130e-02j, -1.33410903e-01+1.61699487e-01j,  1.41818815e-01+7.56963929e-02j, -1.38784180e-01+1.42229126e-01j, -7.82906496e-02+1.02452713e-01j,  1.62728062e-01+5.26600513e-02j, -4.70320160e-02+4.62832652e-02j, -1.34884657e-03+5.09580708e-02j, -1.21163909e-01+1.96909871e-01j, -2.72261417e-01-6.42506010e-02j,  2.86136429e-02+8.86315131e-02j,  5.31085101e-02-2.71828288e-02j,
       -3.21727877e-02-5.77796931e-02j, -9.44638527e-02+1.37507662e-02j, -1.21657787e-01+8.65124885e-02j, -4.96839239e-03-2.97644254e-02j, -1.19540366e-01-1.34414909e-01j,  1.30160754e-01-1.26480898e-02j, -7.18591735e-02-2.46715270e-02j,  1.36140893e-01+4.14359423e-02j, -5.71317147e-03-1.18124735e-01j,  3.73988883e-02-2.36730347e-02j,  5.33823290e-02-8.47267547e-03j,  5.80851057e-02-3.92213264e-02j,  2.96021470e-02+1.06824598e-01j,  9.52300406e-02+8.04315308e-02j, -2.19648403e-02-8.28498260e-02j, -2.89222027e-02-9.84731848e-02j,  1.10461055e-01-1.58070614e-01j,  2.09076408e-01+3.68001449e-02j, -6.73813906e-02+9.16108413e-03j,  3.03631342e-02+2.95543290e-02j, -4.54437979e-03-3.18493443e-03j, -7.63023942e-02+1.55592989e-01j, -3.19220889e-02+4.73941064e-02j,  7.13699814e-02-5.69129591e-02j, -9.54995207e-02-3.92267051e-02j, -3.94296081e-02-1.51384146e-01j,  9.68318889e-02-7.70409747e-02j,  1.25939378e-02+3.18634620e-02j, -6.89127002e-02+7.07882363e-02j,  2.43254950e-02+1.16436448e-01j,
       -9.91874227e-03-5.42893449e-02j,  1.23743411e-01+4.78200570e-02j, -5.85714409e-02+3.39221080e-02j,  3.65412457e-03-4.02554546e-03j, -2.05342433e-02+2.06713323e-01j, -3.02186741e-03+1.99788433e-02j,  1.16854339e-01-1.15247241e-01j,  9.62932923e-02+3.42143052e-03j,  4.26837528e-02+1.80563531e-01j, -1.23230416e-01+8.43397964e-02j, -1.66241403e-01-7.38559917e-02j,  8.12086119e-02-6.97547686e-03j, -7.31213186e-02+7.79960115e-02j,  2.85823841e-02-2.07511068e-02j,  8.27080727e-02+4.36355847e-02j, -1.10859884e-01-1.36676547e-02j,  1.28303434e-01-1.06149501e-02j, -1.46242136e-01+7.04963534e-02j, -8.26304595e-02-4.38762538e-02j,  1.71071748e-01+4.02667862e-02j, -1.35488426e-01-6.87212472e-02j, -4.67922162e-02-1.03931421e-02j, -2.44813702e-02-5.53092986e-02j,  4.73878499e-02+2.91459937e-02j,  1.98580413e-02-6.12412029e-03j,  1.10488978e-01-2.45764352e-02j,  8.00241132e-02-8.57174156e-02j,  1.03702916e-02-5.49489277e-02j,  3.93099516e-02+3.06121558e-02j, -3.02550594e-02+1.18718205e-01j,
       -1.36619382e-01+2.34100763e-02j, -1.45312692e-01-2.24127604e-01j, -2.38248910e-02-1.11579180e-01j,  2.29159458e-02+6.19272697e-02j,  7.32380697e-02+1.36444819e-01j, -4.61302116e-02-2.18822134e-01j, -4.49007145e-02-2.24886131e-02j, -1.47568288e-01-2.04472654e-02j,  2.73175459e-02-2.19629877e-02j,  1.24524515e-01+9.26245193e-02j, -3.55583512e-02+5.09874867e-02j,  1.81713481e-01+8.06582753e-02j,  3.11561167e-02-1.22752357e-01j, -1.72074875e-01-1.29423169e-01j,  5.08999251e-02-6.26136051e-02j,  1.02390035e-01+8.45685534e-02j,  5.52593493e-02+3.68538488e-02j, -5.70061651e-02-4.25009402e-02j, -1.56072617e-02+8.62563257e-03j, -5.21205630e-02+2.36450605e-02j, -1.08956603e-01+5.57188609e-02j, -6.99571459e-02-3.51674530e-02j,  4.86802380e-02+3.37359307e-03j, -4.13618066e-02-2.12316403e-02j, -1.46621977e-01-4.74596174e-02j, -9.63426212e-02+1.72166337e-02j,  2.51705991e-02+3.21369206e-01j,  4.96769009e-02+3.48387702e-02j, -1.04762586e-02+1.56285574e-01j, -3.67404105e-03+7.68483799e-02j,
       -4.46016110e-02+1.95201429e-02j,  4.38113722e-02+8.65962895e-03j,  7.74746787e-02+6.94607249e-02j,  2.47130647e-02-1.12600318e-02j, -7.48410468e-02+7.24300380e-02j, -3.77887553e-02-4.43488254e-02j,  1.32708760e-01+4.87164738e-02j,  1.63778016e-01+1.40758747e-01j,  8.20076371e-02-1.79740773e-01j,  5.49371276e-02+3.22196346e-02j, -2.37249431e-02-1.30833713e-01j,  1.19126218e-01+1.15211057e-01j, -8.03659026e-03+2.24442260e-02j,  9.06621949e-02+1.69073869e-02j, -9.84253648e-02-1.22959233e-01j, -1.13106787e-01-1.00708166e-02j]

        u1 = ql.Unitary("big6qubitone", matrix)


    
    def test_usingqx_sparseunitary(self):
        num_qubits = 5
        p = ql.Program('test_usingqxsparseunitary', platform, num_qubits)
        k = ql.Kernel('akernel', platform, num_qubits)

        matrix = [-0.11921901-0.30382154j, -0.10645804-0.11760155j,       -0.09639953-0.0353926j , -0.32605797+0.19552924j,        0.0168262 -0.26748208j, -0.17808469+0.25265196j,       -0.24676084-0.23228431j, -0.02960302+0.23697569j,
       -0.12435741-0.07223017j,  0.00178745+0.14813263j,       -0.11173158+0.26636089j,  0.27656908+0.05229833j,       -0.02964214-0.01505502j, -0.26959616+0.23274949j,       -0.18183627-0.04041783j,  0.05385991-0.05587908j,
        0.17894461-0.25668366j, -0.01553181-0.07446613j,        0.1876467 -0.49135878j, -0.18292006-0.04599956j,       -0.01618695+0.21951951j,  0.06003169-0.12728871j,       -0.04276406+0.08327372j,  0.30102765+0.18403071j,
       -0.08122018-0.08375638j, -0.02971758+0.09096399j,        0.10753511-0.03359547j, -0.1596309 +0.20649279j,       -0.13684564+0.29450386j,  0.20557421+0.24856224j,        0.0683444 +0.01780095j, -0.22317907-0.12123145j,
       -0.0323504 -0.02668934j,  0.08743777-0.49956832j,       -0.30202031-0.22517221j, -0.10642491-0.11883126j,       -0.13756817-0.20632933j,  0.02593802+0.00583978j,        0.05130972+0.06678859j, -0.10426135-0.14411822j,
        0.12318252+0.28583957j,  0.04903179-0.31898637j,       -0.07650819-0.07261235j, -0.22918932-0.28329527j,       -0.26553775+0.04563403j, -0.07728053+0.14952931j,       -0.10271285-0.00216319j, -0.09000117+0.09055528j,
       -0.15385903+0.01767834j,  0.42229431-0.05610483j,       -0.11330491-0.05458018j,  0.01740815-0.01605897j,       -0.11908997-0.01830574j,  0.21139794-0.10602858j,       -0.23249721-0.25516076j, -0.29066084-0.19129198j,
        0.21273108-0.14369238j, -0.20662513+0.14463032j,        0.2512466 -0.20356141j,  0.0869495 +0.24425667j,        0.09736427-0.03954332j,  0.1446303 +0.14263171j,       -0.25679664+0.09389641j, -0.04020309-0.19362247j,
        0.12577257-0.14527364j,  0.00371525+0.14235318j,       -0.22416134+0.02069087j,  0.03851418-0.05351593j,       -0.00289848-0.33289946j,  0.15454716-0.126633j  ,       -0.08996296-0.09119411j, -0.00804455-0.19149767j,
       -0.13311475-0.47100304j, -0.13920624-0.16994321j,       -0.05030304+0.16820614j,  0.05770089-0.15422191j,       -0.23739468-0.05651883j,  0.19202883+0.03893001j,        0.48514604+0.01905479j, -0.01593819-0.06475285j,
        0.31543713+0.41579542j, -0.08776349+0.24207219j,       -0.07984699-0.12818844j,  0.00359655+0.02677178j,       -0.12110453-0.25327887j, -0.21175671-0.1650074j ,       -0.14570465-0.05140668j,  0.06873883-0.01768705j,
       -0.13804809-0.16458822j,  0.15096981-0.02802171j,       -0.05750448-0.18911017j, -0.01552104+0.03159908j,       -0.0482418 +0.09434822j,  0.1336447 +0.22030451j,       -0.3771364 -0.17773263j,  0.16023381+0.26613455j,
        0.12688452-0.07290393j,  0.14834649+0.08064162j,       -0.06224533+0.04404318j,  0.03464369+0.19965444j,       -0.38140629-0.18927599j, -0.19710535-0.178657j  ,       -0.0507885 +0.19579635j,  0.11741615+0.13922702j,
        0.2673399 -0.01439493j,  0.10844591-0.19799688j,        0.01177533+0.031846j  , -0.07643954+0.25870281j,        0.28971442-0.25385986j, -0.23713666+0.01838019j,        0.1731864 -0.09372299j, -0.36912353-0.02243029j,
        0.03562803-0.09449815j,  0.13578229-0.19205153j,        0.21279127+0.14541266j, -0.20195524+0.187477j  ,       -0.06326783+0.0134827j ,  0.26953438-0.11153784j,       -0.28939961-0.08995754j,  0.20662437-0.15535337j,
       -0.03615272+0.00848631j,  0.14124129-0.10250932j,        0.08990493-0.13010897j, -0.04547667+0.17579099j,       -0.01292137+0.10354402j, -0.21338733-0.11928412j,        0.19733294+0.12876129j,  0.35162495+0.45226713j,
        0.17112722-0.18496045j, -0.34024071-0.09520237j,        0.18864652-0.07147408j,  0.31340662+0.24027412j,       -0.0720874 -0.11081564j,  0.08727975+0.02830958j,       -0.07584662-0.22555917j,  0.07086867-0.27714915j,
       -0.19116148-0.02164144j, -0.24831911+0.1055229j ,       -0.09939105-0.24800283j, -0.15274706-0.12267535j,        0.05237777-0.09974669j, -0.18435891-0.1737002j ,       -0.20884292+0.1076081j , -0.31368958-0.02539025j,
        0.03436293-0.19794965j,  0.11892581-0.17440358j,       -0.03488877+0.02305411j,  0.29835292-0.08836461j,        0.07893495-0.16881403j,  0.21025843+0.13204032j,        0.17194288-0.06285539j, -0.0500497 +0.35833208j,
       -0.14979745-0.07567974j,  0.00193804+0.04092128j,       -0.07528403-0.18508153j, -0.16873521-0.09470809j,        0.50335605+0.00445803j,  0.11671956+0.30273552j,        0.10253226-0.13365319j,  0.16676135+0.18345473j,
       -0.10096334-0.24031019j, -0.18452241+0.05766426j,        0.18102499-0.13532486j,  0.06252468-0.18030042j,       -0.00591499+0.07587582j, -0.35209025-0.12186396j,       -0.25282963-0.26651504j, -0.13074882+0.14059941j,
        0.18125386-0.03889917j,  0.06983104-0.3425076j ,        0.37124455-0.00305632j,  0.04469806-0.31220629j,        0.16501585+0.00125887j,  0.15895714-0.14115809j,       -0.01515444+0.06836136j,  0.03934186+0.13425449j,
        0.0513499 +0.21915368j,  0.00089628-0.3044611j ,        0.05443815-0.05530296j,  0.12091374-0.16717579j,       -0.06795704-0.2515947j , -0.43324316+0.13138954j,        0.03753289-0.00666299j,  0.16823686-0.22736152j,
       -0.00567807+0.05485941j, -0.11705816+0.39078352j,        0.29136164+0.18699453j, -0.09255109+0.08923507j,        0.11214398+0.00806872j,  0.02971631+0.05584961j,        0.2561    +0.22302638j,  0.12491596+0.01725833j,
        0.23473354-0.19203316j, -0.09144197-0.04827201j,       -0.0630975 -0.16831612j,  0.01497053+0.11121057j,        0.1426864 -0.15815582j,  0.21509872-0.0821851j ,        0.00650273+0.42560079j, -0.15721229+0.09919403j,
        0.18076365-0.05697395j, -0.10596487+0.23118383j,        0.30913352+0.24695589j, -0.03403863-0.01778209j,       -0.07783213-0.25923847j,  0.06847369-0.2460447j ,       -0.24223779-0.10590238j,  0.15920784+0.21435437j,
        0.26632193-0.02864663j,  0.06457043+0.0577428j ,       -0.38792984+0.08474334j,  0.00944311+0.22274564j,        0.11762823+0.36687658j, -0.1058428 -0.2103637j ,       -0.12970051-0.27031414j,  0.12684307+0.08398822j,
        0.06711923+0.23195763j, -0.04537262+0.26478843j,        0.10253668-0.07706414j, -0.13531665-0.27150259j,       -0.09124132-0.23306839j, -0.08631867+0.17221145j,        0.17654328-0.10341264j,  0.11171903-0.05824829j,
        0.04708668-0.13436316j, -0.10544253+0.07083904j,        0.04191629+0.28190845j, -0.4212947 -0.28704399j,        0.10278485+0.05713015j,  0.02057009-0.19126408j,        0.04856717+0.26648423j,  0.05388858-0.32433511j,
       -0.09408669-0.12159016j, -0.01355394+0.04757554j,        0.10925003-0.0453999j , -0.02512057-0.23836324j,        0.31375479-0.0993564j , -0.14702106+0.33395328j,       -0.1608029 +0.11439592j, -0.11028577-0.0093615j ,
       -0.08440005-0.12376623j,  0.12932188+0.09711828j,        0.18574716-0.06392924j, -0.13048059+0.0287961j ,       -0.29552716-0.08768809j, -0.02439943-0.01548155j,        0.07775135+0.00727332j,  0.1561534 -0.06489038j,
        0.46665242-0.07708219j, -0.05251139+0.37781248j,       -0.3549081 -0.10086123j,  0.11180645-0.40408473j,        0.03031085+0.16928711j,  0.1190129 -0.10061168j,        0.0318046 -0.12504866j,  0.08689947+0.07223655j]

        u1 = ql.Unitary("testname",matrix)
        u1.decompose()
        k.hadamard(0)
        k.hadamard(1)
        k.hadamard(2)
        k.hadamard(3)
        k.cnot(0, 1)
        k.cnot(0, 2)
        k.cnot(0, 3)
        k.cnot(1, 2)
        k.cnot(1, 3)
        k.cnot(2, 3)

        k.gate(u1, [0, 1, 2, 3])

        p.add_kernel(k)
        p.compile()
        qx.set(os.path.join(output_dir, p.name+'.qasm'))
        qx.execute()
        c0 = qx.get_state()
        
        self.assertAlmostEqual(0.0625*helper_prob((matrix[0]  +  matrix[1] +  matrix[2] +  matrix[3] +  matrix[4] +  matrix[5] +  matrix[6] +  matrix[7] +  matrix[8]  +  matrix[9] +  matrix[10]+  matrix[11]+  matrix[12]+  matrix[13]+  matrix[14]+  matrix[15])), helper_regex(c0)[0], 5)
        self.assertAlmostEqual(0.0625*helper_prob((matrix[16] +  matrix[17]+  matrix[18]+  matrix[19]+  matrix[20]+  matrix[21]+  matrix[22]+  matrix[23]+  matrix[24] +  matrix[25]+  matrix[26]+  matrix[27]+  matrix[28]+  matrix[29]+  matrix[30]+  matrix[31])), helper_regex(c0)[1], 5)
        self.assertAlmostEqual(0.0625*helper_prob((matrix[32] +  matrix[33]+  matrix[34]+  matrix[35]+  matrix[36]+  matrix[37]+  matrix[38]+  matrix[39]+  matrix[40] +  matrix[41]+  matrix[42]+  matrix[43]+  matrix[44]+  matrix[45]+  matrix[46]+  matrix[47])), helper_regex(c0)[2], 5)
        self.assertAlmostEqual(0.0625*helper_prob((matrix[48] +  matrix[49]+  matrix[50]+  matrix[51]+  matrix[52]+  matrix[53]+  matrix[54]+  matrix[55]+  matrix[56] +  matrix[57]+  matrix[58]+  matrix[59]+  matrix[60]+  matrix[61]+  matrix[62]+  matrix[63])), helper_regex(c0)[3], 5)
        self.assertAlmostEqual(0.0625*helper_prob((matrix[64] +  matrix[65]+  matrix[66]+  matrix[67]+  matrix[68]+  matrix[69]+  matrix[70]+  matrix[71]+  matrix[72] +  matrix[73]+  matrix[74]+  matrix[75]+  matrix[76]+  matrix[77]+  matrix[78]+  matrix[79])), helper_regex(c0)[4], 5)
        self.assertAlmostEqual(0.0625*helper_prob((matrix[80] +  matrix[81]+  matrix[82]+  matrix[83]+  matrix[84]+  matrix[85]+  matrix[86]+  matrix[87]+  matrix[88] +  matrix[89]+  matrix[90]+  matrix[91]+  matrix[92]+  matrix[93]+  matrix[94]+  matrix[95])), helper_regex(c0)[5], 5)
        self.assertAlmostEqual(0.0625*helper_prob((matrix[96] +  matrix[97]+  matrix[98]+  matrix[99]+  matrix[100]+ matrix[101]+ matrix[102]+ matrix[103]+ matrix[104] + matrix[105]+ matrix[106]+ matrix[107]+ matrix[108]+ matrix[109]+ matrix[110]+ matrix[111])), helper_regex(c0)[6], 5)
        self.assertAlmostEqual(0.0625*helper_prob((matrix[112] + matrix[113]+ matrix[114]+ matrix[115]+ matrix[116]+ matrix[117]+ matrix[118]+ matrix[119]+ matrix[120] + matrix[121]+ matrix[122]+ matrix[123]+ matrix[124]+ matrix[125]+ matrix[126]+ matrix[127])), helper_regex(c0)[7], 5)
        self.assertAlmostEqual(0.0625*helper_prob((matrix[128] + matrix[129]+ matrix[130]+ matrix[131]+ matrix[132]+ matrix[133]+ matrix[134]+ matrix[135]+ matrix[136] + matrix[137]+ matrix[138]+ matrix[139]+ matrix[140]+ matrix[141]+ matrix[142]+ matrix[143])), helper_regex(c0)[8], 5)
        self.assertAlmostEqual(0.0625*helper_prob((matrix[144] + matrix[145]+ matrix[146]+ matrix[147]+ matrix[148]+ matrix[149]+ matrix[150]+ matrix[151]+ matrix[152] + matrix[153]+ matrix[154]+ matrix[155]+ matrix[156]+ matrix[157]+ matrix[158]+ matrix[159])), helper_regex(c0)[9], 5)
        self.assertAlmostEqual(0.0625*helper_prob((matrix[160] + matrix[161]+ matrix[162]+ matrix[163]+ matrix[164]+ matrix[165]+ matrix[166]+ matrix[167]+ matrix[168] + matrix[169]+ matrix[170]+ matrix[171]+ matrix[172]+ matrix[173]+ matrix[174]+ matrix[175])), helper_regex(c0)[10], 5)
        self.assertAlmostEqual(0.0625*helper_prob((matrix[176] + matrix[177]+ matrix[178]+ matrix[179]+ matrix[180]+ matrix[181]+ matrix[182]+ matrix[183]+ matrix[184] + matrix[185]+ matrix[186]+ matrix[187]+ matrix[188]+ matrix[189]+ matrix[190]+ matrix[191])), helper_regex(c0)[11], 5)
        self.assertAlmostEqual(0.0625*helper_prob((matrix[192] + matrix[193]+ matrix[194]+ matrix[195]+ matrix[196]+ matrix[197]+ matrix[198]+ matrix[199]+ matrix[200] + matrix[201]+ matrix[202]+ matrix[203]+ matrix[204]+ matrix[205]+ matrix[206]+ matrix[207])), helper_regex(c0)[12], 5)
        self.assertAlmostEqual(0.0625*helper_prob((matrix[208] + matrix[209]+ matrix[210]+ matrix[211]+ matrix[212]+ matrix[213]+ matrix[214]+ matrix[215]+ matrix[216] + matrix[217]+ matrix[218]+ matrix[219]+ matrix[220]+ matrix[221]+ matrix[222]+ matrix[223])), helper_regex(c0)[13], 5)
        self.assertAlmostEqual(0.0625*helper_prob((matrix[224] + matrix[225]+ matrix[226]+ matrix[227]+ matrix[228]+ matrix[229]+ matrix[230]+ matrix[231]+ matrix[232] + matrix[233]+ matrix[234]+ matrix[235]+ matrix[236]+ matrix[237]+ matrix[238]+ matrix[239])), helper_regex(c0)[14], 5)
        self.assertAlmostEqual(0.0625*helper_prob((matrix[240] + matrix[241]+ matrix[242]+ matrix[243]+ matrix[244]+ matrix[245]+ matrix[246]+ matrix[247]+ matrix[248] + matrix[249]+ matrix[250]+ matrix[251]+ matrix[252]+ matrix[253]+ matrix[254]+ matrix[255])), helper_regex(c0)[15], 5)


    def test_extremelysparseunitary(self):
        num_qubits = 4
        p = ql.Program('test_usingqx_extremelysparseunitary_newname', platform, num_qubits)
        k = ql.Kernel('akernel', platform, num_qubits)

        matrix = [-0.59111943+0.15726005j, -0.        +0.j        , -0.15509793+0.32339668j, -0.        +0.j        , -0.33317562+0.00860528j, -0.        +0.j        , -0.5566068 +0.27625195j, -0.        +0.j        , -0.        +0.j        , -0.        +0.j        , -0.        +0.j        , -0.        +0.j        , -0.        +0.j        , -0.        +0.j        , -0.        +0.j        , -0.        +0.j        
                , -0.        +0.j        , -0.59111943+0.15726005j, -0.        +0.j        , -0.15509793+0.32339668j, -0.        +0.j        , -0.33317562+0.00860528j, -0.        +0.j        , -0.5566068 +0.27625195j, -0.        +0.j        , -0.        +0.j        , -0.        +0.j        , -0.        +0.j        , -0.        +0.j        , -0.        +0.j        , -0.        +0.j        , -0.        +0.j        
                , -0.00563624-0.59423429j,  0.        -0.j        ,  0.46013302+0.40732351j,  0.        +0.j        , -0.30528142-0.21246605j,  0.        -0.j        ,  0.25389832+0.25771317j,  0.        +0.j        ,  0.        +0.j        ,  0.        +0.j        ,  0.        +0.j        ,  0.        +0.j        ,  0.        +0.j        ,  0.        +0.j        ,  0.        +0.j        ,  0.        +0.j        
                ,  0.        -0.j        , -0.00563624-0.59423429j,  0.        +0.j        ,  0.46013302+0.40732351j,  0.        -0.j        , -0.30528142-0.21246605j,  0.        +0.j        ,  0.25389832+0.25771317j,  0.        +0.j        ,  0.        +0.j        ,  0.        +0.j        ,  0.        +0.j        ,  0.        +0.j        ,  0.        +0.j        ,  0.        +0.j        ,  0.        +0.j        
                ,  0.01676811-0.33500121j,  0.        +0.j        , -0.46028079-0.49188018j,  0.        -0.j        , -0.18833511+0.15673318j, -0.        +0.j        ,  0.14043556+0.59492096j,  0.        +0.j        ,  0.        +0.j        ,  0.        +0.j        ,  0.        +0.j        ,  0.        +0.j        , -0.        +0.j        , -0.        +0.j        ,  0.        +0.j        ,  0.        +0.j        
                ,  0.        +0.j        ,  0.01676811-0.33500121j,  0.        -0.j        , -0.46028079-0.49188018j, -0.        +0.j        , -0.18833511+0.15673318j,  0.        +0.j        ,  0.14043556+0.59492096j,  0.        +0.j        ,  0.        +0.j        ,  0.        +0.j        ,  0.        +0.j        , -0.        +0.j        , -0.        +0.j        ,  0.        +0.j        ,  0.        +0.j        
                ,  0.3733514 +0.14423137j,  0.        +0.j        , -0.18039476+0.08589294j, -0.        +0.j        , -0.80479128+0.20701926j, -0.        +0.j        ,  0.06883732-0.32342174j,  0.        +0.j        ,  0.        +0.j        ,  0.        +0.j        , -0.        +0.j        , -0.        +0.j        , -0.        +0.j        , -0.        +0.j        ,  0.        +0.j        ,  0.        +0.j        
                ,  0.        +0.j        ,  0.3733514 +0.14423137j, -0.        +0.j        , -0.18039476+0.08589294j, -0.        +0.j        , -0.80479128+0.20701926j,  0.        +0.j        ,  0.06883732-0.32342174j,  0.        +0.j        ,  0.        +0.j        , -0.        +0.j        , -0.        +0.j        , -0.        +0.j        , -0.        +0.j        ,  0.        +0.j        ,  0.        +0.j        
                , -0.        +0.j        , -0.        +0.j        , -0.        +0.j        , -0.        +0.j        , -0.        +0.j        , -0.        +0.j        , -0.        +0.j        , -0.        +0.j        , -0.59111943+0.15726005j, -0.        +0.j        , -0.15509793+0.32339668j, -0.        +0.j        , -0.33317562+0.00860528j, -0.        +0.j        , -0.5566068 +0.27625195j, -0.        +0.j        
                , -0.        +0.j        , -0.        +0.j        , -0.        +0.j        , -0.        +0.j        , -0.        +0.j        , -0.        +0.j        , -0.        +0.j        , -0.        +0.j        , -0.        +0.j        , -0.59111943+0.15726005j, -0.        +0.j        , -0.15509793+0.32339668j, -0.        +0.j        , -0.33317562+0.00860528j, -0.        +0.j        , -0.5566068 +0.27625195j
                ,  0.        +0.j        ,  0.        +0.j        ,  0.        +0.j        ,  0.        +0.j        ,  0.        +0.j        ,  0.        +0.j        ,  0.        +0.j        ,  0.        +0.j        , -0.00563624-0.59423429j,  0.        -0.j        ,  0.46013302+0.40732351j,  0.        +0.j        , -0.30528142-0.21246605j,  0.        -0.j        ,  0.25389832+0.25771317j,  0.        +0.j        
                ,  0.        +0.j        ,  0.        +0.j        ,  0.        +0.j        ,  0.        +0.j        ,  0.        +0.j        ,  0.        +0.j        ,  0.        +0.j        ,  0.        +0.j        ,  0.        -0.j        , -0.00563624-0.59423429j,  0.        +0.j        ,  0.46013302+0.40732351j,  0.        -0.j        , -0.30528142-0.21246605j,  0.        +0.j        ,  0.25389832+0.25771317j
                ,  0.        +0.j        ,  0.        +0.j        ,  0.        +0.j        ,  0.        +0.j        , -0.        +0.j        , -0.        +0.j        ,  0.        +0.j        ,  0.        +0.j        ,  0.01676811-0.33500121j,  0.        +0.j        , -0.46028079-0.49188018j,  0.        -0.j        , -0.18833511+0.15673318j, -0.        +0.j        ,  0.14043556+0.59492096j,  0.        +0.j        
                ,  0.        +0.j        ,  0.        +0.j        ,  0.        +0.j        ,  0.        +0.j        , -0.        +0.j        , -0.        +0.j        ,  0.        +0.j        ,  0.        +0.j        ,  0.        +0.j        ,  0.01676811-0.33500121j,  0.        -0.j        , -0.46028079-0.49188018j, -0.        +0.j        , -0.18833511+0.15673318j,  0.        +0.j        ,  0.14043556+0.59492096j
                ,  0.        +0.j        ,  0.        +0.j        , -0.        +0.j        , -0.        +0.j        , -0.        +0.j        , -0.        +0.j        ,  0.        +0.j        ,  0.        +0.j        ,  0.3733514 +0.14423137j,  0.        +0.j        , -0.18039476+0.08589294j, -0.        +0.j        , -0.80479128+0.20701926j, -0.        +0.j        ,  0.06883732-0.32342174j,  0.        +0.j        
                ,  0.        +0.j        ,  0.        +0.j        , -0.        +0.j        , -0.        +0.j        , -0.        +0.j        , -0.        +0.j        ,  0.        +0.j        ,  0.        +0.j        ,  0.        +0.j        ,  0.3733514 +0.14423137j, -0.        +0.j        , -0.18039476+0.08589294j, -0.        +0.j        , -0.80479128+0.20701926j,  0.        +0.j        ,  0.06883732-0.32342174j]


        u1 = ql.Unitary("testname",matrix)
        u1.decompose()
        k.hadamard(0)
        k.hadamard(1)
        k.hadamard(2)
        k.hadamard(3)
        k.cnot(0, 1)
        k.cnot(0, 2)
        k.cnot(0, 3)
        k.cnot(1, 2)
        k.cnot(1, 3)
        k.cnot(2, 3)

        k.gate(u1, [0, 1, 2, 3])

        p.add_kernel(k)
        p.compile()
        qx.set(os.path.join(output_dir, p.name+'.qasm'))
        qx.execute()
        c0 = qx.get_state()
        
        self.assertAlmostEqual(0.0625*helper_prob((matrix[0]  +  matrix[1] +  matrix[2] +  matrix[3] +  matrix[4] +  matrix[5] +  matrix[6] +  matrix[7] +  matrix[8]  +  matrix[9] +  matrix[10]+  matrix[11]+  matrix[12]+  matrix[13]+  matrix[14]+  matrix[15])), helper_regex(c0)[0], 5)
        self.assertAlmostEqual(0.0625*helper_prob((matrix[16] +  matrix[17]+  matrix[18]+  matrix[19]+  matrix[20]+  matrix[21]+  matrix[22]+  matrix[23]+  matrix[24] +  matrix[25]+  matrix[26]+  matrix[27]+  matrix[28]+  matrix[29]+  matrix[30]+  matrix[31])), helper_regex(c0)[1], 5)
        self.assertAlmostEqual(0.0625*helper_prob((matrix[32] +  matrix[33]+  matrix[34]+  matrix[35]+  matrix[36]+  matrix[37]+  matrix[38]+  matrix[39]+  matrix[40] +  matrix[41]+  matrix[42]+  matrix[43]+  matrix[44]+  matrix[45]+  matrix[46]+  matrix[47])), helper_regex(c0)[2], 5)
        self.assertAlmostEqual(0.0625*helper_prob((matrix[48] +  matrix[49]+  matrix[50]+  matrix[51]+  matrix[52]+  matrix[53]+  matrix[54]+  matrix[55]+  matrix[56] +  matrix[57]+  matrix[58]+  matrix[59]+  matrix[60]+  matrix[61]+  matrix[62]+  matrix[63])), helper_regex(c0)[3], 5)
        self.assertAlmostEqual(0.0625*helper_prob((matrix[64] +  matrix[65]+  matrix[66]+  matrix[67]+  matrix[68]+  matrix[69]+  matrix[70]+  matrix[71]+  matrix[72] +  matrix[73]+  matrix[74]+  matrix[75]+  matrix[76]+  matrix[77]+  matrix[78]+  matrix[79])), helper_regex(c0)[4], 5)
        self.assertAlmostEqual(0.0625*helper_prob((matrix[80] +  matrix[81]+  matrix[82]+  matrix[83]+  matrix[84]+  matrix[85]+  matrix[86]+  matrix[87]+  matrix[88] +  matrix[89]+  matrix[90]+  matrix[91]+  matrix[92]+  matrix[93]+  matrix[94]+  matrix[95])), helper_regex(c0)[5], 5)
        self.assertAlmostEqual(0.0625*helper_prob((matrix[96] +  matrix[97]+  matrix[98]+  matrix[99]+  matrix[100]+ matrix[101]+ matrix[102]+ matrix[103]+ matrix[104] + matrix[105]+ matrix[106]+ matrix[107]+ matrix[108]+ matrix[109]+ matrix[110]+ matrix[111])), helper_regex(c0)[6], 5)
        self.assertAlmostEqual(0.0625*helper_prob((matrix[112] + matrix[113]+ matrix[114]+ matrix[115]+ matrix[116]+ matrix[117]+ matrix[118]+ matrix[119]+ matrix[120] + matrix[121]+ matrix[122]+ matrix[123]+ matrix[124]+ matrix[125]+ matrix[126]+ matrix[127])), helper_regex(c0)[7], 5)
        self.assertAlmostEqual(0.0625*helper_prob((matrix[128] + matrix[129]+ matrix[130]+ matrix[131]+ matrix[132]+ matrix[133]+ matrix[134]+ matrix[135]+ matrix[136] + matrix[137]+ matrix[138]+ matrix[139]+ matrix[140]+ matrix[141]+ matrix[142]+ matrix[143])), helper_regex(c0)[8], 5)
        self.assertAlmostEqual(0.0625*helper_prob((matrix[144] + matrix[145]+ matrix[146]+ matrix[147]+ matrix[148]+ matrix[149]+ matrix[150]+ matrix[151]+ matrix[152] + matrix[153]+ matrix[154]+ matrix[155]+ matrix[156]+ matrix[157]+ matrix[158]+ matrix[159])), helper_regex(c0)[9], 5)
        self.assertAlmostEqual(0.0625*helper_prob((matrix[160] + matrix[161]+ matrix[162]+ matrix[163]+ matrix[164]+ matrix[165]+ matrix[166]+ matrix[167]+ matrix[168] + matrix[169]+ matrix[170]+ matrix[171]+ matrix[172]+ matrix[173]+ matrix[174]+ matrix[175])), helper_regex(c0)[10], 5)
        self.assertAlmostEqual(0.0625*helper_prob((matrix[176] + matrix[177]+ matrix[178]+ matrix[179]+ matrix[180]+ matrix[181]+ matrix[182]+ matrix[183]+ matrix[184] + matrix[185]+ matrix[186]+ matrix[187]+ matrix[188]+ matrix[189]+ matrix[190]+ matrix[191])), helper_regex(c0)[11], 5)
        self.assertAlmostEqual(0.0625*helper_prob((matrix[192] + matrix[193]+ matrix[194]+ matrix[195]+ matrix[196]+ matrix[197]+ matrix[198]+ matrix[199]+ matrix[200] + matrix[201]+ matrix[202]+ matrix[203]+ matrix[204]+ matrix[205]+ matrix[206]+ matrix[207])), helper_regex(c0)[12], 5)
        self.assertAlmostEqual(0.0625*helper_prob((matrix[208] + matrix[209]+ matrix[210]+ matrix[211]+ matrix[212]+ matrix[213]+ matrix[214]+ matrix[215]+ matrix[216] + matrix[217]+ matrix[218]+ matrix[219]+ matrix[220]+ matrix[221]+ matrix[222]+ matrix[223])), helper_regex(c0)[13], 5)
        self.assertAlmostEqual(0.0625*helper_prob((matrix[224] + matrix[225]+ matrix[226]+ matrix[227]+ matrix[228]+ matrix[229]+ matrix[230]+ matrix[231]+ matrix[232] + matrix[233]+ matrix[234]+ matrix[235]+ matrix[236]+ matrix[237]+ matrix[238]+ matrix[239])), helper_regex(c0)[14], 5)
        self.assertAlmostEqual(0.0625*helper_prob((matrix[240] + matrix[241]+ matrix[242]+ matrix[243]+ matrix[244]+ matrix[245]+ matrix[246]+ matrix[247]+ matrix[248] + matrix[249]+ matrix[250]+ matrix[251]+ matrix[252]+ matrix[253]+ matrix[254]+ matrix[255])), helper_regex(c0)[15], 5)
  
    def test_sparse2qubitunitary(self):
        num_qubits = 2
        p = ql.Program('test_usingqx_sparse2qubit', platform, num_qubits)
        k = ql.Kernel('akernel', platform, num_qubits)

        matrix = [ 0.2309453 -0.79746147j, -0.53683301+0.15009925j,  0.        +0.j        , -0.        +0.j        
        ,  0.39434916-0.39396473j,  0.80810853+0.19037107j,  0.        +0.j        ,  0.        +0.j        
        ,  0.        +0.j        , -0.        +0.j        ,  0.2309453 -0.79746147j, -0.53683301+0.15009925j
        ,  0.        +0.j        ,  0.        +0.j        ,  0.39434916-0.39396473j,  0.80810853+0.19037107j]

        u1 = ql.Unitary("testname",matrix)
        u1.decompose()
        k.gate(u1, [0, 1])


        p.add_kernel(k)
        p.compile()
        qx.set(os.path.join(output_dir, p.name+'.qasm'))
        qx.execute()
        c0 = qx.get_state()

        self.assertAlmostEqual(helper_prob(matrix[0]), helper_regex(c0)[0], 5)
        self.assertAlmostEqual(helper_prob(matrix[4]), helper_regex(c0)[1], 5)
        self.assertAlmostEqual(helper_prob(matrix[8]), 0, 5) # Zero probabilities do not show up in the output list
        self.assertAlmostEqual(helper_prob(matrix[12]), 0, 5)


  
    def test_sparse2qubitunitaryotherqubit(self):
        num_qubits = 2
        p = ql.Program('test_usingqx_sparse2qubitotherqubit', platform, num_qubits)
        k = ql.Kernel('akernel', platform, num_qubits)

        matrix = [ 0.30279949-0.60010283j,  0.        +0.j        , -0.58058628-0.45946559j,  0.        -0.j        
                ,  0.        +0.j        ,  0.30279949-0.60010283j,  0.        -0.j        , -0.58058628-0.45946559j
                ,  0.04481146-0.73904059j,  0.        +0.j        ,  0.64910478+0.17456782j,  0.        +0.j        
                ,  0.        +0.j        ,  0.04481146-0.73904059j,  0.        +0.j        ,  0.64910478+0.17456782j]
        
        u1 = ql.Unitary("sparse2_qubit_unitary_other_qubit",matrix)
        u1.decompose()
        k.gate(u1, [0, 1])


        p.add_kernel(k)
        p.compile()
        qx.set(os.path.join(output_dir, p.name+'.qasm'))
        qx.execute()
        c0 = qx.get_state()

        self.assertAlmostEqual(helper_prob(matrix[0]), helper_regex(c0)[0], 5)
        self.assertAlmostEqual(helper_prob(matrix[4]), 0, 5) # Zero probabilities do not show up in the output list
        self.assertAlmostEqual(helper_prob(matrix[8]), helper_regex(c0)[1], 5)
        self.assertAlmostEqual(helper_prob(matrix[12]), 0, 5)


    def test_sparse2qubitunitaryotherqubitcheck(self):
        num_qubits = 1
        p = ql.Program('test_usingqx_sparse2qubitotherqubit_test', platform, num_qubits)
        k = ql.Kernel('akernel', platform, num_qubits)

        matrix = [ 0.30279949-0.60010283j, -0.58058628-0.45946559j       
                ,  0.04481146-0.73904059j,    0.64910478+0.17456782j]
        u1 = ql.Unitary("testname",matrix)
        u1.decompose()
        k.gate(u1, [0])


        p.add_kernel(k)
        p.compile()
        qx.set(os.path.join(output_dir, p.name+'.qasm'))
        qx.execute()
        c0 = qx.get_state()

        self.assertAlmostEqual(helper_prob(matrix[0]), helper_regex(c0)[0], 5)
        self.assertAlmostEqual(helper_prob(matrix[1]), helper_regex(c0)[1], 5) # Zero probabilities do not show up in the output list


    def test_sparse2qubit_multiplexor(self):
        num_qubits = 2
        p = ql.Program('test_usingqx_sparse2qubit_multiplexor', platform, num_qubits)
        k = ql.Kernel('akernel', platform, num_qubits)

        matrix = [ 0.30279949-0.60010283j, -0.58058628-0.45946559j, 0                      , 0    
                ,  0.04481146-0.73904059j,  0.64910478+0.17456782j, 0                      , 0
                ,  0.        +0.j        , -0.        +0.j        ,  0.2309453 -0.79746147j, -0.53683301+0.15009925j
                ,  0.        +0.j        ,  0.        +0.j        ,  0.39434916-0.39396473j,  0.80810853+0.19037107j]
        u1 = ql.Unitary("multiplexor",matrix)
        u1.decompose()
        k.hadamard(0)
        k.hadamard(1)
        k.gate(u1, [0, 1])

        p.add_kernel(k)
        p.compile()
        qx.set(os.path.join(output_dir, p.name+'.qasm'))
        qx.execute()
        c0 = qx.get_state()


        self.assertAlmostEqual(0.25*helper_prob((matrix[0] + matrix[1] + matrix[2] + matrix[3] )), helper_regex(c0)[0], 5)
        self.assertAlmostEqual(0.25*helper_prob((matrix[4] + matrix[5] + matrix[6] + matrix[7] )), helper_regex(c0)[1], 5)
        self.assertAlmostEqual(0.25*helper_prob((matrix[8] + matrix[9] + matrix[10]+ matrix[11])), helper_regex(c0)[2], 5)
        self.assertAlmostEqual(0.25*helper_prob((matrix[12]+ matrix[13]+ matrix[14]+ matrix[15])), helper_regex(c0)[3], 5)

    def test_decomposition_rotatedtoffoli(self):
        num_qubits = 3
        p = ql.Program('test_usingqx_rotatedtoffoli', platform, num_qubits)
        k = ql.Kernel('akernel', platform, num_qubits)

        matrix = (np.exp(-1j*0.3*3.141562)*np.array([[1, 0, 0, 0, 0, 0, 0, 0],
                 [0, 1, 0, 0, 0, 0, 0, 0],
                 [0, 0, 1, 0, 0, 0, 0, 0],
                 [0, 0, 0, 1, 0, 0, 0, 0],
                 [0, 0, 0, 0, 1, 0, 0, 0],
                 [0, 0, 0, 0, 0, 1, 0, 0],
                 [0, 0, 0, 0, 0, 0, 0, 1],
                 [0, 0, 0, 0, 0, 0, 1, 0]])).flatten()

        u1 = ql.Unitary("rotatedtoffoli",matrix)
        u1.decompose()
        k.hadamard(0)
        k.hadamard(1)
        k.hadamard(2)

        k.gate(u1, [0, 1, 2])


        p.add_kernel(k)
        p.compile()
        qx.set(os.path.join(output_dir, p.name+'.qasm'))
        qx.execute()
        c0 = qx.get_state()

        self.assertAlmostEqual(0.125*helper_prob((matrix[0]  + matrix[1] + matrix[2] + matrix[3] + matrix[4] + matrix[5] + matrix[6] + matrix[7])), helper_regex(c0)[0], 5)
        self.assertAlmostEqual(0.125*helper_prob((matrix[8]  + matrix[9] + matrix[10]+ matrix[11]+ matrix[12]+ matrix[13]+ matrix[14]+ matrix[15])), helper_regex(c0)[1], 5)
        self.assertAlmostEqual(0.125*helper_prob((matrix[16] + matrix[17]+ matrix[18]+ matrix[19]+ matrix[20]+ matrix[21]+ matrix[22]+ matrix[23])), helper_regex(c0)[2], 5)
        self.assertAlmostEqual(0.125*helper_prob((matrix[24] + matrix[25]+ matrix[26]+ matrix[27]+ matrix[28]+ matrix[29]+ matrix[30]+ matrix[31])), helper_regex(c0)[3], 5)
        self.assertAlmostEqual(0.125*helper_prob((matrix[32] + matrix[33]+ matrix[34]+ matrix[35]+ matrix[36]+ matrix[37]+ matrix[38]+ matrix[39])), helper_regex(c0)[4], 5)
        self.assertAlmostEqual(0.125*helper_prob((matrix[40] + matrix[41]+ matrix[42]+ matrix[43]+ matrix[44]+ matrix[45]+ matrix[46]+ matrix[47])), helper_regex(c0)[5], 5)
        self.assertAlmostEqual(0.125*helper_prob((matrix[48] + matrix[49]+ matrix[50]+ matrix[51]+ matrix[52]+ matrix[53]+ matrix[54]+ matrix[55])), helper_regex(c0)[6], 5)
        self.assertAlmostEqual(0.125*helper_prob((matrix[56] + matrix[57]+ matrix[58]+ matrix[59]+ matrix[60]+ matrix[61]+ matrix[62]+ matrix[63])), helper_regex(c0)[7], 5)

    def test_decomposition_toffoli(self):
        num_qubits = 3
        p = ql.Program('test_usingqx_toffoli', platform, num_qubits)
        k = ql.Kernel('akernel', platform, num_qubits)

        matrix = np.array([[1, 0, 0, 0, 0, 0, 0, 0],
                 [0, 1, 0, 0, 0, 0, 0, 0],
                 [0, 0, 1, 0, 0, 0, 0, 0],
                 [0, 0, 0, 1, 0, 0, 0, 0],
                 [0, 0, 0, 0, 1, 0, 0, 0],
                 [0, 0, 0, 0, 0, 1, 0, 0],
                 [0, 0, 0, 0, 0, 0, 0, 1],
                 [0, 0, 0, 0, 0, 0, 1, 0]]).flatten()

        u1 = ql.Unitary("rotatedtoffoli",matrix)
        u1.decompose()
        k.hadamard(0)
        k.hadamard(1)
        k.hadamard(2)
        k.gate(u1, [0, 1, 2])

        p.add_kernel(k)
        p.compile()
        qx.set(os.path.join(output_dir, p.name+'.qasm'))
        qx.execute()
        c0 = qx.get_state()

        self.assertAlmostEqual(0.125*helper_prob((matrix[0]  + matrix[1] + matrix[2] + matrix[3] + matrix[4] + matrix[5] + matrix[6] + matrix[7])), helper_regex(c0)[0], 5)
        self.assertAlmostEqual(0.125*helper_prob((matrix[8]  + matrix[9] + matrix[10]+ matrix[11]+ matrix[12]+ matrix[13]+ matrix[14]+ matrix[15])), helper_regex(c0)[1], 5)
        self.assertAlmostEqual(0.125*helper_prob((matrix[16] + matrix[17]+ matrix[18]+ matrix[19]+ matrix[20]+ matrix[21]+ matrix[22]+ matrix[23])), helper_regex(c0)[2], 5)
        self.assertAlmostEqual(0.125*helper_prob((matrix[24] + matrix[25]+ matrix[26]+ matrix[27]+ matrix[28]+ matrix[29]+ matrix[30]+ matrix[31])), helper_regex(c0)[3], 5)
        self.assertAlmostEqual(0.125*helper_prob((matrix[32] + matrix[33]+ matrix[34]+ matrix[35]+ matrix[36]+ matrix[37]+ matrix[38]+ matrix[39])), helper_regex(c0)[4], 5)
        self.assertAlmostEqual(0.125*helper_prob((matrix[40] + matrix[41]+ matrix[42]+ matrix[43]+ matrix[44]+ matrix[45]+ matrix[46]+ matrix[47])), helper_regex(c0)[5], 5)
        self.assertAlmostEqual(0.125*helper_prob((matrix[48] + matrix[49]+ matrix[50]+ matrix[51]+ matrix[52]+ matrix[53]+ matrix[54]+ matrix[55])), helper_regex(c0)[6], 5)
        self.assertAlmostEqual(0.125*helper_prob((matrix[56] + matrix[57]+ matrix[58]+ matrix[59]+ matrix[60]+ matrix[61]+ matrix[62]+ matrix[63])), helper_regex(c0)[7], 5)


    def test_decomposition_controlled_U(self):
        num_qubits = 3
        p = ql.Program('test_usingqx_toffoli', platform, num_qubits)
        k = ql.Kernel('akernel', platform, num_qubits)

        matrix = np.array([[1, 0, 0, 0, 0, 0, 0, 0],
                 [0, 1, 0, 0, 0, 0, 0, 0],
                 [0, 0, 1, 0, 0, 0, 0, 0],
                 [0, 0, 0, 1, 0, 0, 0, 0],
                 [0, 0, 0, 0, 1, 0, 0, 0],
                 [0, 0, 0, 0, 0, 1, 0, 0],
                 [0, 0, 0, 0, 0, 0, 0.30279949-0.60010283j, -0.58058628-0.45946559j],
                 [0, 0, 0, 0, 0 ,0, 0.04481146-0.73904059j,  0.64910478+0.17456782j]]).flatten()

        u1 = ql.Unitary("arbitrarycontrolled",matrix)
        u1.decompose()
        k.hadamard(0)
        k.hadamard(1)
        k.hadamard(2)
        k.gate(u1, [0, 1, 2])


        p.add_kernel(k)
        p.compile()
        qx.set(os.path.join(output_dir, p.name+'.qasm'))
        qx.execute()
        c0 = qx.get_state()

        self.assertAlmostEqual(0.125*helper_prob((matrix[0]  + matrix[1] + matrix[2] + matrix[3] + matrix[4] + matrix[5] + matrix[6] + matrix[7])), helper_regex(c0)[0], 5)
        self.assertAlmostEqual(0.125*helper_prob((matrix[8]  + matrix[9] + matrix[10]+ matrix[11]+ matrix[12]+ matrix[13]+ matrix[14]+ matrix[15])), helper_regex(c0)[1], 5)
        self.assertAlmostEqual(0.125*helper_prob((matrix[16] + matrix[17]+ matrix[18]+ matrix[19]+ matrix[20]+ matrix[21]+ matrix[22]+ matrix[23])), helper_regex(c0)[2], 5)
        self.assertAlmostEqual(0.125*helper_prob((matrix[24] + matrix[25]+ matrix[26]+ matrix[27]+ matrix[28]+ matrix[29]+ matrix[30]+ matrix[31])), helper_regex(c0)[3], 5)
        self.assertAlmostEqual(0.125*helper_prob((matrix[32] + matrix[33]+ matrix[34]+ matrix[35]+ matrix[36]+ matrix[37]+ matrix[38]+ matrix[39])), helper_regex(c0)[4], 5)
        self.assertAlmostEqual(0.125*helper_prob((matrix[40] + matrix[41]+ matrix[42]+ matrix[43]+ matrix[44]+ matrix[45]+ matrix[46]+ matrix[47])), helper_regex(c0)[5], 5)
        self.assertAlmostEqual(0.125*helper_prob((matrix[48] + matrix[49]+ matrix[50]+ matrix[51]+ matrix[52]+ matrix[53]+ matrix[54]+ matrix[55])), helper_regex(c0)[6], 5)
        self.assertAlmostEqual(0.125*helper_prob((matrix[56] + matrix[57]+ matrix[58]+ matrix[59]+ matrix[60]+ matrix[61]+ matrix[62]+ matrix[63])), helper_regex(c0)[7], 5)
   
    def test_decomposition_mscthesisaritra(self):
        num_qubits = 4
        p = ql.Program('test_usingqx_mscthesisaritra', platform, num_qubits)
        k = ql.Kernel('akernel', platform, num_qubits)

        matrix = [0.3672   ,-0.3654   ,-0.3654   ,-0.2109   ,-0.3654   ,-0.2109   ,-0.2109   ,-0.1218   ,-0.3654   ,-0.2109   ,-0.2109   ,-0.1218   ,-0.2109   ,-0.1218   ,-0.1218   ,-0.0703
            ,-0.3654   , 0.7891   ,-0.2109   ,-0.1218   ,-0.2109   ,-0.1218   ,-0.1218   ,-0.0703   ,-0.2109   ,-0.1218   ,-0.1218   ,-0.0703   ,-0.1218   ,-0.0703   ,-0.0703   ,-0.0406
            ,-0.3654   ,-0.2109   , 0.7891   ,-0.1218   ,-0.2109   ,-0.1218   ,-0.1218   ,-0.0703   ,-0.2109   ,-0.1218   ,-0.1218   ,-0.0703   ,-0.1218   ,-0.0703   ,-0.0703   ,-0.0406
            ,-0.2109   ,-0.1218   ,-0.1218   , 0.9297   ,-0.1218   ,-0.0703   ,-0.0703   ,-0.0406   ,-0.1218   ,-0.0703   ,-0.0703   ,-0.0406   ,-0.0703   ,-0.0406   ,-0.0406   ,-0.0234
            ,-0.3654   ,-0.2109   ,-0.2109   ,-0.1218   , 0.7891   ,-0.1218   ,-0.1218   ,-0.0703   ,-0.2109   ,-0.1218   ,-0.1218   ,-0.0703   ,-0.1218   ,-0.0703   ,-0.0703   ,-0.0406
            ,-0.2109   ,-0.1218   ,-0.1218   ,-0.0703   ,-0.1218   , 0.9297   ,-0.0703   ,-0.0406   ,-0.1218   ,-0.0703   ,-0.0703   ,-0.0406   ,-0.0703   ,-0.0406   ,-0.0406   ,-0.0234
            ,-0.2109   ,-0.1218   ,-0.1218   ,-0.0703   ,-0.1218   ,-0.0703   , 0.9297   ,-0.0406   ,-0.1218   ,-0.0703   ,-0.0703   ,-0.0406   ,-0.0703   ,-0.0406   ,-0.0406   ,-0.0234
            ,-0.1218   ,-0.0703   ,-0.0703   ,-0.0406   ,-0.0703   ,-0.0406   ,-0.0406   , 0.9766   ,-0.0703   ,-0.0406   ,-0.0406   ,-0.0234   ,-0.0406   ,-0.0234   ,-0.0234   ,-0.0135
            ,-0.3654   ,-0.2109   ,-0.2109   ,-0.1218   ,-0.2109   ,-0.1218   ,-0.1218   ,-0.0703   , 0.7891   ,-0.1218   ,-0.1218   ,-0.0703   ,-0.1218   ,-0.0703   ,-0.0703   ,-0.0406
            ,-0.2109   ,-0.1218   ,-0.1218   ,-0.0703   ,-0.1218   ,-0.0703   ,-0.0703   ,-0.0406   ,-0.1218   , 0.9297   ,-0.0703   ,-0.0406   ,-0.0703   ,-0.0406   ,-0.0406   ,-0.0234
            ,-0.2109   ,-0.1218   ,-0.1218   ,-0.0703   ,-0.1218   ,-0.0703   ,-0.0703   ,-0.0406   ,-0.1218   ,-0.0703   , 0.9297   ,-0.0406   ,-0.0703   ,-0.0406   ,-0.0406   ,-0.0234
            ,-0.1218   ,-0.0703   ,-0.0703   ,-0.0406   ,-0.0703   ,-0.0406   ,-0.0406   ,-0.0234   ,-0.0703   ,-0.0406   ,-0.0406   , 0.9766   ,-0.0406   ,-0.0234   ,-0.0234   ,-0.0135
            ,-0.2109   ,-0.1218   ,-0.1218   ,-0.0703   ,-0.1218   ,-0.0703   ,-0.0703   ,-0.0406   ,-0.1218   ,-0.0703   ,-0.0703   ,-0.0406   , 0.9297   ,-0.0406   ,-0.0406   ,-0.0234
            ,-0.1218   ,-0.0703   ,-0.0703   ,-0.0406   ,-0.0703   ,-0.0406   ,-0.0406   ,-0.0234   ,-0.0703   ,-0.0406   ,-0.0406   ,-0.0234   ,-0.0406   , 0.9766   ,-0.0234   ,-0.0135
            ,-0.1218   ,-0.0703   ,-0.0703   ,-0.0406   ,-0.0703   ,-0.0406   ,-0.0406   ,-0.0234   ,-0.0703   ,-0.0406   ,-0.0406   ,-0.0234   ,-0.0406   ,-0.0234   , 0.9766   ,-0.0135
            ,-0.0703   ,-0.0406   ,-0.0406   ,-0.0234   ,-0.0406   ,-0.0234   ,-0.0234   ,-0.0135   ,-0.0406   ,-0.0234   ,-0.0234   ,-0.0135   ,-0.0234   ,-0.0135   ,-0.0135   , 0.9922]

        u1 = ql.Unitary("mscthesisaritra",matrix)
        u1.decompose()
        k.hadamard(0)
        k.hadamard(1)
        k.hadamard(2)
        k.hadamard(3)
        k.gate(u1, [0, 1, 2, 3])


        p.add_kernel(k)
        p.compile()
        qx.set(os.path.join(output_dir, p.name+'.qasm'))
        qx.execute()
        c0 = qx.get_state()

        # less accuracy because of less accurate input
        self.assertAlmostEqual(0.0625*helper_prob((matrix[0]  +  matrix[1] +  matrix[2] +  matrix[3] +  matrix[4] +  matrix[5] +  matrix[6] +  matrix[7] +  matrix[8]  +  matrix[9] +  matrix[10]+  matrix[11]+  matrix[12]+  matrix[13]+  matrix[14]+  matrix[15])), helper_regex(c0)[0], 2)
        self.assertAlmostEqual(0.0625*helper_prob((matrix[16] +  matrix[17]+  matrix[18]+  matrix[19]+  matrix[20]+  matrix[21]+  matrix[22]+  matrix[23]+  matrix[24] +  matrix[25]+  matrix[26]+  matrix[27]+  matrix[28]+  matrix[29]+  matrix[30]+  matrix[31])), helper_regex(c0)[1], 2)
        self.assertAlmostEqual(0.0625*helper_prob((matrix[32] +  matrix[33]+  matrix[34]+  matrix[35]+  matrix[36]+  matrix[37]+  matrix[38]+  matrix[39]+  matrix[40] +  matrix[41]+  matrix[42]+  matrix[43]+  matrix[44]+  matrix[45]+  matrix[46]+  matrix[47])), helper_regex(c0)[2], 2)
        self.assertAlmostEqual(0.0625*helper_prob((matrix[48] +  matrix[49]+  matrix[50]+  matrix[51]+  matrix[52]+  matrix[53]+  matrix[54]+  matrix[55]+  matrix[56] +  matrix[57]+  matrix[58]+  matrix[59]+  matrix[60]+  matrix[61]+  matrix[62]+  matrix[63])), helper_regex(c0)[3], 2)
        self.assertAlmostEqual(0.0625*helper_prob((matrix[64] +  matrix[65]+  matrix[66]+  matrix[67]+  matrix[68]+  matrix[69]+  matrix[70]+  matrix[71]+  matrix[72] +  matrix[73]+  matrix[74]+  matrix[75]+  matrix[76]+  matrix[77]+  matrix[78]+  matrix[79])), helper_regex(c0)[4], 2)
        self.assertAlmostEqual(0.0625*helper_prob((matrix[80] +  matrix[81]+  matrix[82]+  matrix[83]+  matrix[84]+  matrix[85]+  matrix[86]+  matrix[87]+  matrix[88] +  matrix[89]+  matrix[90]+  matrix[91]+  matrix[92]+  matrix[93]+  matrix[94]+  matrix[95])), helper_regex(c0)[5], 2)
        self.assertAlmostEqual(0.0625*helper_prob((matrix[96] +  matrix[97]+  matrix[98]+  matrix[99]+  matrix[100]+ matrix[101]+ matrix[102]+ matrix[103]+ matrix[104] + matrix[105]+ matrix[106]+ matrix[107]+ matrix[108]+ matrix[109]+ matrix[110]+ matrix[111])), helper_regex(c0)[6], 2)
        self.assertAlmostEqual(0.0625*helper_prob((matrix[112] + matrix[113]+ matrix[114]+ matrix[115]+ matrix[116]+ matrix[117]+ matrix[118]+ matrix[119]+ matrix[120] + matrix[121]+ matrix[122]+ matrix[123]+ matrix[124]+ matrix[125]+ matrix[126]+ matrix[127])), helper_regex(c0)[7], 2)
        self.assertAlmostEqual(0.0625*helper_prob((matrix[128] + matrix[129]+ matrix[130]+ matrix[131]+ matrix[132]+ matrix[133]+ matrix[134]+ matrix[135]+ matrix[136] + matrix[137]+ matrix[138]+ matrix[139]+ matrix[140]+ matrix[141]+ matrix[142]+ matrix[143])), helper_regex(c0)[8], 2)
        self.assertAlmostEqual(0.0625*helper_prob((matrix[144] + matrix[145]+ matrix[146]+ matrix[147]+ matrix[148]+ matrix[149]+ matrix[150]+ matrix[151]+ matrix[152] + matrix[153]+ matrix[154]+ matrix[155]+ matrix[156]+ matrix[157]+ matrix[158]+ matrix[159])), helper_regex(c0)[9], 2)
        self.assertAlmostEqual(0.0625*helper_prob((matrix[160] + matrix[161]+ matrix[162]+ matrix[163]+ matrix[164]+ matrix[165]+ matrix[166]+ matrix[167]+ matrix[168] + matrix[169]+ matrix[170]+ matrix[171]+ matrix[172]+ matrix[173]+ matrix[174]+ matrix[175])), helper_regex(c0)[10], 2)
        self.assertAlmostEqual(0.0625*helper_prob((matrix[176] + matrix[177]+ matrix[178]+ matrix[179]+ matrix[180]+ matrix[181]+ matrix[182]+ matrix[183]+ matrix[184] + matrix[185]+ matrix[186]+ matrix[187]+ matrix[188]+ matrix[189]+ matrix[190]+ matrix[191])), helper_regex(c0)[11], 2)
        self.assertAlmostEqual(0.0625*helper_prob((matrix[192] + matrix[193]+ matrix[194]+ matrix[195]+ matrix[196]+ matrix[197]+ matrix[198]+ matrix[199]+ matrix[200] + matrix[201]+ matrix[202]+ matrix[203]+ matrix[204]+ matrix[205]+ matrix[206]+ matrix[207])), helper_regex(c0)[12], 2)
        self.assertAlmostEqual(0.0625*helper_prob((matrix[208] + matrix[209]+ matrix[210]+ matrix[211]+ matrix[212]+ matrix[213]+ matrix[214]+ matrix[215]+ matrix[216] + matrix[217]+ matrix[218]+ matrix[219]+ matrix[220]+ matrix[221]+ matrix[222]+ matrix[223])), helper_regex(c0)[13], 2)
        self.assertAlmostEqual(0.0625*helper_prob((matrix[224] + matrix[225]+ matrix[226]+ matrix[227]+ matrix[228]+ matrix[229]+ matrix[230]+ matrix[231]+ matrix[232] + matrix[233]+ matrix[234]+ matrix[235]+ matrix[236]+ matrix[237]+ matrix[238]+ matrix[239])), helper_regex(c0)[14], 2)
        self.assertAlmostEqual(0.0625*helper_prob((matrix[240] + matrix[241]+ matrix[242]+ matrix[243]+ matrix[244]+ matrix[245]+ matrix[246]+ matrix[247]+ matrix[248] + matrix[249]+ matrix[250]+ matrix[251]+ matrix[252]+ matrix[253]+ matrix[254]+ matrix[255])), helper_regex(c0)[15], 2)
  
if __name__ == '__main__':
    unittest.main()


