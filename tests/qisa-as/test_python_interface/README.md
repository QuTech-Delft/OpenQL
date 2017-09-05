### Python interface for the QISA Assembler
`test_python_interface.py` is used to showcase the python interface of the
QISA assembler.

The file `../qisa_test_assembly/test_assembly.qumis` is assembled.
The verbose flag is set, so that the instructions are shown as they are
processed.

When the file has been correctly parsed, the resulting instructions are printed
on screen, and saved to a file named `test_assembly.out`.

To demonstrate the dissassembler, this output file (`test_assembly.out`) is
read back in and disassembled.
The output is printed on screen and saved to another file named `test_disassembly.out`.

Once you have built the QISA assembler using the procedure described in the
main README.md file, you can run this file as follows:

##### Linux
```
PYTHONPATH="<your-build-directory>" python3 test_python_interface.py
```

##### Windows (PowerShell)
```
$env:PYTHONPATH='<your-build-directory>'
python test_python_interface.py
```
