# File:         test_rus_private.py
# Purpose:      test RUS using cQASM1.2
# Based on:     test_structure_decomposition.py

import os
import unittest
#from utils import file_compare

import openql as ql


curdir = os.path.dirname(os.path.realpath(__file__))
output_dir = os.path.join(curdir, 'test_output')


class Test_cQASM(unittest.TestCase):

    def run_test_case(self, name):
        old_wd = os.getcwd()
        try:
            os.chdir(curdir)

            in_fn = 'test_' + name + '.cq'
            out_fn = 'test_output/' + name + '_out.cq'
            gold_fn = 'golden/' + name + '_out.cq'

            # ql.set_option('log_level', 'LOG_INFO')
            # ql.set_option('log_level', 'LOG_DEBUG')
            ql.compile(in_fn)

#            self.assertTrue(file_compare(out_fn, gold_fn))

        finally:
            os.chdir(old_wd)

    def test_rus_private(self):
        self.run_test_case('rus_private')


if __name__ == '__main__':
    unittest.main()
