/**
    Copyright 2019 Red Anchor Trading Co. Ltd.

    Distributed under the Boost Software License, Version 1.0.
    See <http://www.boost.org/LICENSE_1_0.txt>
 */


#include <fost/test>
#include <f5/makham/async.hpp>
#include <f5/makham/future.hpp>


namespace {
    f5::makham::async<int> answer() { co_return 42; }
    f5::makham::async<unsigned> fib(unsigned n) {
        if (n < 3u)
            co_return 1;
        else
            co_return co_await fib(n - 1u) + co_await fib(n - 2u);
    }
}


FSL_TEST_SUITE(makham_future);


FSL_TEST_FUNCTION(get_easy) {
    auto f = []() -> f5::makham::future<int> { co_return 42; };
    FSL_CHECK_EQ(f().get(), 42);
}


FSL_TEST_FUNCTION(get_with_await) {
    auto f = []() -> f5::makham::future<int> { co_return co_await answer(); };
    FSL_CHECK_EQ(f().get(), 42);
}


FSL_TEST_FUNCTION(seq_fibonacci) {
    FSL_CHECK_EQ(f5::makham::future<unsigned>::wrap(fib(10u)).get(), 55u);
}
