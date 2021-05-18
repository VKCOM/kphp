<?php

namespace Test;

abstract class Base {
  public abstract function f();
}

class Derived extends Base {
  public $v = 0;
  public function f() { return $v; }
}

function f(Base $b) {
  if ($b instanceof Derived) {
    var_dump($b->v);
  }
}

$d = new Derived();
f($d);
