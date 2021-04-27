<?php

interface Iface1 {}

interface Iface2 {
  public function f();
}

interface Iface extends Iface1, Iface2 {}

abstract class BaseClass implements Iface {}

abstract class F extends BaseClass {
  public function g() {
    $this->f();
  }
}

class G extends F {
  public function f() {}
}

$g = new G();
