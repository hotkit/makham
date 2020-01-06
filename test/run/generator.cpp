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
        co_yield a;
        co_yield b;
        while (true) { co_yield a = std::exchange(b, a + b); }
    }


}


FSL_TEST_SUITE(generator);


FSL_TEST_FUNCTION(fibonacci) {
    std::cout << "\n\nStarting fibonacci" << std::endl;
    auto f = fib();
    auto pos = f.begin();
    FSL_CHECK_EQ(*pos, 1u);
}
