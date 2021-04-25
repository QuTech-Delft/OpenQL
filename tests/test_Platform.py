import os
import unittest
from openql import openql as ql

curdir = os.path.dirname(os.path.realpath(__file__))
output_dir = os.path.join(curdir, 'test_output')

class Test_platform(unittest.TestCase):

    @classmethod
    def setUp(self):
        ql.initialize()
        ql.set_option('output_dir', output_dir)
        ql.set_option('optimize', 'no')
        ql.set_option('scheduler', 'ALAP')
        ql.set_option('log_level', 'LOG_WARNING')

    def test_platform_name(self):
        platf_name = 'starmon_platform'
        platf = ql.Platform(platf_name, 'none')
        self.assertEqual(platf.name, platf_name)

    def test_platform_architectures(self):
        ql.Platform('x', 'cc_light')
        ql.Platform('x', 'cc_light.s5')
        ql.Platform('x', 'cc_light.s7')
        ql.Platform('x', 'cc_light.s17')
        ql.Platform('x', 'cc')

    def test_platform_from_config(self):
        platf_name = 'starmon_platform'
        config_fn = os.path.join(curdir, 'test_cfg_none.json')
        platf = ql.Platform(platf_name, config_fn)
        self.assertEqual(platf.config_file, config_fn)

if __name__ == '__main__':
    unittest.main()
