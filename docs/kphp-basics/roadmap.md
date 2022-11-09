---
sort: 8
noprevnext: true
title: Roadmap
---

# Roadmap

Here is a list of epics that are expected to be implemented some time in the future.  
Small feature requests are present in [GitHub issues]({{site.url_github_issues}}).


## Language features

* support most of the PHP 8 syntax
* modules inside PHP (internal classes inside namespaces, usage scopes, etc.), as well as a plugin for IDE
* generic classes (generic functions already exist), as well as integration of generics into KPHPStorm
* vector\<T> and map\<K,V> instead of a usual PHP array
* partial support for Reflection API
* nullability tracking
* type aliases
* phantom types


## Backend / server features

* tracing API that outputs a timeline of an HTTP query for OpenTelemetry/Sentry
* conan integration, to make it easier to install KPHP from sources
* streaming TCP sockets
* $_SESSION
* built-in support for MySQL, Postgres, Tarantool, Redis
* extend PDO support (prepared statements, etc.)
