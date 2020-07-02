import os
import unittest
from openql import openql as ql

curdir = os.path.dirname(__file__)
output_dir = os.path.join(curdir, 'test_output')

class Test_options(unittest.TestCase):

    def tearDown(self):
        ql.set_option('log_level', 'LOG_NOTHING')
        ql.set_option('optimize', 'no')
        ql.set_option('scheduler', 'ALAP')
        ql.set_option('scheduler_uniform', 'no')
        ql.set_option('use_default_gates', 'yes')
        ql.set_option('decompose_toffoli', 'no')


    def test_set_all_options(self):
        # try to set all legal values of options
        ql.set_option('log_level', 'LOG_NOTHING')
        ql.set_option('log_level', 'LOG_CRITICAL')
        ql.set_option('log_level', 'LOG_ERROR')
        ql.set_option('log_level', 'LOG_WARNING')
        ql.set_option('log_level', 'LOG_INFO')
        ql.set_option('log_level', 'LOG_DEBUG')

        ql.set_option('output_dir', output_dir)

        ql.set_option('optimize', 'yes')
        ql.set_option('optimize', 'no')

        ql.set_option('scheduler', 'ALAP')
        ql.set_option('scheduler', 'ASAP')

        ql.set_option('scheduler_uniform', 'yes')
        ql.set_option('scheduler_uniform', 'no')

        ql.set_option('use_default_gates', 'yes')
        ql.set_option('use_default_gates', 'no')

        ql.set_option('decompose_toffoli', 'no')
        ql.set_option('decompose_toffoli', 'NC')
        ql.set_option('decompose_toffoli', 'AM')


    def test_nok(self):
        # supress error printing first as the following will print errors
        ql.set_option('log_level', 'LOG_NOTHING')

        # illegal values for options should raise errors
        with self.assertRaises(Exception) as cm:
            ql.set_option('optimize', 'nope')

        self.assertEqual(str(cm.exception), 'Error parsing options. The value nope is not an allowed value for --optimize !')


        with self.assertRaises(Exception) as cm:
            ql.set_option('scheduler', 'best')

        self.assertEqual(str(cm.exception), 'Error parsing options. The value best is not an allowed value for --scheduler !')


    def test_get_values(self):
        # try to set a legal value and then test if it is indeed set
        ql.set_option('log_level', 'LOG_INFO')
        self.assertEqual(ql.get_option('log_level'), 'LOG_INFO')

        ql.set_option('output_dir', output_dir)
        self.assertEqual(ql.get_option('output_dir'), output_dir)

        ql.set_option('optimize', 'yes')
        self.assertEqual(ql.get_option('optimize'), 'yes')


        ql.set_option('scheduler', 'ALAP')
        self.assertEqual(ql.get_option('scheduler'), 'ALAP')

        ql.set_option('scheduler_uniform', 'yes')
        self.assertEqual(ql.get_option('scheduler_uniform'), 'yes')


        ql.set_option('use_default_gates', 'yes')
        self.assertEqual(ql.get_option('use_default_gates'), 'yes')

        ql.set_option('decompose_toffoli', 'NC')
        self.assertEqual(ql.get_option('decompose_toffoli'), 'NC')


    def test_default_scheduler(self):
        self.tearDown()
        # tests if 'ALAP' is indeed the default scheduler policy
        self.assertEqual('ALAP', ql.get_option('scheduler'),
            'ALAP is not the default scheduler!')

if __name__ == '__main__':
    unittest.main()
