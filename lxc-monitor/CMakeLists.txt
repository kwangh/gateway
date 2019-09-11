cmake_minimum_required(VERSION 3.9)

set(TARGET monitor)
project(${TARGET} LANGUAGES CXX DESCRIPTION "lxc-monitor process")
message(STATUS "${BoldBlue}${PROJECT_NAME} : ${PROJECT_DESCRIPTION}${ColorReset}")

add_executable(${TARGET}
  lxc-monitor.cc
  )

find_library(LXC_LIB lxc)
target_link_libraries(${TARGET}
pthread
${LXC_LIB}
)
