cmake_minimum_required(VERSION 3.11)

set(CMAKE_VERBOSE_MAKEFILE ON)
set(CMAKE_BUILD_TYPE Release)
SET(CMAKE_CXX_FLAGS "-std=c++20 -fconcepts -s -Wall -Os")

include(GNUInstallDirs)

include_directories(include)
include_directories(${PROJECT_SOURCE_DIR}/framework/include)

set(FILESYSTEM stdc++fs)

set(CLIENT_SYNC file_transfer_client_sync)
set(CLIENT_ASYNC file_transfer_client_async)
set(SERVER_ASYNC file_transfer_server_async)

add_executable(${CLIENT_SYNC} src/file_transfer_client_sync.cpp)
add_executable(${CLIENT_ASYNC} src/file_transfer_client_async.cpp)
add_executable(${SERVER_ASYNC} src/file_transfer_server_async.cpp)

target_link_libraries(${CLIENT_SYNC} pthread ${FILESYSTEM})
target_link_libraries(${CLIENT_ASYNC} pthread ${FILESYSTEM})
target_link_libraries(${SERVER_ASYNC} pthread ${FILESYSTEM})

install(TARGETS ${CLIENT_SYNC} ${CLIENT_ASYNC} ${SERVER_ASYNC} DESTINATION ${PROJECT_SOURCE_DIR}/bin)
