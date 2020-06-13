/**
    Copyright 2020 Red Anchor Trading Co. Ltd.

    Distributed under the Boost Software License, Version 1.0.
    See <http://www.boost.org/LICENSE_1_0.txt>
 */


#include <fost/test>
#include <f5/makham/generator.hpp>


namespace {


    f5::makham::generator<std::size_t> fib() {
        std::size_t a{1}, b{1};
        co_yield 1u;
        while (true) { co_yield a = std::exchange(b, a + b); }
    }

    f5::makham::generator<std::size_t> thrower(bool after_yield) {
        if (not after_yield)
            throw std::runtime_error{"Ooops, something went wrong"};
        co_yield 1;
        throw std::runtime_error{"Ooops, something went wrong after yield"};
    }

    template<typename T>
    f5::makham::generator<T>
            take(std::size_t number, f5::makham::generator<T> gen) {
        for (auto pos{gen.begin()}; number--; ++pos) { co_yield *pos; }
    }


}


FSL_TEST_SUITE(generator);


FSL_TEST_FUNCTION(fibonacci) {
    auto f = fib();
    auto pos = f.begin();
    FSL_CHECK_EQ(*pos, 1u);
    FSL_CHECK_EQ(*++pos, 1u);
    FSL_CHECK_EQ(*++pos, 2u);
    FSL_CHECK_EQ(*++pos, 3u);
    FSL_CHECK_EQ(*++pos, 5u);
    FSL_CHECK_EQ(*++pos, 8u);
}


FSL_TEST_FUNCTION(throws_early) {
    auto f = thrower(false);
    FSL_CHECK_EXCEPTION(f.begin(), std::runtime_error &);
}


FSL_TEST_FUNCTION(throws_late) {
    auto f = thrower(true);
    auto pos = f.begin();
    FSL_CHECK_EQ(*pos, 1u);
    FSL_CHECK_EXCEPTION(++pos, std::runtime_error &);
}


FSL_TEST_FUNCTION(terminates) {
    std::vector<std::size_t> fibs{};
    for (auto f : take(10, fib())) { fibs.push_back(f); }
    FSL_CHECK_EQ(fibs.size(), 10u);
    FSL_CHECK_EQ(fibs.front(), 1u);
    FSL_CHECK_EQ(fibs.back(), 55u);
}
