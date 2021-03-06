/**
    Copyright 2020 Red Anchor Trading Co. Ltd.

    Distributed under the Boost Software License, Version 1.0.
    See <http://www.boost.org/LICENSE_1_0.txt>
 */


#pragma once


#include <iostream>

#include <f5/makham/executor.hpp>

#include <atomic>
#include <mutex>
#include <optional>
#include <vector>

#ifdef MAKHAM_STDOUT_TRACE
#include <iostream>
#endif


namespace f5::makham {

    /// ## Multi-awaitable
    /**
     * A wrapper for an awaitable that allows us to have more than one
     * client awaiting.
     */
    template<
            typename A,
            typename R = decltype(
                    std::declval<typename A::promise_type>().get_value())>
    class multi final {
        using result_type = R;
        std::optional<result_type> *pvalue;

        struct wrapper {
            struct promise_type;
            using handle_type = coroutine_handle<promise_type>;

            handle_type coro;

            wrapper(handle_type h) : coro{h} {}
            wrapper(wrapper const &w) : coro(w.coro) {
                ++coro.promise().wraps;
#ifdef MAKHAM_STDOUT_TRACE
                std::cout << "wrapper copied" << std::endl;
#endif
            }
            wrapper(wrapper &&w) : coro{std::exchange(w.coro, {})} {
#ifdef MAKHAM_STDOUT_TRACE
                std::cout << "wrapper moved" << std::endl;
#endif
            }
            ~wrapper() {
                if (coro) {
                    auto const count = --coro.promise().wraps;
                    if (not count) {
#ifdef MAKHAM_STDOUT_TRACE
                        std::cout << "wrapper destructed -- taking promise"
                                  << std::endl;
#endif
                        coro.destroy();
#ifdef MAKHAM_STDOUT_TRACE
                    } else {
                        std::cout << "wrapper destructed -- promise survives"
                                  << std::endl;
#endif
                    }
#ifdef MAKHAM_STDOUT_TRACE
                } else {
                    std::cout << "wrapper destructed -- only move husk left"
                              << std::endl;
#endif
                }
            }

            struct promise_type {
                /// Counter for the number of wrappers
                std::atomic<std::size_t> wraps = 1u;

                /// Used to signal that the wrapped coroutine is ready
                std::atomic<bool> ready = false;
                result_type value;

                /// Used for overspill only
                std::mutex bottleneck;
                std::vector<coroutine_handle<>> overspill;

                /// Enqueue if the result isn't in yet, otherwise we just resume
                /// the handle.
                void enqueue(coroutine_handle<> h) {
                    if (ready) {
#ifdef MAKHAM_STDOUT_TRACE
                        std::cout << "Enqueued ready, just resume" << std::endl;
#endif
                        /// The result is already there, so we can just resume
                        post(h);
                    } else {
                        {
#ifdef MAKHAM_STDOUT_TRACE
                            std::cout << "Enqueued not ready, listing"
                                      << std::endl;
#endif
                            std::lock_guard<std::mutex> lock{bottleneck};
                            overspill.push_back(h);
                        }
                        if (ready) {
#ifdef MAKHAM_STDOUT_TRACE
                            std::cout << "Enqueued, but, oops, now ready!"
                                      << std::endl;
#endif
                            resume();
                        }
                    }
                }
                /// Calls to this need to be done very carefully
                void resume() {
                    std::lock_guard<std::mutex> lock{bottleneck};
                    for (auto &h : overspill) {
#ifdef MAKHAM_STDOUT_TRACE
                        std::cout << "Resuming awaiting coro" << std::endl;
#endif
                        post(h);
                    }
                    overspill.clear();
                };

                /// Coroutine promise API
                auto get_return_object() {
#ifdef MAKHAM_STDOUT_TRACE
                    std::cout << "Created new wrapper instance" << std::endl;
#endif
                    return wrapper{handle_type::from_promise(*this)};
                }
                auto initial_suspend() { return suspend_never{}; }
                auto final_suspend() {
#ifdef MAKHAM_STDOUT_TRACE
                    std::cout << "Stopping at end of wrapper" << std::endl;
#endif
                    return suspend_always{};
                }
                void unhandled_exception() {
                    // TODO Handle the exception and pass it on
                    std::exit(57);
                }
                auto return_void() {
#ifdef MAKHAM_STDOUT_TRACE
                    std::cout << "Escaped wrapper::create" << std::endl;
#endif
                    /// Block new enqueues
                    if (ready.exchange(true)) {
                        /// Already returned. This is bad
                        throw std::runtime_error{
                                "Coroutine has already been flushed"};
                    }
                    resume();
                    /// We need to do this a second time because it is possible
                    /// that another handle arrived in the overspill after we
                    /// set `ready`
                    resume();
                    return suspend_never{};
                }
            };
            static wrapper create(multi *m, A a) {
                std::optional<result_type> r;
                m->pvalue = &r;
#ifdef MAKHAM_STDOUT_TRACE
                std::cout << "Set pvalue address" << std::endl;
#endif
                r = co_await a;
#ifdef MAKHAM_STDOUT_TRACE
                std::cout << "Set value" << std::endl;
#endif
                co_return;
            }
        } wrapped;
        friend struct wrapper;

      public:
        multi(A &&a) : wrapped{wrapper::create(this, std::move(a))} {}
        multi(multi const &m) : pvalue{m.pvalue}, wrapped{m.wrapped} {
#ifdef MAKHAM_STDOUT_TRACE
            std::cout << "Multi copied" << std::endl;
#endif
        }
        ~multi() {
#ifdef MAKHAM_STDOUT_TRACE
            std::cout << "multi destructed" << std::endl;
#endif
        }
        /// Awaitable
        bool await_ready() const { return false; }
        void await_suspend(coroutine_handle<> awaiting) {
#ifdef MAKHAM_STDOUT_TRACE
            std::cout << "Enqueuing awaiting" << std::endl;
#endif
            wrapped.coro.promise().enqueue(awaiting);
        }
        result_type await_resume() {
#ifdef MAKHAM_STDOUT_TRACE
            std::cout << "Pulling value" << std::endl;
#endif
            return **pvalue;
        }
    };


}
