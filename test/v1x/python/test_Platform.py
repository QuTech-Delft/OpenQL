import openql as ql
import os
import unittest

from config import json_dir, output_dir, qasm_golden_dir
from utils import file_compare


class TestPlatform(unittest.TestCase):
    @classmethod
    def setUp(cls):
        ql.initialize()
        ql.set_option('output_dir', output_dir)
        ql.set_option('optimize', 'no')
        ql.set_option('scheduler', 'ALAP')
        ql.set_option('log_level', 'LOG_WARNING')

    def test_platform_name(self):
        platf_name = 'starmon_platform'
        platform = ql.Platform(platf_name, 'none')
        self.assertEqual(platform.name, platf_name)

    def test_platform_architectures(self):
        ql.Platform('x', 'cc_light')
        ql.Platform('x', 'cc_light.s5')
        ql.Platform('x', 'cc_light.s7')
        ql.Platform('x', 'cc_light.s17')
        ql.Platform('x', 'cc')

    def test_platform_simple_constructor(self):
        platform = ql.Platform('cc_light')
        self.assertEqual(platform.name, 'cc_light')
        self.assertEqual(platform.config_file, 'cc_light')
        platform = ql.Platform()
        self.assertEqual(platform.name, 'none')
        self.assertEqual(platform.config_file, 'none')

    def test_platform_from_config(self):
        platform_name = 'starmon_platform'
        config_fn = os.path.join(json_dir, 'test_cfg_none.json')
        platform = ql.Platform(platform_name, config_fn)
        self.assertEqual(platform.config_file, config_fn)

    def test_platform_modified_in_place(self):
        name = 'test_platform_modified_in_place'
        data = ql.Platform.get_platform_json('cc_light')
        data['instructions']['my_custom_instruction'] = {'duration': 20}
        platform = ql.Platform.from_json(name, data)
        k = ql.Kernel('test', platform)
        k.gate('my_custom_instruction', 0)
        p = ql.Program(name, platform)
        p.add_kernel(k)
        p.compile()
        self.assertTrue(file_compare(
            os.path.join(output_dir, name + '_last.qasm'),
            os.path.join(qasm_golden_dir, name + '_last.qasm')
        ))


if __name__ == '__main__':
    unittest.main()
