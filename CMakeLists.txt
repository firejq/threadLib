cmake_minimum_required(VERSION 3.9)
project(userLevelThreadsLib)

set(CMAKE_CXX_STANDARD 98)

add_executable(userLevelThreadsLib test/test.cpp u_thread.cpp u_thread.h)