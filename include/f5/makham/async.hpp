/**
    Copyright 2019-2020 Red Anchor Trading Co. Ltd.

    Distributed under the Boost Software License, Version 1.0.
    See <http://www.boost.org/LICENSE_1_0.txt>
 */


#pragma once


#include <f5/makham/executor.hpp>

#include <atomic>
#include <chrono>
#include <thread>
#include <variant>

#ifndef NDEBUG
#include <iostream>
#endif


namespace f5 {
    namespace detail {
        template<typename... Fs>
        struct visitor_overload : std::remove_reference_t<Fs>... {
            visitor_overload(Fs &&... fs)
            : std::remove_reference_t<Fs>{std::forward<Fs>(fs)}... {}
            using std::remove_reference_t<Fs>::operator()...;
        };
    }
    template<typename V, typename... T>
    decltype(auto) apply_visitor(V &&v, T &&... t) {
        return std::visit(
                detail::visitor_overload<T...>(std::forward<T>(t)...),
                std::forward<V>(v));
    }
}


namespace f5::makham {


    /// Forward declarations
    template<typename R>
    struct promise_type;
    template<typename R, typename P = promise_type<R>>
    class async;


    /// ## Async
    /**
     * A async whose completion can be awaited..
     */
    template<typename R, typename P>
    class async final {
        bool awaited = false;

      public:
        using promise_type = P;
        friend promise_type;

        /// Not copyable
        async(async const &) = delete;
        async &operator=(async const &) = delete;
        /// Movable
        async(async &&t) noexcept : coro(std::exchange(t.coro, {})) {
#ifndef NDEBUG
            std::cout << "Move construct async" << std::endl;
#endif
        }
        async &operator=(async &&t) noexcept {
            if (coro) {
#ifndef NDEBUG
                std::cout
                        << "Moved assign async & found and old coro to destroy"
                        << std::endl;
#endif
                coro.destroy();
#ifndef NDEBUG
            } else {
                std::cout << "Moved assign async" << std::endl;
#endif
            }
            coro = std::exchange(t.coro, {});
        }
        ~async() {
            // TODO If there has been no co_await we must wait before
            // we destroy this thing...
            if (coro) {
#ifndef NDEBUG
                if (not awaited) {
                    std::cout << "Async not awaited !!!!" << std::endl;
                }
                std::cout << "Async destructed, and taking the promise with it"
                          << std::endl;
#endif
                coro.destroy();
#ifndef NDEBUG
            } else {
                std::cout << "Async destructed, but no coro" << std::endl;
#endif
            }
        }

        /// ### Awaitable
        bool await_ready() const { return false; }
        void await_suspend(coroutine_handle<> awaiting) {
#ifndef NDEBUG
            std::cout << "Async will signal another coroutine" << std::endl;
#endif
            coro.promise().signal(awaiting);
        }
        R await_resume() {
#ifndef NDEBUG
            std::cout << "Async returning co_awaited value" << std::endl;
#endif
            awaited = true;
            return coro.promise().get_value();
        }

      private:
        using handle_type = coroutine_handle<promise_type>;
        handle_type coro;

        async(handle_type c) : coro{c} {
#ifndef NDEBUG
            std::cout << "Async starting" << std::endl;
#endif
            post(coro);
        }
    };


    /// An asynchronous promise
    struct async_promise {
        std::atomic<bool> has_value = false;
        std::atomic<coroutine_handle<>> continuation = {};

        void continuation_if_not_run() {
            if (auto h = continuation.exchange({}); h) {
#ifndef NDEBUG
                std::cout << "Async continuation starting" << std::endl;
#endif
                post(h);
#ifndef NDEBUG
            } else {
                std::cout << "Async no continuation found to start yet"
                          << std::endl;
#endif
            }
        }
        void signal(coroutine_handle<> s) {
            auto const old = continuation.exchange(s);
            if (old) {
#ifndef NDEBUG
                std::cout << "Async signal throwing" << std::endl;
#endif
                throw std::invalid_argument{
                        "An async can only have one awaitable"};
            }
            if (has_value.exchange(false)) {
                continuation_if_not_run();
#ifndef NDEBUG
            } else {
                std::cout << "Async value not available, continuation has been "
                             "set"
                          << std::endl;
#endif
            }
        }
        void value_has_been_set() {
#ifndef NDEBUG
            std::cout << "Async value now set" << std::endl;
#endif
            if (has_value.exchange(true)) {
                throw std::runtime_error{"Coroutine already had a value set"};
            }
            continuation_if_not_run();
        }
        auto initial_suspend() { return suspend_always{}; }
        auto final_suspend() { return suspend_always{}; }
    };


    /// ## The async promise type
    template<typename R>
    struct promise_type final : public async_promise {
        using async_type = async<R, promise_type<R>>;
        using handle_type = coroutine_handle<promise_type<R>>;

        std::variant<std::monostate, std::exception_ptr, R> value = {};

        auto get_return_object() {
            return async_type{handle_type::from_promise(*this)};
        }
        auto return_value(R v) {
#ifndef NDEBUG
            std::cout << "Async co_returned value" << std::endl;
#endif
            value = std::move(v);
            value_has_been_set();
            return suspend_never{};
        }
        void unhandled_exception() {
#ifndef NDEBUG
            std::cout << "Async exception caught" << std::endl;
#endif
            value = std::current_exception();
            value_has_been_set();
        }

        R get_value() {
            return apply_visitor(
                    std::move(value),
                    [](std::monostate) -> R {
                        throw std::runtime_error(
                                "The coroutine doesn't have a value");
                    },
                    [](std::exception_ptr e) -> R { std::rethrow_exception(e); },
                    [](R v) { return v; });
        }
    };

    template<>
    struct promise_type<void> final : public async_promise {
        using async_type = async<void, promise_type<void>>;
        using handle_type = coroutine_handle<promise_type<void>>;

        std::exception_ptr value;

        auto get_return_object() {
            return async_type{handle_type::from_promise(*this)};
        }
        auto return_void() {
#ifndef NDEBUG
            std::cout << "Async co_returned void" << std::endl;
#endif
            value_has_been_set();
            return suspend_never{};
        }
        void unhandled_exception() {
#ifndef NDEBUG
            std::cout << "Async exception caught" << std::endl;
#endif
            value = std::current_exception();
            value_has_been_set();
        }

        void get_value() {
            if (value) std::rethrow_exception(value);
        }
    };


}
