cmake_minimum_required(VERSION 3.1 FATAL_ERROR)
project(CheckBitness C CXX)
configure_file(
    "${CMAKE_CURRENT_SOURCE_DIR}/values.cfg.template"
    "${CMAKE_CURRENT_BINARY_DIR}/values.cfg"
)
