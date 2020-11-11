---
sort: 3
---

# Combining KPHP and PHP code

```tip
You can organize your project in a way, that part of it would be compiled, and part of it would work on PHP.  
For example, file uploading can remain PHP-based, whereas API business logic works on KPHP.
```

This documentation doesn't possess strict guidelines for organizing the code of your project.

Typically, if some parts use PHP on production, the easiest way is to deploy all PHP code — though some of it would never be executed. Deployment of PHP code and KPHP binary supposed to have the same versioning and restart/rollback policy.

While writing nginx configs, you should decide, how you would detect where to proxy a current request — to KPHP or to PHP upstream. Generally, nothing better than determining this by location can't be done — it means, that you should not only organize your code skillfully not to be messed up, but also organize URL mapping to unambiguously detect the correct handler.

You also use PHP for development purposes: unit testing / linting / etc. This PHP code is [invisible to KPHP](../../kphp-language/howto-by-kphp/phpunit-mocks.md) and is typically not deployed at all. 
