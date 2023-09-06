import os

from conan import ConanFile
from conan.tools.cmake import CMakeToolchain, CMakeDeps, CMake, cmake_layout
from conan.tools.files import copy


class OpenQLConan(ConanFile):
    name = "openql"

    settings = "os", "compiler", "build_type", "arch"
    options = {
        "shared": [True, False],
        "build_python": [True, False],
        "build_tests": [True, False],
        "debug_symbols": [True, False],
        "disable_unitary": [True, False],
        "python_dir": [None, "ANY"],
        "python_ext": [None, "ANY"]
    }
    default_options = {
        "shared": False,
        "build_python": False,
        "build_tests": False,
        "debug_symbols": False,
        "disable_unitary": True,
        "python_dir": None,
        "python_ext": None
    }

    def layout(self):
        cmake_layout(self)

    exports_sources = "CMakeLists.txt", "include/*", "python/*", "res/*", "src/*", "test/*"

    def build_requirements(self):
        self.requires("backward-cpp/1.6")
        self.requires("cimg/3.2.0")
        self.requires("eigen/3.4.0")
        self.requires("highs/1.4.2")
        self.tool_requires("m4/1.4.19")
        self.requires("nlohmann_json/3.11.2")
        if self.settings.os == "Windows":
            self.tool_requires("winflexbison/2.5.24")
        else:
            if self.settings.arch != "armv8":
                self.tool_requires("flex/2.6.4")
                self.tool_requires("bison/3.8.2")
        if self.settings.arch != "armv8":
            self.tool_requires("zulu-openjdk/11.0.19")
        if self.options.build_tests:
            self.requires("gtest/1.14.0")

    def requirements(self):
        self.requires("antlr4-cppruntime/4.13.0")

    def generate(self):
        deps = CMakeDeps(self)
        deps.generate()
        tc = CMakeToolchain(self)
        tc.variables["OPENQL_BUILD_PYTHON"] = self.options.build_python
        tc.variables["OPENQL_BUILD_TESTS"] = self.options.build_tests
        tc.variables["OPENQL_DEBUG_SYMBOLS"] = self.options.debug_symbols
        tc.variables["OPENQL_PYTHON_DIR"] = self.options.python_dir
        tc.variables["OPENQL_PYTHON_EXT"] = self.options.python_ext
        tc.variables["OPENQL_WITH_UNITARY_DECOMPOSITION"] = not self.options.disable_unitary
        tc.generate()

    def build(self):
        cmake = CMake(self)
        cmake.configure()
        cmake.build()

    def package(self):
        copy(self, "LICENSE.md", src=self.source_folder, dst=os.path.join(self.package_folder, "licenses"))
        cmake = CMake(self)
        cmake.install()

    def package_info(self):
        self.cpp_info.libs = ["ql"]
