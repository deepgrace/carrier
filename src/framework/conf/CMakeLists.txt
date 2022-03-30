cmake_minimum_required(VERSION 3.11)

include(GNUInstallDirs)

set(CMAKE_VERBOSE_MAKEFILE ON)
set(CONFIGFILE hosts.conf)

install(FILES ${CONFIGFILE} DESTINATION ${PROJECT_SOURCE_DIR}/bin)
