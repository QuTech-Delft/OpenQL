import os
import sys
import unittest
from openql import openql as ql



curdir = os.path.dirname(__file__)

output_dir = os.path.join(curdir, 'test_output')
ql.set_output_dir(output_dir)

sys.path.append(os.path.join(curdir, 'qisa-as', 'build'))
print(sys.path)


class Test_QISA_asssembler(unittest.TestCase):
    def test_QISA_assembler_present(self):
        # from pyQisaAs import QISA_Driver
        from pyQisaAs import QISA_Driver
