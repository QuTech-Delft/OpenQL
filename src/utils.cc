#include "utils.h"

#if defined(_WIN32)
#include <direct.h>
#else
#include <sys/types.h>
#include <sys/stat.h>
#endif

namespace ql {
namespace utils {
namespace logger {

log_level_t LOG_LEVEL;

void set_log_level(const std::string &level) {
    if (level == "LOG_NOTHING") {
        ql::utils::logger::LOG_LEVEL = ql::utils::logger::log_level_t::LOG_NOTHING;
    } else if (level == "LOG_CRITICAL") {
        ql::utils::logger::LOG_LEVEL = ql::utils::logger::log_level_t::LOG_CRITICAL;
    } else if (level == "LOG_ERROR") {
        ql::utils::logger::LOG_LEVEL = ql::utils::logger::log_level_t::LOG_ERROR;
    } else if (level == "LOG_WARNING") {
        ql::utils::logger::LOG_LEVEL = ql::utils::logger::log_level_t::LOG_WARNING;
    } else if (level == "LOG_INFO") {
        ql::utils::logger::LOG_LEVEL = ql::utils::logger::log_level_t::LOG_INFO;
    } else if(level == "LOG_DEBUG") {
        ql::utils::logger::LOG_LEVEL = ql::utils::logger::log_level_t::LOG_DEBUG;
    } else {
        std::cerr << "[OPENQL] " << __FILE__ << ":" << __LINE__
                  << " Error: Unknown log level" << std::endl;
    }
}

} // namespace logger

void make_output_dir(const std::string &dir) {
#if defined(_WIN32)
    _mkdir(dir.c_str());
#else
    mkdir(dir.c_str(), S_IRWXU | S_IRWXG | S_IROTH | S_IXOTH);
#endif
}

std::string to_lower(std::string str) {
    std::transform(
        str.begin(), str.end(), str.begin(),
        [](unsigned char c){ return std::tolower(c); }
    );
    return str;
}

/**
 * @param str
 *    string to be processed
 * @param from
 *    string to be replaced
 * @param to
 *    string used to replace from
 * @brief
 *    replace recursively from by to in str
 */
std::string replace_all(std::string str, const std::string &from, const std::string &to) {
    // https://stackoverflow.com/questions/2896600/how-to-replace-all-occurrences-of-a-character-in-string
    size_t start_pos = 0;
    while((start_pos = str.find(from, start_pos)) != std::string::npos) {
        str.replace(start_pos, from.length(), to);
        start_pos += to.length(); // Handles case where 'to' is a substring of 'from'
    }
    return str;
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

bool string_has(const std::string &str, const std::string &token) {
    return ( str.find(token) != std::string::npos);
}

// Helper function to sort the vector of pairs.
// Pairs are sorted by first element of pairs and then by second element
bool sort_pair_helper(
    const std::pair<size_t,size_t> &a,
    const std::pair<size_t,size_t> &b
) {
    if (a.first < b.first) {
        return true;
    } else if (a.first == b.first) {
        return a.second < b.second;
    } else {
        return false;
    }
}

} // namespace utils

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
