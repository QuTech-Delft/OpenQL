/** /file
 * Simple program for generating resource files.
 */

#include <iostream>
#include <fstream>
#include <iterator>
#include <algorithm>
#include <string>
#include <cctype>
#include <iomanip>

/**
 * Entry point.
 *
 * Expects the following arguments:
 *  - the name of the input file to turn into a resource;
 *  - the directory to place the resources in;
 *  - the name of the resource, which must be a valid C++ identifier.
 *
 * This generates a (private) include file with name <resource-dir>/<name>.inc,
 * with two static const variables declared inside it:
 *
 *  - `static const size_t <uppercase_name>_SIZE = <size>`; and
 *  - `static const char <uppercase_name>_DATA[] = <data>`.
 *
 * The size of the data array is the value in *_SIZE plus one, with a zero at
 * the end. Thus, if there are no embedded NULs, *_DATA can be used directly as
 * a C string.
 */
int main(int argc, char *argv[]) {

    // Parse command-line arguments.
    if (argc != 4) {
        std::cerr << "Usage: " << argv[0] << " <input-fname> <output-dir> <resource-identifier>" << std::endl;
        return 1;
    }
    std::string input_filename = argv[1];
    std::string output_directory = argv[2];
    std::string resource_identifier = argv[3];
    std::string output_filename = output_directory + "/" + resource_identifier + ".inc";
    std::transform(
        resource_identifier.begin(),
        resource_identifier.end(),
        resource_identifier.begin(),
        toupper
    );

    // Open the input data file.
    std::ifstream input_stream(input_filename, std::ios_base::in | std::ios_base::binary);
    if (!input_stream.is_open()) {
        std::cerr << "Failed to open input file " << input_filename << " for reading" << std::endl;
        return 1;
    }

    // Open the output data file.
    std::ofstream output_stream(output_filename);
    if (!output_stream.is_open()) {
        std::cerr << "Failed to open output file " << input_filename << " for writing" << std::endl;
        return 1;
    }

    // Write the data object, while tracking the size.
    output_stream << "/**\n * The contents of " << input_filename << ".\n*/\n";
    output_stream << "static const char " << resource_identifier << "_DATA[] = {";
    output_stream << std::hex << std::uppercase << std::setw(2) << std::setfill('0');
    size_t size = 0;
    std::istreambuf_iterator<char> it(input_stream), end;
    while (true) {
        if (size) {
            output_stream << ",";
        }
        if (size % 16 == 0) {
            output_stream << "\n    ";
        } else {
            output_stream << " ";
        }
        if (it == end) {
            output_stream << "0x00\n};\n\n";
            break;
        }
        output_stream << "0x" << (int)*it;
        size += 1;
        ++it;
    }
    output_stream << std::dec << std::setw(0);
    input_stream.close();

    // Write the size.
    output_stream << "/**\n * The size of " << input_filename << ".\n*/\n";
    output_stream << "static const size_t " << resource_identifier << "_SIZE = ";
    output_stream << size << ";\n";
    output_stream.close();

    return 0;
}
