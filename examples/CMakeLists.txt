cmake_minimum_required(VERSION 3.1 FATAL_ERROR)

# FIXME: the commented-out examples seem to rely on stuff that was changed or
# removed. They should be fixed or removed altogether.
# add_openql_test(multi_qubits_randomized_benchmarking multi_qubits_randomized_benchmarking.cc ../tests)
# add_openql_test(randomized_benchmarking randomized_benchmarking.cc ../tests)
# add_openql_test(rb_single rb_single.cc ../tests)
add_openql_test(simple simple.cc ../tests)
