#include <iostream>
#include <fstream>
#include <sstream>
#include <cstdlib>
#include <cstring>

#include "qisa_driver.h"

std::string usage(const std::string& progName)
{
  std::stringstream ss;
  ss << "Usage: " << progName << " [OPTIONS] INPUT_FILE" << std::endl;
  ss << "Assembler/Disassembler for the Quantum Instuction Set Architecture (QISA)." << std::endl;
  ss << std::endl;
  ss << "Options:" << std::endl;
  ss << "  -d                Disassemble the given INPUT_FILE" << std::endl;
  ss << "  -o OUTPUT_FILE    Save binary assembled or textual disassembled instructions to the given OUTPUT_FILE" << std::endl;
  ss << "  -t                Enable scanner and parser tracing while assembling" << std::endl;
  ss << "  -V, --version     Show the program version and exit" << std::endl;
  ss << "  -v, --verbose     Show informational messages while assembling" << std::endl;
  ss << "  -h, --help        Show this help message and exit" << std::endl;

  return ss.str();
}

int
main(const int argc, const char **argv)
{
  bool enableTrace = false;
  bool enableVerbose = false;
  bool doDisassemble = false;
  const char* inputFilename = 0;
  const char* outputFilename = 0;

  std::string progName = argv[0];

  // EXTRACT PROGRAM NAME

  const char pathSep =
#ifdef _WIN32
  '\\';
#else
  '/';
#endif

  size_t spos = progName.find_last_of(pathSep);
  if (spos != std::string::npos)
      progName = progName.substr(spos + 1);

  // Parse the command line arguments.
  for (int i = 1; i < argc; i++ )
  {
    const char* arg = argv[i];

    if (arg[0] == '-')
    {
      // This is an option.

      if (!std::strcmp(arg, "-h") ||
          !std::strcmp(arg, "--help"))
      {
        std::cout << usage(progName);
        return EXIT_SUCCESS;
      }

      else if (!std::strcmp(arg, "-V") ||
               !std::strcmp(arg, "--version"))
      {
        std::cout << progName << " (Quantum Instuction Set Architecture Assembler) version " << QISA::QISA_Driver::getVersion() << std::endl;
        return EXIT_SUCCESS;
      }
      else if (!std::strcmp(arg, "-v") ||
               !std::strcmp(arg, "--verbose"))
      {
        enableVerbose = true;
      }

      else if (!std::strcmp(arg, "-t"))
      {
        enableTrace = true;
      }

      else if (!std::strcmp(arg, "-d"))
      {
        doDisassemble = true;
      }

      else if (!std::strcmp(arg, "-o"))
      {
        outputFilename = argv[++i];
      }

      else
      {
        std::cerr << progName << ": Unrecognized option: '" << arg << "'" << std::endl
                  << "Try " << progName << " --help for more information." << std::endl;
        return EXIT_FAILURE;
      }
    }
    else
    {
      // This command line argument is not an option.
      // Assume that this is the input filename.
      // Issue an error if an input filename has already been specified.
      if (inputFilename != 0)
      {
        std::cerr << progName << ": Too many input files specified" << std::endl
                  << "Try " << progName << " --help for more information." << std::endl;
        return EXIT_FAILURE;
      }

      inputFilename = arg;
    }
  }

  if (inputFilename == 0)
  {
    std::cerr << progName << ": No input file specified?" << std::endl
              << "Try " << progName << " --help for more information." << std::endl;
    return EXIT_FAILURE;
  }

  // Check if the given input file can be opened for reading.
  // Issue an error if not.
  {
    std::ifstream tstFileStream(inputFilename);
    if (tstFileStream.fail())
    {
      std::cerr << progName << ": Cannot open file '" << inputFilename << "'" << std::endl;
    }
  }

  QISA::QISA_Driver driver;

  driver.enableScannerTracing(enableTrace);
  driver.enableParserTracing(enableTrace);
  driver.setVerbose(enableVerbose);

  /* Parse the file. */
  bool success;

  if (doDisassemble)
  {
    success = driver.disassemble(inputFilename);
  }
  else
  {
    success = driver.parse(inputFilename);
  }

  if (success)
  {
    if (outputFilename == 0)
    {
      if (doDisassemble)
      {
        std::cout << "Disassembly output:" << std::endl;
        std::cout << driver.getDisassemblyOutput();
      }
      else
      {
        std::cout << "Generated assembly:" << std::endl;
        std::cout << driver.getInstructionsAsHexStrings();
      }
    }
    else
    {

      bool save_result = driver.save(outputFilename);

      if (!save_result)
      {
        std::cerr << "Saving terminated with errors:" << std::endl;
        std::cerr << driver.getLastErrorMessage();
        return EXIT_FAILURE;
      }
    }

    return EXIT_SUCCESS;
  }
  else
  {
    std::cout << driver.getLastErrorMessage() << std::endl;

    if (doDisassemble)
    {
      std::cout << "Disassembly terminated with errors." << std::endl;
    }
    else
    {
      std::cout << "Assembly terminated with errors." << std::endl;
    }

    return EXIT_FAILURE;
  }

}
