cmake_minimum_required(VERSION 3.10)
project(f5-makham)

add_subdirectory(makham)
add_subdirectory(thread-pool)

if(TARGET check)
    set_property(TARGET check PROPERTY EXCLUDE_FROM_ALL TRUE)
    add_subdirectory(test)
endif()
