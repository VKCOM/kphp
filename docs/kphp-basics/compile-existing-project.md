---
sort: 4
---

# Compile your existing project

After having compiled small samples from scratch, you'd probably try to use KPHP with existing code.  
There's a catch to it, though.


## It will fail

Remember: KPHP has [a lot of limitations](../kphp-language/kphp-vs-php/kphp-vs-php-differences.md) and guidelines. There's a 100% chance that KPHP will refuse to compile any large PHP library.

Typically, PHP code uses uncompilable approaches for many commonly used features, such as routing, invoking methods by name, precaching, reflection, DB interaction, ORMs with accessing fields by name, and so on. 

```note
This is PHP-style, but compiled languages use other scenarios for the same goals.  
If you want your code to be compiled, you would have to rewrite many such parts.
```

Another disappointing thing to note is that databases/memcache interactions are [almost unsupported](../kphp-client/working-with-external-db). At VK, where KPHP was developed, we use our own storage engines. That's why using standard PHP libraries for databases will fail when run with the first public release of KPHP. Technically, this can and probably will be fixed in the future. For now, though, these abilities are totally missed.

```warning
Treat KPHP as a playground tool, test simple scripts and take a look at how it works.  
Once real-world connectors to memcache/DBs are ready, you can attempt to try KPHP in large projects.
```

When KPHP introduces missing features, you'll still need to rewrite lots of code. But here's an important thing to keep in mind: **code rewritten for KPHP remains valid and working PHP code**. You rewrite code step by step, and every iteration is tested and deployed to production still working on PHP, until it compiles. Rewriting code like this is completely safe.


## Conclusion

Switching from PHP to Go is impossible. You'd need to rewrite the entirety of your code. And until you are done rewriting it, it's not ready for production.  
Switching from PHP to KPHP will be possible in the future. Adapting PHP code is an iterative process that is tested in production every step of the way.
