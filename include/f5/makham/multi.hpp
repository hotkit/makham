/**
    Copyright 2020 Red Anchor Trading Co. Ltd.

    Distributed under the Boost Software License, Version 1.0.
    See <http://www.boost.org/LICENSE_1_0.txt>
 */


#pragma once


#include <f5/makham/executor.hpp>

#include <atomic>
#include <mutex>
#include <vector>


namespace f5::makham {

    /// ## Multi-awaitable
    /**
     * A wrapper for an awaitable that allows us to have more than one
     * client awaiting.
     */
    template<typename A>
    class multi final {
        struct wrapper {
            struct promise_type;
            using handle_type =
                    std::experimental::coroutine_handle<promise_type>;

            handle_type coro;
            wrapper(handle_type h) : coro{h} {}

            struct promise_type {
                /// Counter for the number of wrappers
                std::atomic<std::size_t> wraps;

                /// Used for overspill only
                std::mutex bottleneck;
                std::vector<std::experimental::coroutine_handle<>> overspill;

                /// Enqueue if the result isn't in yet, otherwise we just resume
                /// the handle.
                void enqueue(std::experimental::coroutine_handle<> h) {
                    std::lock_guard<std::mutex> lock{bottleneck};
                    overspill.push_back(h);
                }

                /// Coroutine promise API
                auto get_return_object() {
                    return wrapper{handle_type::from_promise(*this)};
                }
                auto initial_suspend() {
                    return std::experimental::suspend_never{};
                }
                auto final_suspend() {
                    return std::experimental::suspend_always{};
                }
                void unhandled_exception() { std::exit(57); }

                /// Flush the queue
                auto return_void() {
                    return std::experimental::suspend_never{};
                }
            };
            static wrapper create(A &&a) {
                auto r = co_await a;
                co_return;
            }
        } wrapped;

      public:
        multi(A &&a) : wrapped{wrapper::create(std::move(a))} {}

        /// Awaitable
        bool await_ready() const { return false; }
        void await_suspend(std::experimental::coroutine_handle<> awaiting) {
            wrapped.coro.promise().enqueue(awaiting);
        }
        auto await_resume() {}
    };


}
