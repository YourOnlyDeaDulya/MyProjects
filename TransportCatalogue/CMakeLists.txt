cmake_minimum_required(VERSION 3.10)

project(PhoneBook CXX)
set(CMAKE_CXX_STANDARD 17)

find_package(Protobuf REQUIRED)
find_package(Threads REQUIRED)

protobuf_generate_cpp(PROTO_SRCS PROTO_HDRS transport_catalogue.proto map_renderer.proto svg.proto graph.proto transport_router.proto)

set(DOMAIN_FILES domain.h domain.cpp)
set(GEO_FILES geo.h geo.cpp)
set(GRAPH_FILES graph.h router.h ranges.h transport_router.h transport_router.cpp)
set(RENDERER_FILES svg.h svg.cpp map_renderer.h map_renderer.cpp)
set(JSON_LIB_FILES json.h json.cpp json_reader.h json_reader.cpp 
   json_builder.h json_builder.cpp request_handler.h request_handler.cpp)
set(SERIALIZATION_FILES serialization.h serialization.cpp svg.proto map_renderer.proto transport_catalogue.proto graph.proto transport_router.proto)
set(CATALOGUE_FILES transport_catalogue.h transport_catalogue.cpp)

add_executable(transport_catalogue ${PROTO_SRCS} ${PROTO_HDRS} ${DOMAIN_FILES} ${GEO_FILES}
				 ${CATALOGUE_FILES} ${RENDERER_FILES} ${GRAPH_FILES} ${JSON_LIB_FILES} ${SERIALIZATION_FILES} main.cpp)

target_include_directories(transport_catalogue PUBLIC ${Protobuf_INCLUDE_DIRS})
target_include_directories(transport_catalogue PUBLIC ${CMAKE_CURRENT_BINARY_DIR})

string(REPLACE "protobuf.lib" "protobufd.lib" "Protobuf_LIBRARY_DEBUG" "${Protobuf_LIBRARY_DEBUG}")
string(REPLACE "protobuf.a" "protobufd.a" "Protobuf_LIBRARY_DEBUG" "${Protobuf_LIBRARY_DEBUG}")

target_link_libraries(transport_catalogue "$<IF:$<CONFIG:Debug>,${Protobuf_LIBRARY_DEBUG},${Protobuf_LIBRARY}>" Threads::Threads)