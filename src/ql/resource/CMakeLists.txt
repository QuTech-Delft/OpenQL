add_library(resource_
    "qubit.cc"
    "instrument.cc"
    "inter_core_channel.cc"
)

target_include_directories(resource_ PUBLIC include)
target_link_libraries(resource_ PUBLIC utils)
target_link_libraries(resource_ PUBLIC rmgr_resource_types)