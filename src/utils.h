/**
 * @file   utils.h
 * @date   04/2017
 * @author Nader Khammassi
 *         Imran Ashraf
 * @brief  string utils (from qx)
 */

#ifndef QL_UTILS_H
#define QL_UTILS_H

#ifdef _MSC_VER
#ifdef BUILDING_OPENQL
#define OPENQL_DECLSPEC __declspec(dllexport)
#else
#define OPENQL_DECLSPEC __declspec(dllimport)
#endif
#else
#define OPENQL_DECLSPEC
#endif

#include <json.h>
#include <exception.h>

#include <limits>
#include <algorithm>
#include <iterator>
#include <string>
#include <sstream>
#include <fstream>
#include <iostream>
#include <utility>
#include <vector>

#define println(x) std::cout << "[OPENQL] "<< x << std::endl

#if defined(_WIN32)
#include <direct.h>
#else
#include <sys/types.h>
#include <sys/stat.h>
#endif

namespace ql
{

using json = nlohmann::json;

static size_t MAX_CYCLE = std::numeric_limits<int>::max();

/**
 * utils
 */
    namespace utils
    {
        inline void make_output_dir(std::string dir)
        {
            #if defined(_WIN32)
            _mkdir(dir.c_str());
            #else
            mkdir(dir.c_str(), S_IRWXU | S_IRWXG | S_IROTH | S_IXOTH);
            #endif
        }

        inline std::string to_lower(std::string str) {
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
        inline std::string replace_all(std::string str, const std::string &from, const std::string &to) {
            // https://stackoverflow.com/questions/2896600/how-to-replace-all-occurrences-of-a-character-in-string
            size_t start_pos = 0;
            while((start_pos = str.find(from, start_pos)) != std::string::npos) {
                str.replace(start_pos, from.length(), to);
                start_pos += to.length(); // Handles case where 'to' is a substring of 'from'
            }
            return str;
        }

        // from: https://stackoverflow.com/questions/5878775/how-to-find-and-replace-string
        // NB: also see replace_all
        template <typename T, typename U>
        T &replace (
                  T &str,
            const U &from,
            const U &to)
        {
            size_t pos;
            size_t offset = 0;
            const size_t increment = to.size();

            while ((pos = str.find(from, offset)) != T::npos)
            {
                str.replace(pos, from.size(), to);
                offset = pos + increment;
            }

            return str;
        }

        // from https://stackoverflow.com/questions/9146395/reset-c-int-array-to-zero-the-fastest-way
        template<typename T, size_t SIZE> inline void zero(T(&arr)[SIZE]){
            memset(arr, 0, SIZE*sizeof(T));
        }

        /**
        * write content to the file <file_name>
        */
        inline void write_file(std::string file_name, const std::string& content)
        {
            std::ofstream file;
//std::cout << "Try open file " <<  file_name << std::endl;
           
            file.open(file_name);
//std::cout << "opened file = " <<  file.fail() << std::endl;
            if ( file.fail() )
            {
                std::cout << "[x] error opening file '" << file_name << "' !" << std::endl
                          << "         make sure the output directory exists for '" << file_name << "'" << std::endl;
                return;
            }
            file << content;
            file.close();
        }

        template <typename T>
        std::string to_string(T arg)
        {
            std::stringstream ss;
            ss << arg;
            return ss.str ();
        }

        template<class T>
        std::string to_string(std::vector<T> v, std::string vector_prefix = "",
                              std::string elem_sep = ", ")
        {
            std::ostringstream ss;
            ss << vector_prefix << " [";
            size_t sz = v.size();
            if(sz > 0)
            {
                size_t i;
                for (i=0; i<sz-1; ++i)
                    ss << v[i] << elem_sep;
                ss << v[i];
            }

            ss << "]";
            return ss.str();
        }


        /**
         * print vector
         */
        template<typename T>
        void print_vector(std::vector<T> v, std::string prefix="", std::string separator=" | ")
        {
            std::cout << to_string(v, prefix, separator) << std::endl;
        }

        template <typename T>
        int sign_of(T val)
        {
            return (T(0) < val) - (val < T(0));
        }


        inline bool string_has(const std::string & str, const std::string & token)
        {
            return ( str.find(token) != std::string::npos);
        }

        // Helper function to sort the vector of pairs.
        // Pairs are sorted by first element of pairs and then by second element
        inline bool sort_pair_helper(const std::pair<size_t,size_t> &a, const std::pair<size_t,size_t> &b)
        {
            if(a.first < b.first)
                return true;
            else if (a.first == b.first)
                return (a.second < b.second);
            else
                return false;
        }

        namespace logger
        {
            enum log_level_t
            {
                LOG_NOTHING,
                LOG_CRITICAL,
                LOG_ERROR,
                LOG_WARNING,
                LOG_INFO,
                LOG_DEBUG
            };
            OPENQL_DECLSPEC extern log_level_t LOG_LEVEL;

            inline void set_log_level(std::string level)
            {
                if(level == "LOG_NOTHING")
                    ql::utils::logger::LOG_LEVEL = ql::utils::logger::log_level_t::LOG_NOTHING;
                else if(level == "LOG_CRITICAL")
                    ql::utils::logger::LOG_LEVEL = ql::utils::logger::log_level_t::LOG_CRITICAL;
                else if(level == "LOG_ERROR")
                    ql::utils::logger::LOG_LEVEL = ql::utils::logger::log_level_t::LOG_ERROR;
                else if(level == "LOG_WARNING")
                    ql::utils::logger::LOG_LEVEL = ql::utils::logger::log_level_t::LOG_WARNING;
                else if(level == "LOG_INFO")
                    ql::utils::logger::LOG_LEVEL = ql::utils::logger::log_level_t::LOG_INFO;
                else if(level == "LOG_DEBUG")
                    ql::utils::logger::LOG_LEVEL = ql::utils::logger::log_level_t::LOG_DEBUG;
                else
                    std::cerr << "[OPENQL] " << __FILE__ <<":"<< __LINE__ <<" Error: Unknown log level" << std::endl;
            }

        } // logger namespace

    } // utils namespace


} // ql namespace

#define EOUT(content) \
    if ( ql::utils::logger::LOG_LEVEL >= ql::utils::logger::log_level_t::LOG_ERROR ) \
        std::cerr << "[OPENQL] " << __FILE__ <<":"<< __LINE__ <<" Error: "<< content << std::endl

#define WOUT(content) \
    if ( ql::utils::logger::LOG_LEVEL >= ql::utils::logger::log_level_t::LOG_WARNING ) \
        std::cerr << "[OPENQL] " << __FILE__ <<":"<< __LINE__ <<" Warning: "<< content << std::endl

#define IOUT(content) \
    if ( ql::utils::logger::LOG_LEVEL >= ql::utils::logger::log_level_t::LOG_INFO ) \
        std::cout << "[OPENQL] " << __FILE__ <<":"<< __LINE__ <<" Info: "<< content << std::endl

#define DOUT(content) \
    if ( ql::utils::logger::LOG_LEVEL >= ql::utils::logger::log_level_t::LOG_DEBUG ) \
        std::cout << "[OPENQL] " << __FILE__ <<":"<< __LINE__ <<" "<< content << std::endl

#define COUT(content) \
        std::cout << "[OPENQL] " << __FILE__ <<":"<< __LINE__ <<" "<< content << std::endl

// helper macro: stringstream to string
// based on https://stackoverflow.com/questions/21924156/how-to-initialize-a-stdstringstream
#define SS2S(values) std::string(static_cast<std::ostringstream&&>(std::ostringstream() << values).str())

#define FATAL(content) \
        {   EOUT(content); \
            throw ql::exception(SS2S("Error : " << content), false); \
        }

#define ASSERT(condition)   { if (!(condition)) { FATAL("assert " #condition " failed in file " __FILE__ " at line " << __LINE__); } }


// get the number of elements in an array
#define ELEM_CNT(x) (sizeof(x)/sizeof(x[0]))

// check existence of JSON key within node, see PR #194
#define JSON_EXISTS(node, key)  ((node).count(key) > 0)

#define JSON_ASSERT(node, key, nodePath) \
        {   if(!JSON_EXISTS(node, key)) { \
                FATAL("key '" << key << "' not found on path '" << nodePath << "', actual node contents '" << node << "'"); \
            } \
        }

namespace ql {

static json load_json(std::string file_name) {
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

// get json value with error notification
// based on: https://github.com/nlohmann/json/issues/932
template<class T>
T json_get(const json &j, std::string key, std::string nodePath = "") {
    // first check existence of key
    auto it = j.find(key);
    if (it == j.end()) {
        FATAL("Key '" << key
                      << "' not found on path '" << nodePath
                      << "', actual node contents '" << j << "'");
    }

    // then try to get key
    try {
        return it->get<T>();
    } catch (const std::exception &e) {
        FATAL("Could not get value of key '" << key
                                             << "' on path '" << nodePath
                                             << "', exception message '"
                                             << e.what()
                                             << "', actual node contents '" << j
                                             << "'");
    }
}


}

#endif //QL_UTILS_H

