<?php

namespace Foo\Bar;

// This file doesn't import anything, but it defines a rand() function in
// its current namespace, so rand() refers to the local definition
// while \rand() refers to the global (builtin) definition.

function rand(): string {
  return 'random number';
}

function test() {
  var_dump(gettype(\rand()) !== gettype(rand()));
  var_dump(rand());
}

test();
