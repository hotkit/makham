if(TARGET check)
    add_library(f5-makham-test STATIC EXCLUDE_FROM_ALL
            future.cpp
        )
    target_link_libraries(f5-makham-test f5-makham)
    smoke_test(f5-makham-test)
endif()
