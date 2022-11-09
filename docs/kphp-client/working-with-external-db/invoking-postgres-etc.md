---
sort: 3
---

# Invoking postgres / redis / etc

At VK.com, we have lots of self-written engines, and we were never in need of standard databases.  
That's why KPHP doesn't support them natively for now.

Not so long ago, we ended up with a production solution connecting SQLite and PostgreSQL using [FFI](../../kphp-language/php-extensions/ffi.md). 
Some time later we'll expose that snippets as public libraries which encapsulate FFI usage. 

So, if you need DBs in production, they will work 100% having some FFI magic. Please contact us to request help.

```danger
All in all, connecting to external DBs is the most crucial aspect of practical usage. We'll provide some public solutions in the near future to make KPHP be easier usable for our open source clients.
```
