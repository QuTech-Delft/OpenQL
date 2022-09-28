add_pass_library(pass_ana_statistics
    "report.cc"
    "clean.cc"
)

target_include_directories(pass_ana_statistics PUBLIC include)

target_link_libraries(pass_ana_statistics PUBLIC utils)
target_link_libraries(pass_ana_statistics PUBLIC pmgr)
target_link_libraries(pass_ana_statistics PUBLIC ir)