/**
    Copyright 2019-2020 Red Anchor Trading Co. Ltd.

    Distributed under the Boost Software License, Version 1.0.
    See <http://www.boost.org/LICENSE_1_0.txt>
 */


#pragma once


#include <f5/makham/executor.hpp>

#include <future>

#ifdef MAKHAM_STDOUT_TRACE
#include <iostream>
#endif


namespace f5::makham {


    template<typename R>
    struct future_promise;


    /// ## Future
    /**
     * Spawns a coroutine which will run asynchronously and whose result can
     * be fetched from a `std::future. This can be used to bridge from a thread
     * running outside of the coroutine executor to a coroutine, for example
     * from `main`.
     *
     * The calling thread will suspend until the result is ready, so this must
     * never be used between coroutines as the thread waiting on the future
     * will not become available to run other coroutines. This can result in a
     * deadlock at worst, but at best will be very inefficient.
     */
    template<typename R>
    class future final {
        std::atomic<bool> gotten = false;

      public:
        using promise_type = future_promise<R>;

        /// Not copyable
        future(future const &) = delete;
        future &operator=(future const &) = delete;
        /// Movable
        future(future &&t) noexcept : coro(std::exchange(t.coro, {})) {
#ifdef MAKHAM_STDOUT_TRACE
            std::cout << "Future move construct future" << std::endl;
#endif
        }
        future &operator=(future &&t) noexcept {
#ifdef MAKHAM_STDOUT_TRACE
            std::cout << "Future move assign future" << std::endl;
#endif
            coro = std::exchange(t.coro, {});
        }
        ~future() {
            // TODO If there has been no get() we must wait before
            // we destroy this thing...
            if (coro) {
#ifdef MAKHAM_STDOUT_TRACE
                if (not gotten) {
                    std::cout << "Future not got() !!!!" << std::endl;
                }
                std::cout << "Future destructed -- taking the promise with it"
                          << std::endl;
#endif
                coro.destroy();
#ifdef MAKHAM_STDOUT_TRACE
            } else {
                std::cout << "Future destructed -- no coro" << std::endl;
#endif
            }
        }

        R get() {
            gotten = true;
            return coro.promise().fp.get_future().get();
        }

        /// Wrap an awaitable and return a future to its result
        template<typename C>
        static future wrap(C c) {
            co_return co_await c;
        }

      private:
        friend promise_type;
        typename promise_type::handle_type coro;

        future(typename promise_type::handle_type h) : coro(h) {
#ifdef MAKHAM_STDOUT_TRACE
            std::cout << "Future constructed" << std::endl;
#endif
            post(coro);
        }
    };


    template<typename R>
    struct future_promise final {
        using handle_type = coroutine_handle<future_promise>;

        std::promise<R> fp = {};

        auto get_return_object() {
            return future<R>{handle_type::from_promise(*this)};
        }
        auto return_value(R v) {
#ifdef MAKHAM_STDOUT_TRACE
            std::cout << "Future co_returned value" << std::endl;
#endif
            fp.set_value(std::move(v));
            return suspend_never{};
        }
        void unhandled_exception() {
#ifdef MAKHAM_STDOUT_TRACE
            std::cout << "Future exception caught" << std::endl;
#endif
            fp.set_exception(std::current_exception());
        }

        auto initial_suspend() { return suspend_always{}; }
        auto final_suspend() { return suspend_always{}; }
    };
    template<>
    struct future_promise<void> final {
        using handle_type = coroutine_handle<future_promise>;

        std::promise<void> fp = {};

        auto get_return_object() {
            return future<void>{handle_type::from_promise(*this)};
        }
        auto return_void() {
#ifdef MAKHAM_STDOUT_TRACE
            std::cout << "Future co_returned void" << std::endl;
#endif
            fp.set_value();
            return suspend_never{};
        }
        void unhandled_exception() {
#ifdef MAKHAM_STDOUT_TRACE
            std::cout << "Future exception caught" << std::endl;
#endif
            fp.set_exception(std::current_exception());
        }

        auto initial_suspend() { return suspend_always{}; }
        auto final_suspend() { return suspend_always{}; }
    };


}
