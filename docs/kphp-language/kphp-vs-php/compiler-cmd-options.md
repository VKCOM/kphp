---
sort: 5
---

# Compiler command-line options

Here we list command-line options that can be passed to the `kphp` command.  


## Options and environment variables 

Specifying options looks like this:
```bash
kphp [options] main-file.php
```

**Every option has a corresponding environment variable**. If both an environment variable is set, and an option is passed, an option has higher priority.

```bash
KPHP_ENV_VARIABLE_1=... kphp main-file.php
```

All options are either boolean or parametrized. Boolean options are false by default and become true when passed (they don't accept any command-line parameter).
  

## List of available compiler options

```warning
These are options for generating C++ code — NOT for a server. Server options are listed [here](../../kphp-server/execution-options/server-cmd-options.md)
```

<aside>--help / -h</aside>

Show all available options and exit.  
This documentation refers only important options, not absolutely all, so *-h* will show more than listed below.

<aside>--verbosity {level} / -v {level} / KPHP_VERBOSITY = {level}</aside>

Verbosity level, default **0**.  
Available levels: *0 | 1 | 2 | 3*.

<aside>--source-path {path} / -s {path} / KPHP_PATH = {path}</aside>

Path to KPHP source code. Some lookups will be done inside this folder, [functions.txt]({{site.url_functions_txt}}) for example.  
When using Docker or .deb packages, it equals to `/usr/share/vkontakte/kphp_source` directory.  
When compiling from sources, the default value of this argument is auto defined with CMake.

<aside>--mode {mode} / -m {mode} / KPHP_MODE = {mode}</aside>

The output binary type, default **server**.  
Available modes: *server | cli*. Use *cli* for [CLI mode](../../kphp-server/execution-options/cli-mode.md): just execute the script and exit.

<aside>--include-dir {path} / -I {path} / KPHP_INCLUDE_DIR = {path}</aside>

Paths where PHP files will be searched, it can appear multiple times, default empty.  
These directories are roots when *require_once* is used and where classes are searched by [autoloading rules](../kphp-vs-php/reachability-compilation.md#class-autoloading-and-composer-modules).

<aside>--destination-directory {path} / -d {path} / KPHP_DEST_DIR = {path}</aside>

Destination folder: where C++ sources and the resulting binary will be placed, default **'.'** in Docker.

<aside>--output-file {name} / -o {name} / KPHP_USER_BINARY_PATH = {name}</aside>

The resulting binary file name, default **server \| cli** depending on *\-\-mode*.

<aside>--no-make / KPHP_NO_MAKE = 0 | 1</aside>

Do not compile/link a binary after C++ codegeneration, default **0**. When 1, only the C++ code is generated.

<aside>--threads-count {n} / -t {n} / KPHP_THREADS_COUNT = {n}</aside>

Threads number for PHP → C++ codegeneration, default **CPU cores * 2**.

<aside>--jobs-num {n} / -j {n} / KPHP_JOBS_COUNT = {n}</aside>

Processes number to C++ parallel compilation/linkage, default **CPU cores**.

<aside>--globals-split-count {n} / KPHP_GLOBALS_SPLIT_COUNT = {n}</aside>

All global variables (const arrays also) are split into chunks of this size, default **1024**. If you have a few but very heavy global vars, lowering this number can decrease compilation time.

<aside>--tl-schema {file} / -T {file} / KPHP_TL_SCHEMA = {file}</aside>

A *.tl* file with [TL schema](../../kphp-client/tl-schema-and-rpc/tl-schema-basics.md), default empty.

<aside>--Werror / -W / KPHP_ERROR_ON_WARNINGS = 0 | 1</aside>

All compile-time warnings will be treated as errors, default **0**.

<aside>--warnings-file {file} / KPHP_WARNINGS_FILE = {file}</aside>

If passed, print all warnings to file, otherwise, they are printed to stderr, default empty.

<aside>--show-all-type-errors / KPHP_SHOW_ALL_TYPE_ERRORS = 0 | 1</aside>

Show all type infererring errors (if false, only the first error is shown), default **0**.

<aside>--colorize {mode} / KPHP_COLORS = {mode}</aside>

Colorize warnings output: *yes \| no \| auto*, default **auto**.

<aside>--compilation-metrics-file {file} / KPHP_COMPILATION_METRICS_FILE = {file}</aside>

If passed, save codegenerations metrics to file, default empty.

<aside>--php-code-version {version} / KPHP_PHP_CODE_VERSION = {version}</aside>

Specify the compiled PHP code version, default **'unknown'**.  
It will be embedded to the resulting binary and can be accessed from it — with `--version` on launch or by querying server status.

<aside>--cxx {executable} / KPHP_CXX = {executable}</aside>

C++ compiler for building the output binary, default **g++**.

<aside>--extra-cxx-flags {flags} / KPHP_EXTRA_CXXFLAGS = {flags}</aside>

Extra C++ compiler flags for building the output binary, default **-Os -ggdb -march=core2** + some others.

<aside>--extra-linker-flags {flags} / KPHP_EXTRA_LDFLAGS = {flags}</aside>

Extra linker flags for building the output binary, default **-ggdb**.

<aside>--dynamic-incremental-linkage / KPHP_DYNAMIC_INCREMENTAL_LINKAGE = 0 | 1</aside>

Use dynamic incremental linkage `ld` for building the output binary, default **0**, meaning that `KPHP_CXX` is used.

<aside>--profiler {mode} / -g {mode} / KPHP_PROFILER = {mode}</aside>

Enable [embedded profiler](../../kphp-language/best-practices/embedded-profiler.md), default **0**.  
Available modes: *0 | 1 | 2*. See the link above for details.

<aside>--enable-global-vars-memory-stats / KPHP_ENABLE_GLOBAL_VARS_MEMORY_STATS = 0 | 1</aside>

Enables *get_global_vars_memory_stats()* function and compiles debug code tracking memory, default **0**.

<aside>--no-pch / KPHP_NO_PCH = 1</aside>

Forbid to use precompiled headers, default **0**.

<aside>--show-progress / KPHP_SHOW_PROGRESS = 0 | 1</aside>

Show codegeneration progress, each step, line by line, default **0**.

<aside>--composer-root {path} / KPHP_COMPOSER_ROOT = {path}</aside> 

A folder that contains *composer.json* file and *vendor/* folder, default **empty**.  
If empty, KPHP won't search for Composer modules at all, so specify this option to enable psr-4 [autoload](../../kphp-language/kphp-vs-php/reachability-compilation.md#class-autoloading-and-composer-modules).

<aside>--composer-no-dev / KPHP_COMPOSER_NO_DEV = 0 | 1</aside>

Simulate the *-no-dev* flag behavior when handling Composer files, default **0**.

<aside>--require-functions-typing / KPHP_REQUIRE_FUNCTIONS_TYPING = 0 | 1</aside>

Makes typing of functions mandatory, default **0**. Navigate [here](../../kphp-language/static-type-system/phpdoc-to-declare-types.md#compiler-options-to-deny-untyped-code) for details.

<aside>--require-class-typing / KPHP_REQUIRE_CLASS_TYPING = 0 | 1</aside>

Makes typing of classes mandatory, default **0**. Navigate [here](../../kphp-language/static-type-system/phpdoc-to-declare-types.md#compiler-options-to-deny-untyped-code) for details.   
