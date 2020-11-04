#include "utils/filesystem.h"

#include <iostream>
#include <fstream>

#if defined(_WIN32)
#include <direct.h>
#else
#include <sys/types.h>
#include <sys/stat.h>
#endif

namespace ql {
namespace utils {

void make_output_dir(const std::string &dir) {
#if defined(_WIN32)
    _mkdir(dir.c_str());
#else
    mkdir(dir.c_str(), S_IRWXU | S_IRWXG | S_IROTH | S_IXOTH);
#endif
}

/**
 * write content to the file <file_name>
 */
void write_file(const std::string &file_name, const std::string &content) {
    std::ofstream file;
    //std::cout << "Try open file " <<  file_name << std::endl;

    file.open(file_name);
    //std::cout << "opened file = " <<  file.fail() << std::endl;
    if (file.fail()) {
        std::cout << "[x] error opening file '" << file_name << "' !" << std::endl
                  << "         make sure the output directory exists for '" << file_name << "'" << std::endl;
        return;
    }
    file << content;
    file.close();
}

} // namespace utils
} // namespace ql
