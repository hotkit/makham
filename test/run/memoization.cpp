/**
    Copyright 2020 Red Anchor Trading Co. Ltd.

    Distributed under the Boost Software License, Version 1.0.
    See <http://www.boost.org/LICENSE_1_0.txt>
 */


#include <fost/test>
#include <f5/makham/async.hpp>
#include <f5/makham/future.hpp>
#include <f5/makham/multi.hpp>


namespace {
    f5::makham::async<int> answer() { co_return 42; }
}


FSL_TEST_SUITE(memoization);


FSL_TEST_FUNCTION(awaiting) {
    f5::makham::multi<f5::makham::async<int>> a{answer()};

    f5::makham::future<void>::wrap(a).get();
}
