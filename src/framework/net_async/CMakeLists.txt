cmake_minimum_required(VERSION 3.11)

set(CMAKE_VERBOSE_MAKEFILE ON)
set(CMAKE_BUILD_TYPE Release)
SET(CMAKE_CXX_FLAGS "-std=c++20 -s -Wall -Os")

include(GNUInstallDirs)

include_directories(include)
include_directories(${PROJECT_BINARY_DIR}/framework/proto)
include_directories(${PROJECT_SOURCE_DIR}/framework/include)
include_directories(${PROJECT_SOURCE_DIR}/framework/protocol)

set(PROTO proto)
set(PROXY net_proxy_async)
set(CLIENT net_client_async)
set(SERVER net_server_async)
set(GATEWAY net_gateway_async)

find_package(Protobuf REQUIRED)

add_executable(${PROXY} src/net_proxy_async.cpp)
add_executable(${CLIENT} src/net_client_async.cpp)
add_executable(${SERVER} src/net_server_async.cpp)
add_executable(${GATEWAY} src/net_gateway_async.cpp)

target_link_libraries(${PROXY} pthread ${PROTO} ${PROTOBUF_LIBRARY})
target_link_libraries(${CLIENT} pthread ${PROTO} ${PROTOBUF_LIBRARY})
target_link_libraries(${SERVER} pthread ${PROTO} ${PROTOBUF_LIBRARY})
target_link_libraries(${GATEWAY} pthread ${PROTO} ${PROTOBUF_LIBRARY})

install(TARGETS ${PROXY} ${CLIENT} ${SERVER} ${GATEWAY} DESTINATION ${PROJECT_SOURCE_DIR}/bin)
