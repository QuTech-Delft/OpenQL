from openql import openql as ql
import unittest
import os

curdir = os.path.dirname(__file__)

class Test_basic(unittest.TestCase):

    # this test should raise exception as specified configuration
    # file does not exist
    def test_missing_config(self):
        config_fn = os.path.join(curdir, 'test_cfg_cbox_not_available.json')
        try:
            platf = ql.Platform("starmon", config_fn)
            raise
        except:
            pass

    # this test should fail as the eqasm compiler is not specified
    # in the configuration being loaded
    def test_config_exception_compiler(self):
        output_dir = os.path.join(curdir, 'test_output')
        ql.set_output_dir(output_dir)
        config_fn = os.path.join(curdir, 'test_cfg_cbox_broken01.json')
        try:
            platf = ql.Platform("starmon", config_fn)
            raise
        except:
            pass

    # this test should fail as the hardware section is not there
    # in the configuration being loaded
    def test_config_exception_hardware(self):
        output_dir = os.path.join(curdir, 'test_output')
        ql.set_output_dir(output_dir)
        config_fn = os.path.join(curdir, 'test_cfg_cbox_broken02.json')
        try:
            platf = ql.Platform("starmon", config_fn)
            raise
        except:
            pass


if __name__ == '__main__':
    unittest.main()
