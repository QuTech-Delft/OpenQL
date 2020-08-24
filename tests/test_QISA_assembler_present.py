import os
import sys
import unittest
from openql import openql as ql


curdir = os.path.dirname(__file__)

output_dir = os.path.join(curdir, 'test_output')
ql.set_option('output_dir', output_dir)

qmapFilename = curdir
sys.path.append(os.path.join(qmapFilename, 'qisa_opcodes.qmap'))

sys.path.append(os.path.join(curdir, 'qisa-as', 'build'))


driver = None

try:
    from qisa_as import QISA_Driver
    driver = QISA_Driver()
    assembler_present = True
except:
    assembler_present = False


def assemble(QISA_fn):

    # Test that the generated code is valid
    if assembler_present:
        # Try loading quantum instructions from FILE...
        success = driver.loadQuantumInstructions(qmapFilename)

        if success:
            driver.enableScannerTracing(False)
            driver.enableParserTracing(False)
            driver.setVerbose(True)
            print("parsing file ", QISA_fn)
            success = driver.assemble(QISA_fn)
            if not success:
                raise RuntimeError(driver.getLastErrorMessage())
            # Assembler(qumis_fn).convert_to_instructions()
