cmake_minimum_required(VERSION 3.9)

set(TARGET monitor)
project(${TARGET} LANGUAGES CXX DESCRIPTION "lxc-monitor test")
message(STATUS "${BoldBlue}${PROJECT_NAME} : ${PROJECT_DESCRIPTION}${ColorReset}")

add_executable(${TARGET}
  util.cc
  )

set(CURL_LIBRARY "-lcurl")
find_package(CURL REQUIRED)
include_directories(${CURL_INCLUDE_DIR})
target_link_libraries(${TARGET} 
${CURL_LIBRARIES}
pthread)
