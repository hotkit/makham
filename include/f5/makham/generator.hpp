/**
    Copyright 2019-2020 Red Anchor Trading Co. Ltd.

    Distributed under the Boost Software License, Version 1.0.
    See <http://www.boost.org/LICENSE_1_0.txt>
 */


#pragma once


#include <iostream>

#include <f5/makham/coroutine.hpp>
#include <optional>

#ifdef MAKHAM_STDOUT_TRACE
#include <iostream>
#endif


namespace f5::makham {


    template<typename Y>
    struct generator_promise;


    template<typename Y>
    class generator final {
        friend struct generator_promise<Y>;
        using handle_type = typename generator_promise<Y>::handle_type;
        handle_type coro;

        generator(handle_type h) : coro(h) {}

      public:
        using promise_type = generator_promise<Y>;

        /// Not copyable
        generator(generator const &) = delete;
        generator &operator=(generator const &) = delete;
        /// Movable
        generator(generator &&t) noexcept : coro(std::exchange(t.coro, {})) {}
        generator &operator=(generator &&t) noexcept {
            coro = std::exchange(t.coro, {});
        }
        ~generator() {
            if (coro) coro.destroy();
        }

        /// Iteration
        class iterator {
            friend class generator;
            generator *pseq;
            iterator(generator *s) : pseq{s} { pseq->coro.resume(); }

          public:
            Y operator*() {
                // TODO Check for exception to throw
                return std::exchange(pseq->coro.promise().value, {}).value();
            }
        };
        auto begin() { return iterator{this}; }
    };


    template<typename Y>
    struct generator_promise {
        std::optional<Y> value = {};
        std::exception_ptr eptr = {};

        using handle_type = coroutine_handle<generator_promise>;

        auto yield_value(Y y) {
#ifdef MAKHAM_STDOUT_TRACE
            std::cout << "Got a value from a co_yield" << std::endl;
#endif
            value = std::move(y);
            return suspend_always{};
        }
        void unhandled_exception() { eptr = std::current_exception(); }
        auto return_void() {
#ifdef MAKHAM_STDOUT_TRACE
            std::cout << "generator ended" << std::endl;
#endif
            value = {};
            return suspend_never{};
        }

        auto get_return_object() {
            return generator<Y>{handle_type::from_promise(*this)};
        }
        auto initial_suspend() { return suspend_always{}; }
        auto final_suspend() { return suspend_always{}; }
    };


}
