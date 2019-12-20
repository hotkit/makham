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
}


FSL_TEST_SUITE(makham_future);


FSL_TEST_FUNCTION(get) {
    auto f = []() -> f5::makham::future<int> {
        auto a = answer();
        auto const v = co_await a;
        co_return v;
    };
    FSL_CHECK_EQ(f().get(), 42);
}
