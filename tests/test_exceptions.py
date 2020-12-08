from openql import openql as ql
import unittest
import os

curdir = os.path.dirname(os.path.realpath(__file__))

class Test_basic(unittest.TestCase):

    @classmethod
    def setUp(self):
        ql.initialize()

    # this test should raise exception as specified configuration
    # file does not exist
    def test_missing_config(self):
        config_fn = os.path.join(curdir, 'test_cfg_cbox_not_available.json')
        try:
            platf = ql.Platform("starmon", config_fn)
            raise
        except:
            pass

if __name__ == '__main__':
    unittest.main()
