#include <openql>

int main(int argc, char** argv) {
    if (argc > 3) {
        return 1;
    }

    std::string filename = argv[1];

    bool debug = false;
    if (argc == 3) {
        std::string opt = argv[2];
        if (opt != "-d") {
            std::cout << "Wrong usage" << std::endl;
            return 1;
        }

        debug = true;
    }

    if (debug) {
        ql::set_option("log_level", "LOG_DEBUG");
    }

    ql::compile(filename);
    return 0;
}