<?php

namespace Foo\Bar;

function f() {
  define('C1', 10);
  define('Foo\Bar\C2', g());
}

function g() {
  return 20;
}
