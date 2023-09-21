cmake_minimum_required(VERSION 3.10)

project(SearchServer CXX)
set(CMAKE_CXX_STANDARD 17)


set(H_FILES log_duration.h concurrent_map.h document.h paginator.h process_queries.h read_input_functions.h
            remove_duplicates_h.h request_queue.h string_processing.h test_example_functions.h search_server.h)
set(CPP_FILES document.cpp process_queries.cpp read_input_functions.cpp remove_duplicates_cpp.cpp 
            request_queue.cpp string_processing.cpp test_example_functions.cpp  search_server.cpp)

if (CMAKE_SYSTEM_NAME MATCHES "^MINGW")
    set(SYSTEM_LIBS -lstdc++)
else()
    set(SYSTEM_LIBS)
endif()

add_executable(search_server ${H_FILES} ${CPP_FILES} main.cpp)

target_link_libraries(search_server ${SYSTEM_LIBS})