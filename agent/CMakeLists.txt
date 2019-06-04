cmake_minimum_required(VERSION 3.9)

set(TARGET vds_ca)
project(${TARGET} LANGUAGES CXX DESCRIPTION "vds control agent")
message(STATUS "${BoldBlue}${PROJECT_NAME} : ${PROJECT_DESCRIPTION}${ColorReset}")

find_package(Boost 1.62.0 REQUIRED COMPONENTS system)

set (CMAKE_CXX_STANDARD 11)

add_executable(${TARGET}
  agent.cc
  cds_http.cc
  )

target_link_libraries(${TARGET}
  boost_system
  pthread
  )
