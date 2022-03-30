cmake_minimum_required(VERSION 3.11)

set(CMAKE_VERBOSE_MAKEFILE ON)
set(CMAKE_BUILD_TYPE Release)
SET(CMAKE_CXX_FLAGS "-std=c++20 -s -Wall -Os")

include(GNUInstallDirs)

include_directories(include)
include_directories(${PROJECT_BINARY_DIR}/framework/proto)
include_directories(${PROJECT_SOURCE_DIR}/framework/include)

set(PROTO proto)
set(PROXY websocket_proxy_async)
set(CLIENT websocket_client_async)
set(SERVER websocket_server_async)
set(GATEWAY websocket_gateway_async)

find_package(Protobuf REQUIRED)

add_executable(${PROXY} src/websocket_proxy_async.cpp)
add_executable(${CLIENT} src/websocket_client_async.cpp)
add_executable(${SERVER} src/websocket_server_async.cpp)
add_executable(${GATEWAY} src/websocket_gateway_async.cpp)

target_link_libraries(${PROXY} pthread ${PROTO} ${PROTOBUF_LIBRARY})
target_link_libraries(${CLIENT} pthread ${PROTO} ${PROTOBUF_LIBRARY})
target_link_libraries(${SERVER} pthread ${PROTO} ${PROTOBUF_LIBRARY})
target_link_libraries(${GATEWAY} pthread ${PROTO} ${PROTOBUF_LIBRARY})

install(TARGETS ${PROXY} ${CLIENT} ${SERVER} ${GATEWAY} DESTINATION ${PROJECT_SOURCE_DIR}/bin)
