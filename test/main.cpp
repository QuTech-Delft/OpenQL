#include <gtest/gtest.h>
#include <iostream>

int main_impl(int argc, char** argv, std::ostream&) {
    ::testing::InitGoogleTest(&argc, argv);

    return RUN_ALL_TESTS();
}

int main(int argc, char** argv) {
    return main_impl(argc, argv, std::cout);
}
