[![macos](https://github.com/VKCOM/kphp/actions/workflows/macos.yml/badge.svg?branch=master)](https://github.com/VKCOM/kphp/actions/workflows/macos.yml)
[![debian](https://github.com/VKCOM/kphp/actions/workflows/debian.yml/badge.svg?branch=master)](https://github.com/VKCOM/kphp/actions/workflows/debian.yml)
[![ubuntu](https://github.com/VKCOM/kphp/actions/workflows/ubuntu.yml/badge.svg?branch=master)](https://github.com/VKCOM/kphp/actions/workflows/ubuntu.yml)

# KPHP — a PHP compiler

KPHP is a PHP compiler. It compiles a limited subset of PHP to a native binary running faster than PHP.

KPHP was developed at VK.com and maintained as proprietary for years — until open-sourced in late 2020.

[**Visit the KPHP website with documentation, demos, etc.**](https://vkcom.github.io/kphp/)


## Limitations

KPHP wouldn't compile just any random PHP code:

* It doesn't support features that can't be compiled, such as calling by name or mocks.
* It won't compile code, that breaks the type system, for instance, mixing numbers and objects in an array.
* It doesn't have PHP features that VK.com never had a need for, such as SPL classes and XML parsing.
* Some PHP syntax details just weren't implemented, like generators and anonymous classes.

Read more on this here: [KPHP vs PHP differences](https://vkcom.github.io/kphp/kphp-language/kphp-vs-php/kphp-vs-php-differences.html).


## Features over PHP

KPHP analyzes your code as a whole and performs various optimizations focusing on performance and safety:

* Inferring [types of all variables](https://vkcom.github.io/kphp/kphp-language/static-type-system/type-inferring.html), how to declare them in C++.
* Compile-time optimizations, such as inlining getters or reducing refcounters flapping.
* Compile-time checks, including immutability and [type system requirements](https://vkcom.github.io/kphp/kphp-language/static-type-system/kphp-type-system.html).
* Runtime optimizations, like constant arrays pre-initing and typed vectors.

Aside from the above, KPHP has [coroutines](https://vkcom.github.io/kphp/kphp-language/best-practices/async-programming-forks.html). For now, however, they are almost inapplicable outside of VK code.


## Benchmarks 

Generally, when your code fits best practices, it runs **3–10 times faster than PHP**.

Take a look at the [**benchmarks page**](https://vkcom.github.io/kphp/kphp-language/kphp-vs-php/benchmarks.html) comparing KPHP and PHP performance.  
You can also refer to [**PHP vs KPHP vs C++**](https://vkcom.github.io/kphp/various-topics/walk-through-php-kphp-cpp.html).

KPHP isn't *always* faster than PHP, but it can be used to speed up your code by focusing on strict typing and KPHP built-in functions.   


## "I tried to compile PHP code, but failed"

This situation is quite common. KPHP rarely compiles already existing code without errors. It usually takes some time to rewrite PHP code for it to be compilable with KPHP.

Read more about this in [compiling an existing project](https://vkcom.github.io/kphp/kphp-basics/compile-existing-project.html).


## The License

KPHP is distributed under the GPLv3 license, on behalf of VK.com (V Kontakte LLC).


## Ask questions and provide feedback

To communicate with KPHP community, use GitHub issues or a [Telegram chat](https://t.me/kphp_chat).

You can also take a look at our [FAQ page](https://vkcom.github.io/kphp/kphp-basics/faq.html) and [Roadmap page](https://vkcom.github.io/kphp/kphp-basics/roadmap.html).


## Contributing

Please, refer to the [Contributing page](https://vkcom.github.io/kphp/kphp-internals/developing-and-extending-kphp/contributing-to-kphp.html).


## How do I start using KPHP?

Proceed to the [installation page](https://vkcom.github.io/kphp/kphp-basics/installation.html) and just follow the left menu.
