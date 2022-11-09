---
sort: 6
noprevnext: true
---

# FAQ

This section contains helpful notes and brief answers to common questions about KPHP.

```note
If you have something to ask besides these topics, use [GitHub issues]({{site.url_github_issues}}) or a [Telegram chat]({{site.url_telegram_chat}}).
```

<hr>


<blockquote class="faq">Is KPHP faster than PHP? To what extent?</blockquote>

Typically, KPHP is 3–10 times faster, especially if you focus on clean types.  
Read more in-depth information about this on the [benchmarks page](../kphp-language/kphp-vs-php/benchmarks.md).


<blockquote class="faq">If KPHP is a compiler, how does it serve HTTP requests?</blockquote>

Generated C++ code in coupled with [a web server](../kphp-server/kphp-as-backend/web-server.md), but you can also launch your script in [CLI mode](../kphp-server/execution-options/cli-mode.md).


<blockquote class="faq">PHP 7 introduced type hints, isn't that enough?</blockquote>

With PHP 7 type hints, you can use primitives and ?nullables, but you can't express *T\|false* or *int[]*.  
With PHP 8 union types, you can use *int\|false*, but you still can't express *int[]* or *(int|false)[]\|false*.

KPHP type system is [much more expansive](../kphp-language/static-type-system/kphp-type-system.md): besides primitives and typed arrays, it contains *tuples*, *shapes*, and others. They can't be expressed by PHP syntax and should be written as [PHPDoc](../kphp-language/static-type-system/phpdoc-to-declare-types.md) only.


<blockquote class="faq">Does IDE understand KPHP extended types?</blockquote>

[KPHPStorm](../kphp-language/kphpstorm-ide-plugin/README.md) is a PhpStorm plugin, that makes IDE understand extended PHPDoc and highlight mismatch types on the fly, all without compilation.


<blockquote class="faq">Is there something like "duck typing", regardless of strict typing?</blockquote>

For this, you might want to read about [generic functions](../kphp-language/static-type-system/generic-functions.md).


<blockquote class="faq">Why can't KPHP compile my code?</blockquote>

That's because KPHP can't compile *just any random PHP code*. Your code must follow strict guidelines.  
Take a look at the information on [this page](./compile-existing-project.md).


<blockquote class="faq">Why doesn't KPHP support Postgres and all other DBs?</blockquote>

KPHP was initially developed at VK, which uses self-written storage engines that are not made open source yet.   
That's why KPHP has great support for [TL/RPC](../kphp-client/tl-schema-and-rpc/README.md), but almost nothing for "real-world" DBs.


<blockquote class="faq">Will Postgres and other DBs be supported in the future?</blockquote>

Probably. They can be added, but the main obstacle is in making them [async](../kphp-language/best-practices/async-programming-forks.md).


<blockquote class="faq">Are PHP extensions compatible with KPHP?</blockquote>

No. Zend API has no relation to internal KPHP runtime.


<blockquote class="faq">How to use PHPUnit with KPHP?</blockquote>

Don't use it in production. Test your site on PHP, and then compile a binary without dev tools.  
Read more [in the dedicated article](../kphp-language/howto-by-kphp/phpunit-mocks.md).


<blockquote class="faq">Is there a "list of libraries" compatible with KPHP?</blockquote>

Consult [this repository]({{site.url_github_kphp_snippets}}) containing PHP snippets which you can copy and paste. Something more sophisticated might be added in the future.


<blockquote class="faq">Where do I find logs and runtime errors?</blockquote>

KPHP is a server, and it writes [a lot of logs](../kphp-server/deploy-and-maintain/logging.md) and [statistics](../kphp-server/deploy-and-maintain/statsd-metrics.md).  
When a runtime error occurs, a C++ trace is logged. To map this trace onto PHP code, you'll need [special magic](../kphp-server/deploy-and-maintain/map-cpp-trace-to-php.md).


<blockquote class="faq">Does KPHP compile the whole project from scratch every time?</blockquote>

KPHP is incremental. It does not recompile C++ sources that haven't changed since the previous run.   
KPHP **parses** all reachable *.php* files on every run. Without parsing everything, type inferring is impossible. But it rewrites and invokes *g++* only on files that were changed by themselves or through dependencies. This is especially useful in huge projects where compiling C++ code takes a long time. 


<blockquote class="faq">Who uses KPHP except for VK.com?</blockquote>

As of the first public release? Nobody. Standard database support needs to be added to greatly improve potential KPHP applicability.


<blockquote class="faq">Сan you say a few words about KPHP internals?</blockquote>

Here is [a separate section about KPHP architecture](../kphp-internals/kphp-architecture/README.md), not just a few words :)


<div class="rst-footer-buttons" role="navigation" aria-label="footer navigation">
    <a href="{{ site.baseurl }}/kphp-language/kphp-vs-php/kphp-vs-php-differences.html" class="btn btn-neutral float-right" accesskey="n" rel="next">
      Next: KPHP vs PHP <span class="fa fa-arrow-circle-right"></span>
    </a>
</div>
