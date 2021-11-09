cmake_minimum_required(VERSION 3.5 FATAL_ERROR)
project(test_driver)
set( CMAKE_CXX_FLAGS "-std=c++11" )
add_library(device_lib
        serialport_driver.cpp
        serialport_driver.h
        )
add_executable(test_driver test_driver.cpp)
target_link_libraries(test_driver device_lib pthread)