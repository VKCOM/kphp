---
sort: 6
---

# Benchmarks

KPHP has strong limitations in its area of applicability, but when code compiles, it runs much faster than PHP.


## Zend benchmarks

```danger
Comparing performance on Zend benchmarks is absolutely synthetic and done just formally.  
```

We'll take [micro_bench.php](https://github.com/php/php-src/blob/master/Zend/micro_bench.php) and [bench.php](https://github.com/php/php-src/blob/master/Zend/bench.php) from the PHP repository.   
Compiling them without modification fails, but after removing a couple of tests and correcting errors, they can be easily launched. 

It comes about such results for 5 runs (PHP 7.4 with OpCache and without XDebug):

|  micro_bench — PHP 7.4 | micro_bench — KPHP | bench — PHP 7.4 | bench — KPHP |
|:-----------------------|:-------------------|:----------------|:-------------|
| 3.383 | 0.459 | 0.620 | 0.120 |
| 3.183 | 0.471 | 0.632 | 0.121 |
| 3.261 | 0.454 | 0.773 | 0.121 |
| 3.336 | 0.460 | 0.660 | 0.117 |
| 3.320 | 0.456 | 0.610 | 0.116 |
| **avg PHP 7.4** | **avg KPHP** | **avg PHP 7.4** | **avg KPHP** |
| 3.297 | 0.460 | 0.659 | 0.119 |

According to these benchmarks, KPHP is found to be 5–7 times faster.  
Although they are quite synthetic, this is pretty much similar to other measurements.


## Compare PHP vs KPHP vs C++

Consider the [**dedicated page**](../../various-topics/walk-through-php-kphp-cpp.md). It also shows a 4–10 times breakthrough.  

```note
The main idea to pick from that text: as far as your project grows, using KPHP becomes more reliable.  
```

This can be called *"zero-cost abstractions"* — not zero of course, but considerably smaller than in PHP:
* all defines and class constants are inlined at compile-time
* all simple methods, like getters and setters, are inlined and have no invocation overhead
* access modifiers are checked at compile-time, they don't exist at runtime at all
* class fields access is a compile-time known memory offset, without any hashtable lookups
* … and many others

```tip
But still: the only adequate way to compare performance is to take a real working high-loaded site.
```


## VK.com pages generation time

If we measure PHP 7.2 version and KPHP-compiled on the same server, for a random account, we get

<p class="pay-attention">
  <i>vk.com/feed</i> — PHP: 10.1 sec, KPHP: 0.95 sec<br>
  <i>vk.com/im</i> — PHP: 1.25 sec, KPHP: 0.46 sec<br>
  <i>vk.com/ads</i> — PHP: 0.73 sec, KPHP: 0.20 sec
</p>

In general, for VK.com we have **from 3 to 10 times** average profit. Pretty much to suffer a bit, right?

Having a goal to optimize particular places, you can achieve even better results.


## Machine-learning models with lots of maths

VK.com has some heavy pieces of business logic, which perform ML calculations in real-time. Such areas were specially optimized, fulfilling all KPHP potential.

Measuring that isolated pieces of code shows tremendous superiority of strict typing:
<p class="pay-attention">
  <i>neural networks with huge matrices</i> — PHP: ~8 sec, KPHP: 0.24 sec<br>
  <i>deep decision trees with tons of branches</i> — PHP: ~5 sec, KPHP: 0.14 sec
</p>

<hr> 

```danger
But. It's a bad idea to state "KPHP is always faster than PHP". 
```

It may be wrong. If PHP code just calls *strpos()* or *parse_url()*, KPHP would have almost no benefits, as they are just PHP standard functions implemented in C. But the more business logic is PHP-implemented, the faster KPHP becomes in comparison — especially if you write PHP code keeping types in mind.  

**You can** speed up your code — if you really ought to. 
