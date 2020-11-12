/** \file
 * Provides utilities for handling JSON files, and wraps nlohmann::json in
 * OpenQL's code style.
 */

#include "utils/json.h"

#include <fstream>
#include "utils/str.h"

namespace ql {
namespace utils {

/**
 * Loads a JSON file that may include // comments.
 */
Json load_json(const Str &path) {
    std::ifstream fs(path);
    Json j;
    if (fs.is_open()) {
        std::stringstream stripped; // file contents with comments stripped
        Str line;

        // strip comments
        while (getline(fs, line)) {
            Str::size_type n = line.find("//");
            if (n != Str::npos) line.erase(n);
            std::istringstream iss(line);
            stripped << line;
        }

        try {
            stripped >> j;  // pass stripped line to json. NB: the whole file must be passed in 1 go
        } catch (Json::parse_error &e) {
            // treat parse errors separately to give the user a clue about what's wrong
            QL_EOUT("error parsing JSON file : \n\t" << e.what());
            if (e.byte != 0) {
                // go through file once again to find error position
                unsigned int lineNr = 1;
                size_t absPos = 0;
                fs.clear();
                fs.seekg(0, std::ios::beg);
                while (getline(fs, line)) {
                    Str::size_type n = line.find("//");
                    if (n != Str::npos) line.erase(n);
                    if (e.byte >= absPos && e.byte < absPos + line.size()) {
                        unsigned int relPos = e.byte - absPos;
                        line = utils::replace_all(line, "\t", " "); // make a TAB take one position
                        QL_FATAL(
                            "in line " << lineNr
                                       << " at position " << relPos << ":" << std::endl
                                       << line << std::endl
                                       << Str(relPos > 0 ? relPos - 1 : 0, ' ')
                                       << "^" << std::endl);
                    }
                    lineNr++;
                    absPos += line.size();
                }
                QL_FATAL("error position " << e.byte << " points beyond last file position " << absPos);
            } else {
                QL_FATAL("no information on error position");
            }
        }
        catch (Json::exception &e) {
            QL_FATAL("malformed JSON file : \n\t" << e.what());
        }
    } else {
        QL_FATAL("failed to open file '" << path << "'");
    }
    return j;
}

} // namespace utils
} // namespace ql
