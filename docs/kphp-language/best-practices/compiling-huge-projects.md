---
sort: 8
title: "Compiling huge projects"
---

# Compiling really huge projects

KPHP translates PHP code to C++, and then launches the C++ compiler (`KPHP_CXX`, typically g++) for codegenerated sources. At the first run, KPHP will compile everything. Afterward, it will compile only difference of cpp sources including recursive dependencies. KPHP also generates a precompiled header from all runtime sources, which is included to every codegenerated cpp file. 

For small projects, which result in just hundreds of cpp files, local g++ compilation is okay and takes a small amount of time. KPHP launches and balances g++ in `KPHP_JOBS_COUNT` (or `-j / --jobs-num`) simultaneous processes, which defaults to the number of CPU cores — to minimize the C++ compilation time.

However, if your PHP project is huge and results if tons of cpp files, it may take minutes or even hours to compile C++ from scratch (though diff recompilation could be still small and fast).

The only way you can speed up (re)compilation is to parallelize g++ invocations across many machines.


## KPHP + distcc

KPHP easily integrates with [distcc](https://www.distcc.org/) — a distributed C++ compiler. You should install distcc, launch remote servers, and configure local distcc according to its documentation to provide servers' addresses.

To make KPHP use distcc instead of local compilation, just set env vars (or corresponding kphp2cpp [options](../kphp-vs-php/compiler-cmd-options.md)):
```bash
KPHP_CXX=distcc g++  # not g++
KPHP_JOBS_COUNT=400  # much more than local CPU cores, depends on how many distcc agents you have
```


## KPHP + nocc

[nocc]({{site.url_github_nocc}}) is also a distributed C++ compiler, a replacement for distcc, it works much faster, especially for KPHP case. It was developed in VK.com by KPHP developers to deal with a giant VK codebase.

Using nocc is equal to using distcc:
```bash
KPHP_CXX=nocc g++    # not g++
KPHP_JOBS_COUNT=600  # much more than local CPU cores, depends on how many nocc servers you have
```

For installation, configuration and comparison with distcc, visit

<a href="{{ site.url_github_nocc }}" class="btn-github-page">
  <span class="icon icon-github"></span>
  <span>nocc on GitHub</span>
</a>


## An article about nocc

Here's an article on Habr (in Russian) about nocc and its implementation:

**[nocc — распределённый компилятор для гигантских проектов на С++](https://habr.com/ru/company/vk/blog/694536/)**
