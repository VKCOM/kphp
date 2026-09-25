@ok
<?php

// Regression for #57: multi-level inheritance — __clone defined two levels up.

class GrandParent {
  public string $log = '';

  public function __clone() {
    $this->log .= '+cloned';
  }
}

class Parent_ extends GrandParent {
  // no __clone
}

class Child extends Parent_ {
  // no __clone either — must still reach GrandParent::__clone
}

$obj = new Child();
$obj->log = 'original';
$copy = clone $obj;

var_dump($obj->log);   // string(8) "original"
var_dump($copy->log);  // string(15) "original+cloned"
