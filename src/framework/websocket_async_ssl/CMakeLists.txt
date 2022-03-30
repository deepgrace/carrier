cmake_minimum_required(VERSION 3.11)

set(CMAKE_VERBOSE_MAKEFILE ON)
set(CMAKE_BUILD_TYPE Release)
SET(CMAKE_CXX_FLAGS "-std=c++20 -s -Wall -Os")

include(GNUInstallDirs)

include_directories(include)
include_directories(${PROJECT_BINARY_DIR}/framework/proto)
include_directories(${PROJECT_SOURCE_DIR}/framework/include)

set(PROTO proto)
set(PROXY websocket_proxy_async_ssl)
set(CLIENT websocket_client_async_ssl)
set(SERVER websocket_server_async_ssl)
set(GATEWAY websocket_gateway_async_ssl)

find_package(Protobuf REQUIRED)

add_executable(${PROXY} src/websocket_proxy_async_ssl.cpp)
add_executable(${CLIENT} src/websocket_client_async_ssl.cpp)
add_executable(${SERVER} src/websocket_server_async_ssl.cpp)
add_executable(${GATEWAY} src/websocket_gateway_async_ssl.cpp)

target_link_libraries(${PROXY} pthread crypto ssl ${PROTO} ${PROTOBUF_LIBRARY})
target_link_libraries(${CLIENT} pthread crypto ssl ${PROTO} ${PROTOBUF_LIBRARY})
target_link_libraries(${SERVER} pthread crypto ssl ${PROTO} ${PROTOBUF_LIBRARY})
target_link_libraries(${GATEWAY} pthread crypto ssl ${PROTO} ${PROTOBUF_LIBRARY})

install(TARGETS ${PROXY} ${CLIENT} ${SERVER} ${GATEWAY} DESTINATION ${PROJECT_SOURCE_DIR}/bin)
