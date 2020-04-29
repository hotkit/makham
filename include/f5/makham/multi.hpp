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


namespace f5::makham {

    /// ## Multi-awaitable
    /**
     * A wrapper for an awaitable that allows us to have more than one
     * client awaiting.
     */
    template<typename A>
    class multi final {
        using result_type =
                decltype(std::declval<typename A::promise_type>().get_value());
        std::optional<result_type> *pvalue;

        struct wrapper {
            struct promise_type;
            using handle_type = coroutine_handle<promise_type>;

            handle_type coro;

            wrapper(handle_type h) : coro{h} {}
            wrapper(wrapper const &w) : coro(w.coro) {
                ++coro.promise().wraps;
                std::cout << "wrapper copied" << std::endl;
            }
            wrapper(wrapper &&w) : coro{std::exchange(w.coro, {})} {
                std::cout << "wrapper moved" << std::endl;
            }
            ~wrapper() {
                if (coro) {
                    auto const count = --coro.promise().wraps;
                    if (not count) {
                        std::cout << "wrapper destructed -- taking promise"
                                  << std::endl;
                        coro.destroy();
                    } else {
                        std::cout << "wrapper destructed -- promise survives"
                                  << std::endl;
                    }
                } else {
                    std::cout << "wrapper destructed -- only move husk left"
                              << std::endl;
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
                        std::cout << "Enqueued ready, just resume" << std::endl;
                        /// The result is already there, so we can just resume
                        post(h);
                    } else {
                        {
                            std::cout << "Enqueued not ready, listing"
                                      << std::endl;
                            std::lock_guard<std::mutex> lock{bottleneck};
                            overspill.push_back(h);
                        }
                        if (ready) {
                            std::cout << "Enqueued, but, oops, now ready!"
                                      << std::endl;
                            resume();
                        }
                    }
                }
                /// Calls to this need to be done very carefully
                void resume() {
                    std::lock_guard<std::mutex> lock{bottleneck};
                    for (auto &h : overspill) {
                        std::cout << "Resuming awaiting coro" << std::endl;
                        post(h);
                    }
                    overspill.clear();
                };

                /// Coroutine promise API
                auto get_return_object() {
                    std::cout << "Created new wrapper instance" << std::endl;
                    return wrapper{handle_type::from_promise(*this)};
                }
                auto initial_suspend() { return suspend_never{}; }
                auto final_suspend() {
                    std::cout << "Stopping at end of wrapper" << std::endl;
                    return suspend_always{};
                }
                void unhandled_exception() {
                    // TODO Handle the exception and pass it on
                    std::exit(57);
                }
                auto return_void() {
                    std::cout << "Escaped wrapper::create" << std::endl;
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
                std::cout << "Set pvalue address" << std::endl;
                r = co_await a;
                std::cout << "Set value" << std::endl;
                co_return;
            }
        } wrapped;
        friend struct wrapper;

      public:
        multi(A &&a) : wrapped{wrapper::create(this, std::move(a))} {}
        multi(multi const &m) : pvalue{m.pvalue}, wrapped{m.wrapped} {
            std::cout << "Multi copied" << std::endl;
        }
        ~multi() { std::cout << "multi destructed" << std::endl; }

        /// Awaitable
        bool await_ready() const { return false; }
        void await_suspend(coroutine_handle<> awaiting) {
            std::cout << "Enqueuing awaiting" << std::endl;
            wrapped.coro.promise().enqueue(awaiting);
        }
        result_type await_resume() {
            std::cout << "Pulling value" << std::endl;
            return **pvalue;
        }
    };


}
