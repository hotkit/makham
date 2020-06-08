/**
    Copyright 2019-2020 Red Anchor Trading Co. Ltd.

    Distributed under the Boost Software License, Version 1.0.
    See <http://www.boost.org/LICENSE_1_0.txt>
 */


#pragma once


#include <thread-pool/fixed_function.hpp>
#include <f5/makham/coroutine.hpp>

#ifndef NDEBUG
#include <iostream>
#endif


namespace f5::makham {


    /// Fixes size for the closure allowed when posting functions
    /// to the executor.
    using function_type = tp::FixedFunction<void(), 128>;


    /// Execute the function in the Makham executor's thread pool.
    void post(function_type);

    /// Resume this coroutine handle as a new job in the Makham
    /// executor's thread pool.
    inline void post(coroutine_handle<> coro) {
        if (coro) {
            post([coro]() mutable {
#ifndef NDEBUG
                if (not coro) {
                    std::cout << "What happened to my coro?" << std::endl;
                    return;
                }
#endif
                coro.resume();
            });
        } else {
#ifndef NDEBUG
            std::cout << "Somebody wanted to resume a NULL coro" << std::endl;
#endif
        }
    }


}
