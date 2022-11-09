---
sort: 7
title: Examine codegenerated sources
---

# kphp-inspector: query codegenerated sources

After KPHP converts PHP to C++, lots of codegenerated files appear in the output folder. It may be a point of interest to investigate such questions:
* How does the C++ source of a specific function look like?
* How types of local vars and arguments were inferred?
* Was the function automatically inlined?

**kphp-inspector** is a console tool to analyze codegenerated sources.

<img width="474" alt="screen" src="https://user-images.githubusercontent.com/67757852/92613543-c1ab6080-f2c3-11ea-96d6-ee2d4a6c09c8.png">

It is not a part of the KPHP compiler, it is kept and maintained in another repository — **kphp-tools**.

<a href="{{ site.url_github_kphp_tools }}/tree/master/kphp-inspector" class="btn-github-page">
  <span class="icon icon-github"></span>
  <span>Visit kphp-inspector page</span>
</a>
