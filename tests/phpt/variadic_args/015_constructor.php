@ok
<?php

class A {
  public $args;
  public function __construct(...$args) {
    var_dump(!!$this);
    var_dump($args);
    $this->args = $args;
  }
};

$x = new A(1, 2, 3);
var_dump($x->args);
