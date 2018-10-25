import os
import unittest
from openql import openql as ql

curdir = os.path.dirname(__file__)
output_dir = os.path.join(curdir, 'test_output')

class Test_options(unittest.TestCase):

    def test_ok(self):
        ql.set_option('output_dir', output_dir)
        ql.set_option('optimize', 'no')
        ql.set_option('scheduler', 'ASAP')
        ql.set_option('log_level', 'LOG_WARNING')

    @unittest.skip
    def test_nok(self):
        # with self.assertRaises(RuntimeError) as cm:
        try:
            ql.set_option('optimize', 'Nope')
            raise
        except:
            pass

    def test_default_scheduler(self):
        # tests if 'ALAP' is indeed the default scheduler policy
        self.assertEqual('ALAP', ql.get_option('scheduler'),
            'ALAP is not the default scheduler!')

if __name__ == '__main__':
    unittest.main()
