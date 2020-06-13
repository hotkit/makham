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

        generator(handle_type h) : coro(h) {
#ifdef MAKHAM_STDOUT_TRACE
            std::cout << "Created a generator" << std::endl;
#endif
        }

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
            handle_type coro;

            iterator() : coro{} {}
            iterator(generator *s) : coro{std::exchange(s->coro, {})} {
#ifdef MAKHAM_STDOUT_TRACE
                std::cout << "Created an iterator" << std::endl;
#endif
                coro.resume();
                throw_if_needed();
            }

            void throw_if_needed() {
                if (coro.promise().eptr) {
                    std::rethrow_exception(coro.promise().eptr);
                }
            }

          public:
            iterator(iterator const &) = delete;
            iterator &operator=(iterator const &) = delete;

            ~iterator() {
                if (coro) { coro.destroy(); }
            }

            Y operator*() {
                return std::exchange(coro.promise().value, {}).value();
            }

            auto &operator++() {
                coro.resume();
                throw_if_needed();
                if (not coro.promise().value) {
                    std::exchange(coro, {}).destroy();
                }
                return *this;
            }

            friend bool operator==(iterator const &l, iterator const &r) {
                if (not l.coro && not r.coro) {
                    return true;
                } else {
                    return false;
                }
            }
            friend bool operator!=(iterator const &l, iterator const &r) {
                return not(l == r);
            }
        };
        auto begin() { return iterator{this}; }
        auto end() { return iterator{}; }
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
