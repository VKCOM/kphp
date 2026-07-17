@ok
<?php

// Regression test for https://github.com/VKCOM/kphp/issues/57
// When a child class does not define __clone() but a parent does,
// cloning an instance of the child must invoke the parent's __clone().

class Base {
  public int $value;

  public function __construct(int $v) {
    $this->value = $v;
  }

  public function __clone() {
    $this->value *= 2;
  }
}

class Child extends Base {
  // No __clone() here — must inherit Base::__clone()
}

$obj = new Child(5);
$copy = clone $obj;

// Base::__clone() doubles value: original stays 5, copy becomes 10
var_dump($obj->value);   // int(5)
var_dump($copy->value);  // int(10)
