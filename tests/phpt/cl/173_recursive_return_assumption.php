@ok
<?php

class A {
  public $x = 0;
}

function g() {
  if (1) return new A();
  return f();
}

function f() {
  if (1) return new A();
  return g();
}

f()->x = 5;
