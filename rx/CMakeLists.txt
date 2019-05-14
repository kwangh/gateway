cmake_minimum_required(VERSION 3.9)

set(TARGET vdi_rx)
project(${TARGET} LANGUAGES CXX DESCRIPTION "VDI rx")
message(STATUS "${BoldBlue}${PROJECT_NAME} : ${PROJECT_DESCRIPTION}${ColorReset}")

add_executable(${TARGET}
  rx.cc
  )

target_link_libraries(${TARGET}
  boost_system
  -pthread
  -lboost_thread
  )
