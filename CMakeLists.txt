cmake_minimum_required(VERSION 3.30)
project(birdserver)

set(CMAKE_CXX_STANDARD 20)

add_executable(birdserver main.cpp
        headers/server.h
        headers/threadpool.h
        src/server.cpp
        main.cpp
        src/threadpool.cpp
        src/connection.cpp
        headers/connection.h
        src/respond.cpp
)
