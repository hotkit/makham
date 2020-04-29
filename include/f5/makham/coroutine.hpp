/**
    Copyright 2020 Red Anchor Trading Co. Ltd.

    Distributed under the Boost Software License, Version 1.0.
    See <http://www.boost.org/LICENSE_1_0.txt>
 */


#pragma once


#if __has_include(<coroutine>)
#include <coroutine>
namespace f5::makham {
    template<typename T = void>
    using coroutine_handle = std::coroutine_handle<T>;
    using suspend_always = std::suspend_always;
    using suspend_never = std::suspend_never;
}
/**
Super bad idea, but the sort of thing that would be needed to make
clang work with libstdc++ as clang seems to be hard coded to look
in the experimental namespace for things.
```cpp
namespace std::experimental {
    template<typename T = void>
    using coroutine_handle = std::coroutine_handle<T>;
    template<typename... Ts>
    using coroutine_traits = std::coroutine_traits<Ts...>;
    using suspend_always = std::suspend_always;
    using suspend_never = std::suspend_never;
}
```
*/
#else
#include <experimental/coroutine>
namespace f5::makham {
    template<typename T = void>
    using coroutine_handle = std::experimental::coroutine_handle<T>;
    using suspend_always = std::experimental::suspend_always;
    using suspend_never = std::experimental::suspend_never;
}
#endif
