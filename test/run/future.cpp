/**
    Copyright 2019-2020 Red Anchor Trading Co. Ltd.

    Distributed under the Boost Software License, Version 1.0.
    See <http://www.boost.org/LICENSE_1_0.txt>
 */


#include <fost/test>
#include <f5/makham/async.hpp>
#include <f5/makham/future.hpp>


namespace {
    f5::makham::async<int> answer() { co_return 42; }

    f5::makham::async<unsigned> fib(unsigned n) {
        if (n < 3u) {
            co_return 1;
        } else {
            auto a = fib(n - 1u);
            auto b = fib(n - 2u);
            co_return co_await a + co_await b;
        }
    }

    std::atomic<bool> did_nothing;
    f5::makham::async<void> nothing() {
        did_nothing.store(true);
        co_return;
    }
}


FSL_TEST_SUITE(makham_future);


FSL_TEST_FUNCTION(get_easy) {
    auto f = []() -> f5::makham::future<int> { co_return 42; };
    FSL_CHECK_EQ(f().get(), 42);
}


FSL_TEST_FUNCTION(get_with_await) {
    // FIXME With the lambda below rather than `wrap` there are
    // segfaults when running. The `async` can be destructed twice
    //     auto f = []() -> f5::makham::future<int> { co_return co_await
    //     answer(); };
    FSL_CHECK_EQ(f5::makham::future<int>::wrap(answer()).get(), 42);
}


FSL_TEST_FUNCTION(seq_fibonacci) {
    // FSL_CHECK_EQ(f5::makham::future<unsigned>::wrap(fib(10u)).get(), 55u);
}


FSL_TEST_FUNCTION(with_nothing) {
    did_nothing.store(false);
    f5::makham::future<void>::wrap(nothing()).get();
    FSL_CHECK(did_nothing);
}
