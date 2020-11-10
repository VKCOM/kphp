---
sort: 1
---

# Implementation and architecture

Main KPHP parts are the following:
1. [**compiler**](./internals-compiler.md) — translates PHP sources to C++ sources
2. [**runtime**](./internals-runtime.md) — implements PHP standard library and PHP types (strings, arrays, etc.)
3. [**server**](./internals-server.md) — handles HTTP requests and performs master/worker orchestration
4. [**vkext**](./internals-vkext.md) — exposes network layer and RPC/TL as a PHP extension for development

You can see all these parts as top-level folders of the KPHP repository.

