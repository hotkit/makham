add_library(makham-headers-tests STATIC EXCLUDE_FROM_ALL
        async.cpp
        executor.cpp
        future.cpp
        unit.cpp
    )
target_link_libraries(makham-headers-tests f5-makham)
add_dependencies(check makham-headers-tests)
