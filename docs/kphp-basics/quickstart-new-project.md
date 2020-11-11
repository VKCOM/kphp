---
sort: 3
---

# Quickstart a new project

In this section, we'll discuss several aspects of starting small KPHP-based projects from scratch.


## The difference between "just compile a script" and a project

Here's what you need to take into consideration:
* You write PHP code in a local IDE, but the code may be executed on a remote server.
* You test your code using PHP during development and compile a KPHP binary for production.
* You use special KPHP built-in functions that don't exist in plain PHP by default.
* You use extended KPHP types and annotations, that IDE is unaware of.
* If your project is more than just a playground, you should take care of versioning, error logging, etc.

```tip
Below we'll address all of the above points.
```


## Set up the IDE

Use your IDE the same way you do when working on a regular PHP project. It is reliable without any additions.

But KPHP has an extended [type system](../kphp-language/static-type-system/kphp-type-system.md), and you can control types with *@var/@param/@return* PHPDocs. The IDE will not hint or autocomplete custom KPHP types (tuples, for example), even though the PHP code is correct.

<p class="pay-attention has-bg">
    Optional: install the KPHPStorm plugin for PhpStorm to support KPHP specifics.
</p>

Installing it is recommended, but optional. Read more about KPHPStorm [here](../kphp-language/kphpstorm-ide-plugin) or on [the landing page]({{site.url_website_kphpstorm}}) (in Russian).  


## Set up KPHP polyfills

Polyfills are PHP implementations of [functions](../kphp-language/kphp-vs-php/list-of-additional-kphp-functions.md) natively supported by KPHP.  
Without polyfills, invoking such functions would compile and work on KPHP, but fail on PHP.

Read more about polyfills [here](../kphp-language/php-extensions/php-polyfills.md).

<p class="pay-attention has-bg">
    Polyfills are in a Composer package, making them very easy to install.
</p>

```warning
Polyfills will be available on packagist soon, making the installation a one-line command; for now, use this:
```

* create `composer.json` with these contents:
```json
{
    "repositories": [
      {
        "type": "vcs",
        "url": "git@github.com:VKCOM/kphp-polyfills.git"
      }
    ],
    "require": {
      "vkcom/kphp-polyfills": "^1.0.0"
    }
}
```
* run `composer install`
* call autoload in the index file:
```php
require_once '.../vendor/autoload.php';
```

And that's it, you're done! All KPHP functions now work on plain PHP.


## Set up a PHP development server

You might already have a PHP development environment.    
If your code is executed remotely, you might typically sync your changes with SFTP, or use sshfs to make local changes instantly appear on the server. Most likely, you already have nginx or Apache server in front of PHP.  
For local purposes, you can just use an embedded PHP server: `php -S`

If you don't, you can easily start locally with the same Docker you already have installed for KPHP.  
Create a project in `~/kphp-project/` with `index.php` main file. Run this to map your folder to `/tmp/dev`:
```bash
# if you have installed KPHP via the Docker registry
docker run -ti -v ~/kphp-project/:/tmp/dev:rw -p 8080:8080 vkcom/kphp

# or, if you have build an image from the Dockerfile locally
docker run -ti -v ~/kphp-project/:/tmp/dev:rw -p 8080:8080 kphp
```
Then navigate to `/tmp/dev`, a mirror of your local folder.  
```bash
# (inside Docker)
cd /tmp/dev
php -S 0.0.0.0:8080
```

<p class="pay-attention has-bg">
    Use PHP for development, but keep in mind that the compilation will fail if you write incorrect code. 
</p>

For instance, PHPUnit can never be compiled by KPHP. You don't have to compile it, though, since testing is part of development and shouldn't be compiled for production. Read more [here](../kphp-language/howto-by-kphp/phpunit-mocks.md).


## Set up a KPHP compilation process

When you are certain that the PHP code works, it's time to compile it. 

<p class="pay-attention has-bg">
    The compilation is invoked by a single command: <code>kphp [options] index.php</code>
</p>

KPHP has many limitations and guidelines. Without experience, the compilation would probably fail even for small pieces of code. You would have to correct all the errors for it to run. Read about [KPHP vs PHP differences](../kphp-language/kphp-vs-php/kphp-vs-php-differences.md).

After compilation, you'll get a `./kphp_out/server` binary. We've already seen this when compiling a sample script.  
Stop the PHP server and launch the KPHP binary on the same port.
```bash
./kphp_out/server -H 8080
```

You can pass various [compiler options](../kphp-language/kphp-vs-php/compiler-cmd-options.md) to the `kphp` command or [execution options](../kphp-server/execution-options/server-cmd-options.md) to the `./server` binary.

There is a [kphp-inspector tool](../kphp-language/best-practices/kphp-inspector-tool.md) — a handy thing for querying and examining codegenerated C++ sources.


## Set up deployment and error logging

<p class="pay-attention has-bg">
    For simple scripts and playground projects, this might seem excessive to implement.
</p>

Read about [maintaining](../kphp-server/deploy-and-maintain), just follow the left menu.


## What's next?

Currently, KPHP can't connect to standard databases. While using KPHP as a proprietary tool, we never needed to implement support for that. VK has custom engines and data storage, which aren't made open source yet.

In other words, for now, KPHP may be interesting to experiment with. When all real-world necessities of such a tool are implemented, you would be able to try it for real projects.

This is explained in detail on [the next page](./compile-existing-project.md).
