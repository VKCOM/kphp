# CLAUDE.md

This file provides guidance to Claude Code (claude.ai/code) when working with code in this repository.

## Project Overview

KPHP is a programming language developed at VK and open-sourced in 2020.
This repository contains the KPHP compiler, which compiles a limited subset of PHP to native binaries,
along with two language runtimes. While KPHP focuses primarily on supporting PHP features, it also
includes additional functionality, such as asynchronous programming, a custom RPC client and server,
and various optimizations used at VK.

## Architecture Overview

KPHP consists of the following components:

### 1. Compiler (`/compiler/`)

Translates PHP to C++ through a pipeline:
- **Lexer** (`lexer.cpp`, `keywords.gperf`) - tokenizes PHP source
- **Parser** (`gentree.cpp`) - builds AST from tokens
- **AST Vertices** - defined in `vertex-desc.json`, auto-generated to `objs/generated/auto/`
- **Pipeline passes** (`/compiler/pipes/`) - transform AST through multiple stages
- **Type inferring** (`/compiler/inferring/`) - builds type graph and infers all variable types
- **Code generation** (`/compiler/code-gen/`) - outputs C++ from final AST

### 2. Common Runtime

Implements the PHP standard library and types used by both runtimes:
- **Types**: string, array, mixed, class_instance - all copy-on-write
- **Functions**: all PHP functions prefixed with `f$` (e.g., `f$array_merge`)
- **Memory management**: pre-allocated buffer with slab allocator

### 3. Legacy Runtime (`/runtime/`)

Implements the PHP standard library and types:
- **Async**: resumable functions for forks/RPC

Function declarations are in `/builtin-functions/kphp-full/_functions.txt`:
```
function array_merge ($a ::: array, ...$args) ::: ^1;
```

### 4. Server (`/server/`)

Pre-fork master/worker HTTP/RPC server architecture:
- Master process forks worker processes
- Workers accept TCP connections on a shared port
- Each worker handles one request at a time
- Supports keep-alive connections
- Signals: SIGKILL (immediate stop), SIGTERM (graceful shutdown), SIGUSR1 (log rotate)

### 5. vkext (`/vkext/`)

PHP extension for development that exposes the network layer and RPC/TL to plain PHP.

### 6. Common (`/common/`)

Shared utilities: TL parsing, RPC, crypto, allocators, containers.

### 7. New Runtime (K2) (`/runtime-light/`)

An alternative KPHP runtime that is asynchronous and re-entrant. It allows a single thread to switch
between multiple requests. Each KPHP program compiles into an image represented by a dynamic library.
A single image can produce multiple components (an image with passed arguments).
Incoming HTTP/RPC requests are handled by isolated, stateless component instances.

K2 runtime must follow these rules:
- It must be thread-safe, as a thread may switch to executing another instance at any time
- It must be stateless; all state is managed by the external K2 platform via the C API
  declared at `/runtime-light/k2-platform/k2-header.h`.

## Code Style

- C++17 for the Legacy Runtime, C++23 for the K2 runtime
- Use `.clang-format` and `.clangd` configs
- Follow existing patterns in the codebase

## Debugging

- Use `KPHP_VERBOSITY=3` for detailed compilation output
- C++ code is generated to `objs/generated/` (or the build directory)
- Use `objs/bin/kphp2cpp --help` for compiler options
- The `-M cli` mode compiles to a CLI binary instead of a server
