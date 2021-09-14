/** \file
 * Provides utilities for handling JSON files, and wraps nlohmann::json in
 * OpenQL's code style.
 */

#include "ql/utils/json.h"

#include <fstream>
#include "ql/utils/str.h"

namespace ql {
namespace utils {

/**
 * Parses JSON data that may include // comments.
 */
Json parse_json(std::istream &is) {
    Json j;
    std::stringstream stripped; // file contents with comments stripped
    Str line;

    // strip comments
    while (getline(is, line)) {
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
            is.clear();
            is.seekg(0, std::ios::beg);
            while (getline(is, line)) {
                Str::size_type n = line.find("//");
                if (n != Str::npos) line.erase(n);
                if (e.byte >= absPos && e.byte < absPos + line.size()) {
                    unsigned int relPos = e.byte - absPos;
                    line = utils::replace_all(line, "\t", " "); // make a TAB take one position
                    QL_JSON_ERROR(
                        "in line " << lineNr
                                   << " at position " << relPos << ":" << std::endl
                                   << line << std::endl
                                   << Str(relPos > 0 ? relPos - 1 : 0, ' ')
                                   << "^" << std::endl);
                }
                lineNr++;
                absPos += line.size();
            }
            QL_ICE("error position " << e.byte << " points beyond last file position " << absPos);
        } else {
            QL_ICE("no information on error position");
        }
    }
    catch (Json::exception &e) {
        QL_JSON_ERROR("malformed JSON file : \n\t" << e.what());
    }
    return j;
}

/**
 * Parses JSON data that may include // comments.
 */
Json parse_json(const Str &data) {
    auto is = std::istringstream(data);
    return parse_json(is);
}

/**
 * Loads a JSON file that may include // comments.
 */
Json load_json(const Str &path) {
    std::ifstream fs(path);
    if (fs.is_open()) {
        return parse_json(fs);
    } else {
        QL_USER_ERROR("failed to open file '" << path << "'");
    }
}

template<>
const Json &json_get(const Json &j, const Str &key, const Str &nodePath) {
    // first check existence of key
    auto it = j.find(key);
    if (it == j.end()) {
        QL_JSON_ERROR("Key '" << key
                              << "' not found on path '" << nodePath
                              << "', actual node contents '" << j << "'");
    }

    return *it;
}

} // namespace utils
} // namespace ql
