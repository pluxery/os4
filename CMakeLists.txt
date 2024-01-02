cmake_minimum_required(VERSION 3.27)
project(os4)

set(CMAKE_CXX_STANDARD 11)

add_executable(os4 my_serial.hpp main.cpp
        repository.hpp
        help_utils.hpp
        db/sqlite3.c
        db/sqlite3.h
)

