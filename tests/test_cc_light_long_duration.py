import numpy as np
import os
import unittest
from openql import openql as ql
from utils import file_compare

curdir = os.path.dirname(os.path.realpath(__file__))
output_dir = os.path.join(curdir, 'test_output')

class Test_CCL_long_duration(unittest.TestCase):

    @classmethod
    def setUp(self):
        ql.initialize()

    def test_AllXY(self):
        """
        Single qubit AllXY sequence.
        Writes output files to the directory specified in openql.
        Output directory is set as an attribute to the program for convenience.

        Input pars:
            qubit_idx:      int specifying the target qubit (starting at 0)
            platf_cfg:      filename of the platform config file
            double_points:  if true repeats every element twice
        Returns:
            p:              OpenQL Program object containing


        """
        ql.set_option('output_dir', output_dir)
        config_fn = os.path.join(curdir, 'test_cfg_CCL_long_duration.json')
        platf  = ql.Platform('seven_qubits_chip', config_fn)
        p = ql.Program("AllXYLongDuration", platf, platf.get_qubit_number())

        allXY = [ ['i', 'i'], ['rx180', 'ry180'], ['ry180', 'rx180'] ]
                # ,
                #  ['rx180', 'ry180'], ['ry180', 'rx180'],
                #  ['rx90', 'i'], ['ry90', 'i'], ['rx90', 'ry90'],
                #  ['ry90', 'rx90'], ['rx90', 'ry180'], ['ry90', 'rx180'],
                #  ['rx180', 'ry90'], ['ry180', 'rx90'], ['rx90', 'rx180'],
                #  ['rx180', 'rx90'], ['ry90', 'ry180'], ['ry180', 'ry90'],
                #  ['rx180', 'i'], ['ry180', 'i'], ['rx90', 'rx90'],
                #  ['ry90', 'ry90']]

        # this should be implicit
        p.set_sweep_points(np.arange(len(allXY), dtype=float))
        qubit_idx=0
        for i, xy in enumerate(allXY):
            k = ql.Kernel("AllXY_"+str(i), platf, platf.get_qubit_number())
            k.prepz(qubit_idx)
            k.gate(xy[0], [qubit_idx])
            k.gate(xy[1], [qubit_idx])
            k.measure(qubit_idx)
            p.add_kernel(k)

        p.compile()

        GOLD_fn = os.path.join(curdir, 'golden', p.name + '.qisa')
        QISA_fn = os.path.join(output_dir, p.name+'.qisa')

        self.assertTrue(file_compare(QISA_fn, GOLD_fn))


if __name__ == '__main__':
    unittest.main()
