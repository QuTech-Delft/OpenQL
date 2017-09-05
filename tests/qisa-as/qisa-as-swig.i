%begin %{
#ifdef _MSC_VER
#define SWIG_PYTHON_INTERPRETER_NO_DEBUG
#endif
%}

%module pyQisaAs
%{
#include "qisa_driver.h"
%}

%include std_string.i
using std::string;

namespace QISA
{

class QISA_Driver
{
public:

  %feature("autodoc", "Constructor");
  QISA_Driver();

  %feature("autodoc");
  virtual ~QISA_Driver();

  %feature("autodoc", "
Return a string that represents the version of the assembler.
");
  static std::string getVersion();

  %feature("autodoc", "
Enable or disable scanner (flex) tracing.
This is a debugging aid that can be used during development of this assembler.

Parameters
----------
enabled  -- True if scanner tracing should be enabled, False if it should be disabled.
");
  void enableScannerTracing(bool enabled);

  %feature("autodoc", "
Enable or disable parser (bison) tracing.
This is a debugging aid that can be used during development of this assembler.

Parameters
----------
enabled: bool  -- True if parser tracing should be enabled, False if it should be disabled.
");
  void enableParserTracing(bool enabled);

  %feature("autodoc", "
Run the parser on the given file.

Parameters
----------
filename: str File to parse.

Returns
-------
--> bool: True on success, false on failure.
");
  bool parse(const std::string& filename);

%feature("autodoc", "
Disassemble the given file.

Parameters
----------
filename: str File to disassemble.

Returns
-------
--> bool: True on success, false on failure.
");
  bool disassemble(const std::string& filename);

  %feature("autodoc", "
Returns
-------
--> str: The last generated error message.
");
  std::string getLastErrorMessage();

  %feature("autodoc", "
Change the verbosity of the assembler.
This determines whether or not informational messages are shown while the assembler decodes its input instructions.

Parameters
----------
verbose: bool  -- Specifies the verbosity of the assembler.

");
  void setVerbose(bool verbose);

  %feature("autodoc", "
Retrieve the generated code as a multi-line string that contains the hex values of the encoded instructions.

Returns
-------
--> str: The generated instructions, one encoded instruction per line..
");
  std::string getInstructionsAsHexStrings();

%feature("autodoc", "
Retrieve the disassembly output as a multi-line string.

Returns
-------
--> str: The disassembly output: one (or more, in case of quantum) disassembled instruction per line.
");
  std::string getDisassemblyOutput();


  %feature("autodoc", "
Save binary assembled or textual disassembled instructions to given output file.

Parameters
----------
outputFileName: str  -- Name of the file in which to store the generated output.
");
  bool save(const std::string& outputFileName);
};

}
