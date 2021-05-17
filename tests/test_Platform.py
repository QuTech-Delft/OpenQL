import os
import unittest
from openql import openql as ql
from utils import file_compare

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

    def test_platform_simple_constructor(self):
        platf = ql.Platform('cc_light')
        self.assertEqual(platf.name, 'cc_light')
        self.assertEqual(platf.config_file, 'cc_light')
        platf = ql.Platform()
        self.assertEqual(platf.name, 'none')
        self.assertEqual(platf.config_file, 'none')

    def test_platform_from_config(self):
        platf_name = 'starmon_platform'
        config_fn = os.path.join(curdir, 'test_cfg_none.json')
        platf = ql.Platform(platf_name, config_fn)
        self.assertEqual(platf.config_file, config_fn)

    def test_platform_modified_in_place(self):
        name = 'test_platform_modified_in_place'
        data = ql.Platform.get_platform_json('cc_light')
        data['instructions']['my_custom_instruction'] = {'duration': 20}
        platf = ql.Platform.from_json(name, data)
        k = ql.Kernel('test', platf)
        k.gate('my_custom_instruction', 0)
        p = ql.Program(name, platf)
        p.add_kernel(k)
        p.compile()
        self.assertTrue(file_compare(
            os.path.join(output_dir, name + '_last.qasm'),
            os.path.join(curdir, 'golden', name + '_last.qasm')
        ))

if __name__ == '__main__':
    unittest.main()
