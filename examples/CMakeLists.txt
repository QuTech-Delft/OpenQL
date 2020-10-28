cmake_minimum_required(VERSION 3.1 FATAL_ERROR)

add_openql_test(multi_qubits_randomized_benchmarking multi_qubits_randomized_benchmarking.cc ../tests)
add_openql_test(randomized_benchmarking randomized_benchmarking.cc ../tests)
add_openql_test(rb_single rb_single.cc ../tests)
add_openql_test(simple simple.cc ../tests)
