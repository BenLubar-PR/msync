add_executable(tests "")
target_sources_local(tests PRIVATE main.cpp option_file.cpp test_helpers.hpp argparsing.cpp user_options.cpp global_options.cpp)
target_link_libraries(tests PRIVATE Catch2::Catch2 options)