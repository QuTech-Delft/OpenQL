import os
import unittest
from openql import openql as ql
import numpy as np

rootDir = os.path.dirname(os.path.realpath(__file__))
curdir = os.path.dirname(__file__)
output_dir = os.path.join(curdir, 'test_output')


'''
  grover algorithm implementation
'''

def init(platform):
        init_k   = ql.Kernel('init', platform)
        
        # oracle qubit
        oracle = 4

        # init
        init_k.x(oracle)
        init_k.hadamard(oracle)

        # Hn search space 
        for i in range(4):
           init_k.hadamard(i) 

        return init_k


def grover(platform):
        grover_k = ql.Kernel('grover', platform)
        # oracle qubit
        oracle = 4
        # name search space
        s0 = 0 
        s1 = 1
        s2 = 2
        s3 = 3
 
        # oracle
        grover_k.x(s0)
        grover_k.x(s1)
        grover_k.toffoli(s0,s1, 5)
        grover_k.toffoli(s1, 5, 6)
        grover_k.toffoli(s2, 6, 7)
        grover_k.toffoli(s3, 7, 8)
        grover_k.cnot(8, oracle)
        grover_k.toffoli(s3,7,8)
        grover_k.toffoli(s2,6,7)
        grover_k.toffoli(s1,5,6)
        grover_k.toffoli(s0,1,5)

        # restore ss
        grover_k.x(s0)
        grover_k.x(s1)
        
        # diffusion 
        for i in range(4):
           grover_k.hadamard(i) 
        
        for i in range(4):
           grover_k.x(i) 
        
        # controlled-phase shift
        grover_k.hadamard(s3)
        grover_k.toffoli(s0,s1,5)
        grover_k.toffoli(s1,5,6)
        grover_k.toffoli(s2,6,7)
        grover_k.cnot(7,s3)        
        grover_k.toffoli(s2,6,7)
        grover_k.toffoli(s1,5,6)
        grover_k.toffoli(s0,s1,5)
        grover_k.hadamard(s3)
        
        # restore search space
        for i in range(4):
           grover_k.x(i) 
        for i in range(4):
           grover_k.hadamard(i) 

        grover_k.gate('display',[])

        return grover_k
        
        

def grover_algorithm():
        ql.set_option('output_dir', output_dir)
        ql.set_option('optimize', 'no')
        ql.set_option('scheduler', 'ASAP')
        ql.set_option('log_level', 'LOG_CRITICAL')

        config_fn = os.path.join(curdir, 'hardware_config_qx.json')
        platform  = ql.Platform('platform_none', config_fn)
        num_qubits = 9
        # oracle qubit
        oracle = 4
        
        # create a grover program
        p = ql.Program('test_grover', num_qubits, platform)
        
        # kernels
        init_k   = init(platform)
        grover_k = grover(platform)
        result_k = ql.Kernel('result', platform)
       
        # result
        result_k.hadamard(oracle)
        result_k.measure(oracle)
        result_k.gate("display",[])

        result_k.measure(0)
        result_k.measure(1)
        result_k.measure(2)
        result_k.measure(3)
        result_k.gate("display_binary",[])

        # build the program 
        p.add_kernel(init_k)
        p.add_kernel(grover_k,3)
        p.add_kernel(result_k)
        ql.set_option('decompose_toffoli', 'NC')
        p.compile()

if __name__ == '__main__':
    grover_algorithm()
