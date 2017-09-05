import os
import sys
import unittest
from openql import openql as ql


curdir = os.path.dirname(__file__)

output_dir = os.path.join(curdir, 'test_output')
ql.set_output_dir(output_dir)

sys.path.append(os.path.join(curdir, 'qisa-as', 'build'))


class Test_QISA_asssembler(unittest.TestCase):

    def test_QISA_assembler_present(self):
        # from pyQisaAs import QISA_Driver
        from pyQisaAs import QISA_Driver


try:
    from pyQisaAs import QISA_Driver
    assembler_present = True
except:
    assembler_present = False


def assemble(QISA_fn):
    # Test that the generated code is valid
    if assembler_present:
        driver = QISA_Driver()
        driver.enableScannerTracing(False)
        driver.enableParserTracing(False)
        driver.setVerbose(True)
        print("parsing file ", QISA_fn)
        success = driver.parse(QISA_fn)
        if not success:
            raise RuntimeError(driver.getLastErrorMessage())
        # Assembler(qumis_fn).convert_to_instructions()
