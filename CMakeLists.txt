cmake_minimum_required(VERSION 3.12)
project(Memory_Database_master)

set(CMAKE_CXX_STANDARD 14)

include_directories(.)

add_executable(Memory_Database_master
        parser.cpp
        parser.h
        Btree.cpp
        Btree.h
        Buffer.cpp
        Buffer.h
        command.h
        exceptions.h
        loader.cpp
        loader.h
        #Log.cpp
        #Log.h
        main.cpp
        Manager.cpp
        Manager.h
        schema.cpp
        schema.h
        table.cpp
        table.h
        )
