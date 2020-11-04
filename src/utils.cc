#include "utils.h"

namespace ql {

json load_json(const std::string &file_name) {
    std::ifstream fs(file_name);
    json j;
    if (fs.is_open()) {
        std::stringstream stripped; // file contents with comments stripped
        std::string line;

        // strip comments
        while (getline(fs, line)) {
            std::string::size_type n = line.find("//");
            if (n != std::string::npos) line.erase(n);
            std::istringstream iss(line);
            stripped << line;
        }

        try {
            stripped >> j;  // pass stripped line to json. NB: the whole file must be passed in 1 go
        } catch (json::parse_error &e) {
            // treat parse errors separately to give the user a clue about what's wrong
            EOUT("error parsing JSON file : \n\t" << e.what());
            if (e.byte != 0) {
                // go through file once again to find error position
                unsigned int lineNr = 1;
                size_t absPos = 0;
                fs.clear();
                fs.seekg(0, std::ios::beg);
                while (getline(fs, line)) {
                    std::string::size_type n = line.find("//");
                    if (n != std::string::npos) line.erase(n);
                    if (e.byte >= absPos && e.byte < absPos + line.size()) {
                        unsigned int relPos = e.byte - absPos;
                        line = utils::replace_all(line, "\t", " "); // make a TAB take one position
                        FATAL(
                            "in line " << lineNr
                                       << " at position " << relPos << ":" << std::endl
                                       << line << std::endl
                                       << std::string(relPos > 0 ? relPos - 1 : 0, ' ')
                                       << "^" << std::endl);
                    }
                    lineNr++;
                    absPos += line.size();
                }
                FATAL("error position " << e.byte << " points beyond last file position " << absPos);
            } else {
                FATAL("no information on error position");
            }
        }
        catch (json::exception &e) {
            FATAL("malformed JSON file : \n\t" << e.what());
        }
    } else {
        FATAL("failed to open file '" << file_name << "'");
    }
    return j;
}

} // namespace ql
