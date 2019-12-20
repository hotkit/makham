/**
    Copyright 2019 Red Anchor Trading Co. Ltd.

    Distributed under the Boost Software License, Version 1.0.
    See <http://www.boost.org/LICENSE_1_0.txt>
 */


#pragma once


#include <f5/makham/executor.hpp>

#include <future>


namespace f5::makham {


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
      public:
        ~future() {
            if (coro) coro.destroy();
        }

        /// Not copyable
        future(future const &) = delete;
        future &operator=(future const &) = delete;
        /// Movable
        future(future &&t) noexcept : coro(std::exchange(t.coro, {})) {}
        future &operator=(future &&t) noexcept {
            coro = std::exchange(t.coro, {});
        }

        struct promise_type final {
            using handle_type =
                    std::experimental::coroutine_handle<promise_type>;

            std::promise<R> fp = {};

            auto get_return_object() {
                return future{handle_type::from_promise(*this)};
            }
            auto return_value(R v) {
                fp.set_value(std::move(v));
                return std::experimental::suspend_never{};
            }
            void unhandled_exception() {
                fp.set_exception(std::current_exception());
            }

            auto initial_suspend() {
                return std::experimental::suspend_always{};
            }
            auto final_suspend() { return std::experimental::suspend_always{}; }
        };

        R get() { return coro.promise().fp.get_future().get(); }

      private:
        friend promise_type;
        typename promise_type::handle_type coro;

        future(typename promise_type::handle_type h) : coro(h) {
            makham::post(coro);
        }
    };


}
