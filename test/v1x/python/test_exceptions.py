import openql as ql
import os
import unittest

from config import json_dir


class TestBasic(unittest.TestCase):
    @classmethod
    def setUp(cls):
        ql.initialize()

    # this test should raise exception as specified configuration
    # file does not exist
    def test_missing_config(self):
        config_fn = os.path.join(json_dir, 'test_cfg_cbox_not_available.json')
        try:
            ql.Platform("starmon", config_fn)
            raise
        except:
            pass


if __name__ == '__main__':
    unittest.main()
