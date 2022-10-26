---
title: KPHP — a PHP compiler
permalink: /
notopnavigation: true
---

# KPHP — a PHP compiler

<a href="{{ site.url_github_kphp }}" class="btn-github-page">
  <span class="icon icon-github"></span>
  <span>GitHub page</span>
</a>

KPHP is a PHP compiler. It compiles a limited subset of PHP to a native binary running faster than PHP.

KPHP takes your PHP source code and converts it to a **C++ equivalent**, then compiles the generated C++ code and runs it within an embedded HTTP server. You could call KPHP *a transpiler*, but we call it *a compiler*.

KPHP is not JIT-oriented: all types are inferred at compile time. It has no "slow startup" phase.

```note
KPHP was developed at VK.com and maintained as proprietary for years — until open-sourced in late 2020.
```


## Limitations

KPHP wouldn't compile just any random PHP code:

* It doesn't support features that can't be compiled, such as calling by name or mocks.
* It won't compile code, that breaks the type system, for instance, mixing numbers and objects in an array.
* It doesn't have PHP features that VK.com never had a need for, such as SPL classes and XML parsing.
* Some PHP syntax details just weren't implemented, like generators and anonymous classes.

You can read more on this here: [KPHP vs PHP differences](../kphp-language/kphp-vs-php/kphp-vs-php-differences.md).


## Features over PHP

KPHP analyzes your code as a whole and performs various optimizations focusing on performance and safety:

* Inferring [types of all variables](../kphp-language/static-type-system/type-inferring.md), how to declare them in C++.
* Compile-time optimizations, such as inlining getters or reducing refcounters flapping.
* Compile-time checks, including immutability and [type system requirements](../kphp-language/static-type-system/kphp-type-system.md).
* Runtime optimizations, like constant arrays pre-initing and typed vectors.

Aside from the above, KPHP has [coroutines](../kphp-language/best-practices/async-programming-forks.md). For now, however, they are almost inapplicable outside of VK code.


## Benchmarks 

Generally, when your code fits best practices, it runs **3–10 times faster than PHP**.

Take a look at the [**benchmarks page**](../kphp-language/kphp-vs-php/benchmarks.md) comparing KPHP and PHP performance.  
You can also refer to [**PHP vs KPHP vs C++**](../various-topics/walk-through-php-kphp-cpp.md).

KPHP isn't always faster than PHP, but it can be used to speed up your code:   
* Think about types and control them with PHPDoc, because [strict typing increases performance](../kphp-language/best-practices/strict-typing-performance.md).
* Use classes instead of associative arrays, as they are [much faster](../kphp-language/static-type-system/instances.md) in KPHP.
* Tweak memory and CPU usage with [built-in functions](../kphp-language/kphp-vs-php/list-of-additional-kphp-functions.md) and [profiler](../kphp-language/best-practices/embedded-profiler.md).
  
Another essential KPHP aspect is "zero-cost abstractions". Create an unlimited number of defines and constants — they don't exist at runtime. Simple functions like getters are inlined and have no overhead at all. 

```tip
KPHP performance compared to PHP:
* *"a random piece of PHP code can work faster"* — if you're lucky, then yes, but it might not.
* *"but I can optimize it to run faster"* — usually yes, focus on clean types and KPHP built-ins.
* *"KPHP is faster in large projects with tons of abstractions"* — usually yes.
```


## "I tried to compile PHP code, but failed"

This situation is quite common. KPHP rarely compiles already existing code without errors. It usually takes some time to rewrite PHP code for it to be compilable with KPHP.

Read more about this in [compiling an existing project](./compile-existing-project.md).


## Typical development with KPHP

* Use PHP for development: sync your IDE with the dev server and write code. It's uploaded automatically and just works — PHP is an interpreter.
* Use PHP for testing and bundling: PHPUnit can't and shouldn't be compiled.
* Compile a server with KPHP and deploy it to production.
* Monitor [logs](../kphp-server/deploy-and-maintain/logging.md) and [statistics](../kphp-server/deploy-and-maintain/statsd-metrics.md) on production.

You can organize your project in a way that one part of it would be compiled, and the other would work on PHP.


## The License

KPHP is distributed under the [GPLv3 license]({{site.url_license_file}}), on behalf of VK.com (V Kontakte LLC).


## Ask questions and provide feedback

To communicate with KPHP community, use [GitHub issues]({{site.url_github_issues}}) or a [Telegram chat]({{site.url_telegram_chat}}).

You can also take a look at our [FAQ page](./faq.md) and [Roadmap page](./roadmap.md).


## Contributing

If you've developed a useful PHP library that successfully compiles with KPHP, feel free to share it [here]({{site.url_github_kphp_snippets}}).

If you want to contribute to KPHP itself, try to dig into its internals. They're quite hard to take in all at once. Learn about [KPHP architecture](../kphp-internals/kphp-architecture/README.md) and examine imaginary [contribution scenarios](../kphp-internals/developing-and-extending-kphp/contributing-to-kphp.md).


## How do I start using KPHP?

Just follow the left menu. KPHP's documentation is organized in a natural learning order.

<div class="rst-footer-buttons" role="navigation" aria-label="footer navigation">
    <a href="{{ site.baseurl }}/kphp-basics/installation.html" class="btn btn-neutral float-right" accesskey="n" rel="next">
      Next: install KPHP and compile a sample PHP script <span class="fa fa-arrow-circle-right"></span>
    </a>
</div>
