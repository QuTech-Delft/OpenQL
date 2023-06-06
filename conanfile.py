from conan import ConanFile
from conan.tools.cmake import CMakeToolchain, CMakeDeps, CMake, cmake_layout


class OpenQLConan(ConanFile):
    name = "ql"

    settings = "os", "compiler", "build_type", "arch"
    options = {
        "build_tests": [True, False]
    }
    default_options = {
        "build_tests": False
    }

    def layout(self):
        cmake_layout(self)

    def build_requirements(self):
        self.requires("backward-cpp/1.6")
        self.requires("cimg/3.2.0")
        self.requires("eigen/3.4.0")
        self.requires("highs/1.4.2")
        self.requires("nlohmann_json/3.11.2")
        if self.options.build_tests:
            self.requires("doctest/2.4.9")
            self.requires("gtest/1.12.1")

    def generate(self):
        deps = CMakeDeps(self)
        deps.generate()
        tc = CMakeToolchain(self)
        tc.variables["OPENQL_BUILD_TESTS"] = self.options.build_tests
        tc.generate()

    def build(self):
        cmake = CMake(self)
        cmake.configure()
        cmake.build()
