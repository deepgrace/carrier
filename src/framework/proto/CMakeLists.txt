cmake_minimum_required(VERSION 3.11)

include(GNUInstallDirs)

set(PROTO proto)
set(CMAKE_VERBOSE_MAKEFILE ON)
set(CMAKE_BUILD_TYPE Release)
SET(CMAKE_CXX_FLAGS "-std=c++20 -s -Wall -Os")

find_package(Protobuf REQUIRED)
file(GLOB PROTOS "*.proto")
PROTOBUF_GENERATE_CPP(PROTO_SRC PROTO_HEADER ${PROTOS})

add_library(${PROTO} SHARED ${PROTO_HEADER} ${PROTO_SRC})
install(TARGETS ${PROTO} DESTINATION ${PROJECT_SOURCE_DIR}/lib)
target_link_libraries(${PROTO} ${PROTOBUF_LIBRARY})
