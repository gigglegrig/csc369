cmake_minimum_required(VERSION 3.10)
project(starter_code)

set(CMAKE_CXX_STANDARD 11)

include_directories(.)

add_executable(starter_code
        interceptor.c
        interceptor.h
        Kbuild
        Makefile
        test_full.c
        test_intercept.c)
